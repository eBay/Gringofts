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

#include <string>

#ifndef SRC_INFRA_ES_CRYPTO_H_
#define SRC_INFRA_ES_CRYPTO_H_

namespace gringofts {

/**
 * An interface defining the syntax for encrypting and decrypting large files.
 */
class Crypto {
 public:
  virtual ~Crypto() = default;

  /**
   * Start encrypting file
   * Sample usage:
   * ```
   * beginEncryption(bufferSize);
   * encryptUint64ToFile(ofs, content);
   * ...
   * commitEncryption(ofs);
   * ```
   * @param bufferSize the size of buffer that will hold the raw data
   */
  virtual void beginEncryption(uint64_t bufferSize) = 0;
  /**
   * Encrypt an uint64 to file
   * @param ofs the output stream of the encrypted file
   * @param content the uint64 value
   */
  virtual void encryptUint64ToFile(std::ofstream &ofs, uint64_t content) = 0;
  /**
   * Encrypt a string to file
   * @param ofs the output stream of the encrypted file
   * @param content the string value
   */
  virtual void encryptStrToFile(std::ofstream &ofs, const std::string &content) = 0;
  /**
   * Signal that all raw data has been sent for encryption
   * @param ofs the output stream of the encrypted file
   */
  virtual void commitEncryption(std::ofstream &ofs) = 0;

  /**
   * Start decrypting file
   * Sample usage:
   * ```
   * beginDecryption(bufferSize);
   * decryptUint64FromFile(ifs, content);
   * ...
   * commitDecryption(ifs);
   * ```
   * @param bufferSize the size of buffer that will hold the raw data
   */
  virtual void beginDecryption(uint64_t bufferSize) = 0;
  /**
   * Decrypt an uint64 from file
   * @param ifs the input stream of the decrypted file
   * @returns an uint64 value
   */
  virtual uint64_t decryptUint64FromFile(std::ifstream &ifs) = 0;
  /**
   * Decrypt an uint64 from file
   * @param ifs the input stream of the decrypted file
   * @returns a string value
   */
  virtual std::string decryptStrFromFile(std::ifstream &ifs) = 0;
  /**
   * Signal that all raw data has been read for decryption
   * @param ifs the input stream of the decrypted file
   */
  virtual void commitDecryption(std::ifstream &ifs) = 0;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_ES_CRYPTO_H_
