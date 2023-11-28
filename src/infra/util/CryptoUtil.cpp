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

#include <vector>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

#include "FileUtil.h"
#include "TimeUtil.h"

namespace gringofts {

void CryptoUtil::init(const INIReader &reader) {
  init(reader, SecretKeyFactoryDefault());
}

void CryptoUtil::init(const INIReader &reader, const SecretKeyFactoryInterface &secretFactory) {
  if (mEnabled) {
    SPDLOG_ERROR("Set AES key twice.");
    exit(1);
  }

  bool enableCrypt = reader.GetBoolean("aes", "enable", true);
  std::string keyFileName = reader.Get("aes", "filename", "");
  // This code snippet is to be compatible with the old config file.
  // Actually, CryptoUtil shouldn't be awareness of the existence of microvault.
  // TODO(jingyichen): remove this code snippet after all the config files are updated.
  std::string keyPathOnMicrovault = reader.Get("aes", "microvault.key_path", "");

  // Three cases and actions:
  // 1. Both 'enable' and 'filename' are not set:
  //     Ignore encrypt and do nothing.
  // 2. Both 'enable' and 'filename' are set:
  //     If 'enable' is true and filename is legal, encrypt.
  // 3. Set 'filename' and ignore 'enable':
  //     Encrypt.
  if (!enableCrypt || (keyFileName.empty() && keyPathOnMicrovault.empty())) {
    mEnabled = false;
    SPDLOG_WARN("Raft log is plain.");
    return;
  }

  mSecKeys = secretFactory.create(reader);

  mEnabled = true;
  SPDLOG_INFO("Raft log and snapshot will be encrypted.");
}

int CryptoUtil::encrypt(std::string *payload, SecKeyVersion version) const {
  if (!mEnabled) {
    return 0;
  }

  std::vector<unsigned char> buffer;
  buffer.resize(payload->size() + AES_BLOCK_SIZE);

  auto cipherLen = 0;
  auto res = encrypt(reinterpret_cast<const unsigned char *>(payload->c_str()),
      payload->size(), mSecKeys->getKeyByVersion(version), mIV, &buffer[0], &cipherLen);
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

  std::vector<unsigned char> buffer;
  buffer.resize(payload->size());

  auto plainLen = 0;
  auto res = decrypt(reinterpret_cast<const unsigned char *>(payload->c_str()),
      payload->size(), mSecKeys->getKeyByVersion(version), mIV, &buffer[0], &plainLen);
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

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int len = 0;

  /// Using sha256 hash engine here.
  /// You may use other hash engines. e.g., EVP_md5(), EVP_sha224(), EVP_sha512(), etc
  auto *ptr = HMAC(EVP_sha256(), mSecKeys->getKeyByVersion(version), SecretKey::kKeyLen, d, n, digest, &len);

  assert(ptr != nullptr);
  assert(len == 32);  /// output length of SHA256 should be 32 bytes (a.k.a. 256 bits).

  return std::string(reinterpret_cast<const char *>(digest), len);
}

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

}  /// namespace gringofts
