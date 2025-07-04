#include "base64.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

std::string Base64::encode(std::string data) {
    return encode(data.data(), data.size());
}

std::string Base64::encode(const char *data, int length) {
    BIO* base64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    
    BIO_push(base64, mem);
    
    BIO_write(base64, data, length);
    BIO_flush(base64);
    
    BUF_MEM* ptr;
    
    BIO_get_mem_ptr(base64, &ptr);
    
    std::string str(ptr->data, ptr->length);
    
    BIO_free_all(base64);
    
    return str;
}

std::string Base64::decode(std::string data) {
    return decode(data.data(), data.size());
}

std::string Base64::decode(const char *data, int length) {
    BIO* base64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    
    BIO_push(base64, mem);
    
    BIO_write(mem, data, length);
    
    char* buf = new char[length];
    int buf_len = BIO_read(base64, buf, length);
    std::string out(buf, buf_len);
    
    delete[] buf;
    BIO_free_all(base64);
    
    return out;
}
