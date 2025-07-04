#ifndef RSACRYPTO_H
#define RSACRYPTO_H

#include <openssl/evp.h>

#include <map>
#include <string>

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

    using hash_func = const EVP_MD* (*)(void);

    explicit RsaCrypto();

    explicit RsaCrypto(const std::string& filename, KeyType type);

    ~RsaCrypto();

    void prase_string_to_key(const std::string& data, KeyType type);

    void generate_rsa_key(KeyLength bits, std::string pub = "public.pem", std::string pri = "private.pem");

    std::string pub_key_encrypt(std::string data);

    std::string pri_key_decrypt(std::string data);

    std::string sign(std::string data, QCryptographicHash::Algorithm hash = QCryptographicHash::Sha256);

    bool verify(std::string sign, std::string data, QCryptographicHash::Algorithm hash = QCryptographicHash::Sha256);

  private:
    const std::map<QCryptographicHash::Algorithm, hash_func> hash_methods_ = {
        {QCryptographicHash::Md5, EVP_md5},           {QCryptographicHash::Sha1, EVP_sha1},
        {QCryptographicHash::Sha224, EVP_sha224},     {QCryptographicHash::Sha256, EVP_sha256},
        {QCryptographicHash::Sha384, EVP_sha384},     {QCryptographicHash::Sha512, EVP_sha512},
        {QCryptographicHash::Sha3_224, EVP_sha3_224}, {QCryptographicHash::Sha3_256, EVP_sha3_256},
        {QCryptographicHash::Sha3_384, EVP_sha3_384}, {QCryptographicHash::Sha3_512, EVP_sha3_512},
    };

    EVP_PKEY* pub_key_;
    EVP_PKEY* pri_key_;
};

#endif  // RSACRYPTO_H
