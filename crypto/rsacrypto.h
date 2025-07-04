#ifndef RSACRYPTO_H
#define RSACRYPTO_H

#include <openssl/evp.h>

#include <string>

#include "hash.h"

class RsaCrypto {
  public:
    enum KeyLength {
        kBits1k = 1024,
        kBits2k = 2048,
        kBits3k = 3072,
        kBits4k = 4096,
    };

    enum KeyType {
        kPublicKey,
        kPrivateKey
    };

    explicit RsaCrypto();

    explicit RsaCrypto(const std::string& filename, KeyType type);

    ~RsaCrypto();

    void prase_string_to_key(const std::string& data, KeyType type);

    void generate_rsa_key(KeyLength bits, std::string pub = "public.pem", std::string pri = "private.pem");

    std::string pub_key_encrypt(std::string data);

    std::string pri_key_decrypt(std::string data);

    std::string sign(std::string data, HashType hash = HashType::kSha256);

    bool verify(std::string sign, std::string data, HashType hash = HashType::kSha256);

  private:
    EVP_PKEY* pub_key_;
    EVP_PKEY* pri_key_;
};

#endif  // RSACRYPTO_H
