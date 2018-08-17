#pragma once

#include <string>
#include <vector>

namespace util {
    // Environment manipulation
    std::string getEnvVar(std::string const &key, std::string const &deflt);
    void setEnvVar(const std::string &varName, const std::string &value);
    void unsetEnvVar(const std::string &varName);

    // Byte handling
    std::vector<uint8_t> stringToBytes(const std::string &str);
    void trimTrailingZeros(std::vector<uint8_t> &vectorIn);

    // Files
    std::string readFileToString(const std::string &path);

    // Misc
    int randomInteger();
}
