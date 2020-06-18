/************************************************************************
Copyright 2019-2020 eBay Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#include "CryptoUtil.h"

#include <google/protobuf/text_format.h>
#include <vector>

#include "FileUtil.h"
#include "TimeUtil.h"
#include "../es/store/generated/store.pb.h"

namespace gringofts {

void CryptoUtil::init(const INIReader &reader) {
  if (mEnabled) {
    SPDLOG_ERROR("Set AES key twice.");
    exit(1);
  }

  bool enableCrypt = reader.GetBoolean("aes", "enable", true);
  std::string keyFileName = reader.Get("aes", "filename", "");

  // Three cases and actions:
  // 1. Both 'enable' and 'filename' are not set:
  //     Ignore encrypt and do nothing.
  // 2. Both 'enable' and 'filename' are set:
  //     If 'enable' is true and filename is legal, encrypt.
  // 3. Set 'filename' and ignore 'enable':
  //     Encrypt.
  if (!enableCrypt || keyFileName.empty()) {
    SPDLOG_WARN("Raft log is plain.");
    return;
  }

  std::string content = FileUtil::getFileContent(keyFileName);
  gringofts::es::EncryptSecKeySet keySet;
  assert(google::protobuf::TextFormat::ParseFromString(content, &keySet));
  mAllKeys.clear();
  mDescendingSecKeyVersions.clear();
  for (auto keyAndVersion : keySet.keys()) {
    auto version = keyAndVersion.version();
    assert(version > SecretKey::kInvalidSecKeyVersion);
    mAllKeys[version].mVersion = version;
    decodeBase64Key(keyAndVersion.key(), mAllKeys[version].mKey, SecretKey::kKeyLen);
    if (version > mLatestVersion) {
      mLatestVersion = version;
    }
    mDescendingSecKeyVersions.push_back(version);
  }
  sort(mDescendingSecKeyVersions.rbegin(), mDescendingSecKeyVersions.rend());
  assert(mLatestVersion > 0);
  assert(!mAllKeys.empty());
  assert(!mDescendingSecKeyVersions.empty());
  mLatestVersionGauge.set(mLatestVersion);

  mEnabled = true;
  SPDLOG_INFO("Raft log and snapshot will be encrypted.");
}

void CryptoUtil::init(SecKeyVersion version, const std::string &key) {
  if (mEnabled) {
    SPDLOG_ERROR("Set AES key twice.");
    exit(1);
  }
  if (key.size() != SecretKey::kKeyLen || version <= SecretKey::kInvalidSecKeyVersion) {
    SPDLOG_ERROR("invalid key size or version, expect {}, got {}", SecretKey::kKeyLen, key.size());
    exit(1);
  }

  mAllKeys.clear();
  mDescendingSecKeyVersions.clear();
  mAllKeys[version].mVersion = version;
  memcpy(mAllKeys[version].mKey, key.c_str(), SecretKey::kKeyLen);
  mLatestVersion = version;
  mDescendingSecKeyVersions.push_back(version);
  assert(mLatestVersion > 0);
  assert(mAllKeys.size() == 1);
  assert(mDescendingSecKeyVersions.size() == 1);
  mLatestVersionGauge.set(mLatestVersion);

  mEnabled = true;
  SPDLOG_INFO("Raft log and snapshot will be encrypted.");
}

void CryptoUtil::assertValidVersion(SecKeyVersion version) const {
  if (mAllKeys.find(version) == mAllKeys.end()) {
    SPDLOG_ERROR("invalid key version: {}", version);
    exit(1);
  }
}

int CryptoUtil::encrypt(std::string *payload, SecKeyVersion version) const {
  if (!mEnabled) {
    return 0;
  }
  assertValidVersion(version);

  std::vector<unsigned char> buffer;
  buffer.resize(payload->size() + AES_BLOCK_SIZE);

  auto cipherLen = 0;
  auto res = encrypt(reinterpret_cast<const unsigned char *>(payload->c_str()),
                     payload->size(), mAllKeys.at(version).mKey, mIV, &buffer[0], &cipherLen);
  if (res == 0) {
    payload->assign(reinterpret_cast<const char *>(&buffer[0]),
                    cipherLen);
  }
  return res;
}

int CryptoUtil::decrypt(std::string *payload, SecKeyVersion version) const {
  if (!mEnabled) {
    return 0;
  }
  assertValidVersion(version);

  std::vector<unsigned char> buffer;
  buffer.resize(payload->size());

  auto plainLen = 0;
  auto res = decrypt(reinterpret_cast<const unsigned char *>(payload->c_str()),
                     payload->size(), mAllKeys.at(version).mKey, mIV, &buffer[0], &plainLen);
  if (res == 0) {
    payload->assign(reinterpret_cast<const char *>(&buffer[0]), plainLen);
  }
  return res;
}

std::string CryptoUtil::hmac(const std::string &payload, SecKeyVersion version) const {
  return hmac(reinterpret_cast<const unsigned char *>(payload.c_str()),
              payload.size(), version);
}

std::string CryptoUtil::hmac(const unsigned char *d, std::size_t n, SecKeyVersion version) const {
  if (!mEnabled) {
    return "";
  }
  assertValidVersion(version);

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int len = 0;

  /// Using sha256 hash engine here.
  /// You may use other hash engines. e.g., EVP_md5(), EVP_sha224(), EVP_sha512(), etc
  auto *ptr = HMAC(EVP_sha256(), mAllKeys.at(version).mKey, SecretKey::kKeyLen, d, n, digest, &len);

  assert(ptr != nullptr);
  assert(len == 32);  /// output length of SHA256 should be 32 bytes (a.k.a. 256 bits).

  return std::string(reinterpret_cast<const char *>(digest), len);
}

void CryptoUtil::decodeBase64Key(const std::string &base64,
                                 unsigned char *key, int keyLen) {
  BIO *bio;
  BIO *b64;

  bio = BIO_new_mem_buf(base64.c_str(), base64.length());
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_push(b64, bio);

  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // Do not use newlines to flush buffer

  auto len = BIO_read(bio, key, keyLen);
  if (len != keyLen) {
    // len should equal keyLen, else something went horribly wrong
    SPDLOG_ERROR("Decode AES key error. expected: {}, actual: {}", SecretKey::kKeyLen, len);
    exit(1);
  }
  BIO_free_all(bio);
}

/// TODO(https://jirap.corp.ebay.com/browse/RTCUTOFF-3882): Deprecate fam/fas state machine v1 (full memory)
int CryptoUtil::handleErrors() {
  ERR_print_errors_fp(stderr);
  return -1;
}

int CryptoUtil::encrypt(const unsigned char *plain, int plainLen,
                        const unsigned char *key, const unsigned char *iv,
                        unsigned char *cipher, int *cipherLen) {
  EVP_CIPHER_CTX *ctx;

  int len;

  /** Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    return handleErrors();
  }

  /**
   * Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
    return handleErrors();
  }

  /**
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if (1 != EVP_EncryptUpdate(ctx, cipher, &len, plain, plainLen)) {
    return handleErrors();
  }
  *cipherLen = len;

  /**
   * Finalise the encryption. Further cipher bytes may be written at
   * this stage.
   */
  if (1 != EVP_EncryptFinal_ex(ctx, cipher + len, &len)) {
    return handleErrors();
  }
  *cipherLen += len;

  /** Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return 0;
}

int CryptoUtil::decrypt(const unsigned char *cipher, int cipherLen,
                        const unsigned char *key, const unsigned char *iv,
                        unsigned char *plain, int *plainLen) {
  EVP_CIPHER_CTX *ctx;

  int len;

  /** Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    return handleErrors();
  }

  /**
   * Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
    SPDLOG_ERROR("error when decrypting init");
    EVP_CIPHER_CTX_free(ctx);
    return handleErrors();
  }

  /**
   * Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if (1 != EVP_DecryptUpdate(ctx, plain, &len, cipher, cipherLen)) {
    SPDLOG_ERROR("error when decrypting update");
    EVP_CIPHER_CTX_free(ctx);
    return handleErrors();
  }
  *plainLen = len;

  /**
   * Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if (1 != EVP_DecryptFinal_ex(ctx, plain + len, &len)) {
    SPDLOG_ERROR("error when finalize decrypting");
    EVP_CIPHER_CTX_free(ctx);
    return handleErrors();
  }
  *plainLen += len;

  /** Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return 0;
}

void CryptoUtil::beginEncryption(uint64_t bufferSize) {
  if (!mEnabled) {
    SPDLOG_WARN("AES is not enabled, will not encrypt.");
    return;
  }

  /** Create and initialise the context */
  if (!(mCtx = EVP_CIPHER_CTX_new())) {
    handleErrors();
  }

  /**
   * Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (1 != EVP_EncryptInit_ex(mCtx, EVP_aes_256_cbc(), nullptr, mAllKeys.at(mLatestVersion).mKey, mIV)) {
    handleErrors();
  }

  mCurrentOffset = 0;
  assert(bufferSize <= kPlainMaxLen);
  mPlainBufferSize = bufferSize;
  mPlainBuffer.resize(mPlainBufferSize);
  mCipherBufferSize = mPlainBufferSize + AES_BLOCK_SIZE;
  mCipherBuffer.resize(mCipherBufferSize);
}

void CryptoUtil::encryptUint64ToFile(std::ofstream &ofs, uint64_t content) {
  if (mEnabled) {
    bufferOrEncrypt(ofs, reinterpret_cast<const unsigned char *>(&content), sizeof(uint64_t));
  } else {
    FileUtil::writeUint64ToFile(ofs, content);
  }
}

void CryptoUtil::encryptStrToFile(std::ofstream &ofs, const std::string &content) {
  auto len = content.length();
  if (mEnabled) {
    encryptUint64ToFile(ofs, len);
    bufferOrEncrypt(ofs, reinterpret_cast<const unsigned char *>(content.c_str()), len);
  } else {
    FileUtil::writeStrToFile(ofs, content);
  }
}

void CryptoUtil::bufferOrEncrypt(std::ofstream &ofs, const unsigned char *content, size_t length) {
  auto leftLen = length;

  while (leftLen + mCurrentOffset >= mPlainBufferSize) {
    auto copyLen = mPlainBufferSize - mCurrentOffset;
    ::memmove(mPlainBuffer.data() + mCurrentOffset, content + length - leftLen, copyLen);

    /// encrypt
    int cipherLen = 0;
    if (1 != EVP_EncryptUpdate(mCtx, mCipherBuffer.data(), &cipherLen, mPlainBuffer.data(), mPlainBufferSize)) {
      handleErrors();
    }
    assert(cipherLen <= mCipherBufferSize);

    /// write to file
    auto ts1InNano = TimeUtil::currentTimeInNanos();

    /// time elapse between two flush should be larger than 1s
    while (TimeUtil::currentTimeInNanos() - mLastFlushTimeInNano < 1000 * 1000 * 1000) {   /// 1s
      usleep(1);
    }

    auto ts2InNano = TimeUtil::currentTimeInNanos();

    ofs.write(reinterpret_cast<const char *>(mCipherBuffer.data()), cipherLen);
    FileUtil::checkFileState(ofs);

    auto ts3InNano = TimeUtil::currentTimeInNanos();

    SPDLOG_INFO("flush {}MiB data in cipher buffer, defer cost {}ms, flush cost {}ms",
                cipherLen / 1024.0 / 1024.0,
                (ts2InNano - ts1InNano) / 1000.0 / 1000.0,
                (ts3InNano - ts2InNano) / 1000.0 / 1000.0);

    /// update
    mLastFlushTimeInNano = ts2InNano;
    mCurrentOffset = 0;
    leftLen -= copyLen;
  }

  /// copy the rest of the content
  ::memmove(mPlainBuffer.data() + mCurrentOffset, content + length - leftLen, leftLen);
  mCurrentOffset += leftLen;
}

void CryptoUtil::commitEncryption(std::ofstream &ofs) {
  if (!mEnabled) {
    SPDLOG_WARN("AES is not enabled, will not commit encryption.");
    return;
  }

  /// encrypt the rest of the data and write to file
  auto len = 0;
  if (1 != EVP_EncryptUpdate(mCtx, mCipherBuffer.data(), &len, mPlainBuffer.data(), mCurrentOffset)) {
    handleErrors();
  }
  mCurrentOffset = 0;

  auto totalLen = len;
  /// Finalise the encryption. Further cipher bytes may be written at this stage
  if (1 != EVP_EncryptFinal_ex(mCtx, mCipherBuffer.data() + len, &len)) {
    handleErrors();
  }
  totalLen += len;
  assert(totalLen <= mCipherBufferSize);

  /// Clean up
  EVP_CIPHER_CTX_free(mCtx);

  /// Write to file
  ofs.write(reinterpret_cast<const char *>(mCipherBuffer.data()), totalLen);
  FileUtil::checkFileState(ofs);
}

void CryptoUtil::beginDecryption(uint64_t bufferSize) {
  if (!mEnabled) {
    SPDLOG_WARN("AES is not enabled, will not decrypt.");
    return;
  }

  /** Create and initialise the context */
  if (!(mCtx = EVP_CIPHER_CTX_new())) {
    handleErrors();
  }

  /**
   * Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (1 != EVP_DecryptInit_ex(mCtx, EVP_aes_256_cbc(), nullptr, mAllKeys.at(mLatestVersion).mKey, mIV)) {
    handleErrors();
  }

  mCurrentOffset = 0;
  assert(bufferSize <= kPlainMaxLen);
  mPlainBufferSize = 0;
  mPlainBuffer.resize(bufferSize);
  mCipherBufferSize = bufferSize + AES_BLOCK_SIZE;
  mCipherBuffer.resize(mCipherBufferSize);
}

uint64_t CryptoUtil::decryptUint64FromFile(std::ifstream &ifs) {
  if (mEnabled) {
    uint64_t content;
    readOrDecrypt(ifs, reinterpret_cast<unsigned char *>(&content), sizeof(uint64_t));
    return content;
  } else {
    return FileUtil::readUint64FromFile(ifs);
  }
}

std::string CryptoUtil::decryptStrFromFile(std::ifstream &ifs) {
  if (mEnabled) {
    auto length = decryptUint64FromFile(ifs);
    std::vector<char> buffer;
    buffer.resize(length + 1);
    readOrDecrypt(ifs, reinterpret_cast<unsigned char *>(buffer.data()), length);
    buffer[length] = '\0';
    return std::string(buffer.data());
  } else {
    return FileUtil::readStrFromFile(ifs);
  }
}

void CryptoUtil::readOrDecrypt(std::ifstream &ifs, unsigned char *buffer, std::size_t length) {
  if (!mEnabled) {
    return;
  }

  auto leftLen = length;

  /// clean and reload plain buffer if needed
  while (!ifs.eof() && mCurrentOffset + leftLen > mPlainBufferSize) {
    /// clear plain buffer
    auto copyLen = mPlainBufferSize - mCurrentOffset;
    ::memmove(buffer + length - leftLen, mPlainBuffer.data() + mCurrentOffset, copyLen);

    /// update leftLen
    leftLen -= copyLen;

    /// read encrypt data from file
    ifs.read(reinterpret_cast<char *>(mCipherBuffer.data()), mCipherBufferSize);
    auto encryptLen = ifs.gcount();

    assert(encryptLen <= mCipherBufferSize);

    /// reload plain buffer
    auto plainLen = 0;
    if (1 != EVP_DecryptUpdate(mCtx, mPlainBuffer.data(), &plainLen, mCipherBuffer.data(), encryptLen)) {
      handleErrors();
    }

    mPlainBufferSize = plainLen;
    mCurrentOffset = 0;
    SPDLOG_INFO("reload plain buffer, encryptLen={}, plainLen={}", encryptLen, mPlainBufferSize);

    if (ifs.eof()) {
      /**
       * Finalise the decryption. Further plaintext bytes may be written at
       * this stage.
       * IMPORTANT: below method can be called multiple times, returning exactly the same result.
       * Make sure it is only called ONCE.
       */
      auto extraLen = 0;
      if (1 != EVP_DecryptFinal_ex(mCtx, mPlainBuffer.data() + mPlainBufferSize, &extraLen)) {
        handleErrors();
      }

      mPlainBufferSize += extraLen;
      SPDLOG_INFO("finalise the decryption, extraLen={}", extraLen);
    }
  }

  /// copy the rest of the content
  assert(mCurrentOffset + leftLen <= mPlainBufferSize);
  ::memmove(buffer + length - leftLen, mPlainBuffer.data() + mCurrentOffset, leftLen);
  mCurrentOffset += leftLen;
}

void CryptoUtil::commitDecryption(std::ifstream &ifs) {
  if (!mEnabled) {
    SPDLOG_WARN("AES is not enabled, will not commit decryption.");
    return;
  }

  /// Clean up
  EVP_CIPHER_CTX_free(mCtx);
}

}  /// namespace gringofts