#ifndef HASH_H
#define HASH_H

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <map>
#include <string>

enum class HashType : char {
    kMd5,
    kSha1,
    kSha224,
    kSha256,
    kSha384,
    kSha512,
    kSha3_224,
    kSha3_256,
    kSha3_384,
    kSha3_512,
};

using hash_func = const EVP_MD* (*)();

const std::map<HashType, hash_func> kHashMethods = {
    {HashType::kMd5, EVP_md5},           {HashType::kSha1, EVP_sha1},         {HashType::kSha224, EVP_sha224},
    {HashType::kSha256, EVP_sha256},     {HashType::kSha384, EVP_sha384},     {HashType::kSha512, EVP_sha512},
    {HashType::kSha3_224, EVP_sha3_224}, {HashType::kSha3_256, EVP_sha3_256}, {HashType::kSha3_384, EVP_sha3_384},
    {HashType::kSha3_512, EVP_sha3_512},
};

const std::map<HashType, int> kHashLengths = {
    {HashType::kMd5, MD5_DIGEST_LENGTH},         {HashType::kSha1, SHA_DIGEST_LENGTH},
    {HashType::kSha224, SHA224_DIGEST_LENGTH},   {HashType::kSha256, SHA256_DIGEST_LENGTH},
    {HashType::kSha384, SHA384_DIGEST_LENGTH},   {HashType::kSha512, SHA512_DIGEST_LENGTH},
    {HashType::kSha3_224, SHA224_DIGEST_LENGTH}, {HashType::kSha3_256, SHA256_DIGEST_LENGTH},
    {HashType::kSha3_384, SHA384_DIGEST_LENGTH}, {HashType::kSha3_512, SHA512_DIGEST_LENGTH},
};

class Hash {
  public:
    enum class Type : char {
        kBinary,
        kHex
    };

    explicit Hash(HashType type);
    ~Hash();

    void add_data(std::string data);
    void add_data(const char* data, int length);

    std::string result(Type type = Type::kHex);

  private:
    EVP_MD_CTX* ctx_;

    HashType type_;
};

#endif  // HASH_H
