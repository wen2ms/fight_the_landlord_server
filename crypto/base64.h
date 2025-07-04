#ifndef BASE64_H
#define BASE64_H

#include <string>

class Base64 {
  public:
    explicit Base64() = default;
    
    std::string encode(std::string data);
    
    std::string encode(const char* data, int length);
    
    std::string decode(std::string data);
    
    std::string decode(const char* data, int length);
};

#endif  // BASE64_H
