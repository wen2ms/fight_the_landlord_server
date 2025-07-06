#include "hash.h"

#include <cassert>

Hash::Hash(HashType type) : type_(type), ctx_(NULL) {
    ctx_ = EVP_MD_CTX_new();

    assert(ctx_ != NULL);

    int ret = EVP_DigestInit_ex(ctx_, kHashMethods.at(type)(), NULL);

    assert(ret == 1);
}

Hash::~Hash() {
    if (ctx_ != NULL) {
        EVP_MD_CTX_free(ctx_);
    }
}

void Hash::add_data(std::string data) {
    add_data(data.data(), data.length());
}

void Hash::add_data(const char* data, int length) {
    int ret = EVP_DigestUpdate(ctx_, data, length);

    assert(ret == 1);
}

std::string Hash::result(Type type) {
    unsigned int len = 0;
    unsigned char md[kHashLengths.at(type_)];
    int ret = EVP_DigestFinal_ex(ctx_, md, &len);

    assert(ret == 1);

    if (type == Type::kHex) {
        char res[len * 2];

        for (int i = 0; i < len; ++i) {
            sprintf(&res[i * 2], "%02x", md[i]);
        }

        return std::string(res, len * 2);
    }

    return {reinterpret_cast<char*>(md), len};
}