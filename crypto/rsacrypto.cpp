#include "rsacrypto.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <cassert>

#include "base64.h"

RsaCrypto::RsaCrypto() : pub_key_(NULL), pri_key_(NULL) {}

RsaCrypto::RsaCrypto(const std::string& filename, KeyType type) : RsaCrypto() {
    BIO* bio = BIO_new_file(filename.data(), "rb");

    assert(bio != NULL);

    if (type == kPublicKey) {
        PEM_read_bio_PUBKEY(bio, &pub_key_, NULL, NULL);
    } else {
        PEM_read_bio_PrivateKey(bio, &pri_key_, NULL, NULL);
    }

    BIO_free(bio);
}

RsaCrypto::~RsaCrypto() {
    if (pub_key_) {
        EVP_PKEY_free(pub_key_);
    }

    if (pri_key_) {
        EVP_PKEY_free(pri_key_);
    }
}

void RsaCrypto::prase_string_to_key(const std::string& data, KeyType type) {
    BIO* bio = BIO_new_mem_buf(data.data(), data.size());

    assert(bio != NULL);

    if (type == kPublicKey) {
        PEM_read_bio_PUBKEY(bio, &pub_key_, NULL, NULL);
    } else {
        PEM_read_bio_PrivateKey(bio, &pri_key_, NULL, NULL);
    }

    BIO_free(bio);
}

void RsaCrypto::generate_rsa_key(KeyLength bits, std::string pub, std::string pri) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    int ret = EVP_PKEY_keygen_init(ctx);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);

    assert(ret == 1);

    ret = EVP_PKEY_generate(ctx, &pri_key_);

    assert(ret == 1);

    EVP_PKEY_CTX_free(ctx);

    BIO* bio = BIO_new_file(pub.data(), "wb");

    ret = PEM_write_bio_PUBKEY(bio, pri_key_);

    assert(ret == 1);

    BIO_flush(bio);
    BIO_free(bio);

    bio = BIO_new_file(pri.data(), "wb");
    ret = PEM_write_bio_PrivateKey(bio, pri_key_, NULL, NULL, 0, NULL, NULL);

    assert(ret == 1);

    BIO_flush(bio);
    BIO_free(bio);
}

std::string RsaCrypto::pub_key_encrypt(std::string data) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pub_key_, NULL);

    assert(ctx != NULL);

    int ret = EVP_PKEY_encrypt_init(ctx);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    assert(ret == 1);

    size_t outlen = 0;

    ret = EVP_PKEY_encrypt(ctx, NULL, &outlen, reinterpret_cast<const unsigned char*>(data.data()), data.size());

    assert(ret == 1);

    auto out = new unsigned char[outlen];

    ret = EVP_PKEY_encrypt(ctx, out, &outlen, reinterpret_cast<const unsigned char*>(data.data()), data.size());

    assert(ret == 1);

    Base64 base64;
    std::string str = base64.encode(reinterpret_cast<char*>(out), outlen);

    delete[] out;
    EVP_PKEY_CTX_free(ctx);

    return str;
}

std::string RsaCrypto::pri_key_decrypt(std::string data) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pri_key_, NULL);

    assert(ctx != NULL);

    int ret = EVP_PKEY_decrypt_init(ctx);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    assert(ret == 1);

    Base64 base64;

    data = base64.decode(data);

    size_t out_len = 0;

    ret = EVP_PKEY_decrypt(ctx, NULL, &out_len, reinterpret_cast<const unsigned char*>(data.data()), data.size());

    assert(ret == 1);

    auto out = new unsigned char[out_len];

    ret = EVP_PKEY_decrypt(ctx, out, &out_len, reinterpret_cast<const unsigned char*>(data.data()), data.size());

    assert(ret == 1);

    std::string str(reinterpret_cast<char*>(out), out_len);

    delete[] out;
    EVP_PKEY_CTX_free(ctx);

    return str;
}

std::string RsaCrypto::sign(std::string data, HashType hash) {
    Hash hash_val(hash);

    hash_val.add_data(data);

    std::string md = hash_val.result();

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pri_key_, NULL);

    assert(ctx != NULL);

    int ret = EVP_PKEY_sign_init(ctx);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_signature_md(ctx, kHashMethods.at(hash)());

    assert(ret == 1);

    size_t outlen = 0;

    ret = EVP_PKEY_sign(ctx, NULL, &outlen, reinterpret_cast<const unsigned char*>(md.data()), md.size());

    assert(ret == 1);

    auto out = new unsigned char[outlen];

    ret = EVP_PKEY_sign(ctx, out, &outlen, reinterpret_cast<const unsigned char*>(md.data()), md.size());

    assert(ret == 1);

    Base64 base64;
    std::string str = base64.encode(reinterpret_cast<char*>(out), outlen);

    delete[] out;
    EVP_PKEY_CTX_free(ctx);

    return str;
}

bool RsaCrypto::verify(std::string sign, std::string data, HashType hash) {
    Base64 base64;

    sign = base64.decode(sign);

    Hash hash_val(hash);

    hash_val.add_data(data);

    std::string md = hash_val.result();

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pub_key_, NULL);

    assert(ctx != NULL);

    int ret = EVP_PKEY_verify_init(ctx);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);

    assert(ret == 1);

    ret = EVP_PKEY_CTX_set_signature_md(ctx, kHashMethods.at(hash)());

    assert(ret == 1);

    ret = EVP_PKEY_verify(ctx, reinterpret_cast<const unsigned char*>(sign.data()), sign.size(),
                          reinterpret_cast<const unsigned char*>(md.data()), md.size());

    EVP_PKEY_CTX_free(ctx);

    return ret == 1;
}
