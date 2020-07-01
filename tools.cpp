#include "tools.h"



std::string uuid() {
    char buf[64] = { 0 };
    uuid_t uu;
    uuid_generate(uu);
    uuid_unparse(uu, buf);
    return std::move(std::string(buf));
}

std::string randomID() {
    auto id = uuid();
    std::string rv;
    for (auto c : id) {
        if (c >= '0' && c <= '9' || c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z') {
            rv.push_back(c);
        }
    }
    return std::move(rv);
}