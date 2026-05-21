#pragma once
#include <string>
#include <cstdint>

class RouteNode {
public:
    std::int32_t line_ = 0;
    std::int32_t column_ = 0;
    std::string http_method_;
    std::string path_;
    std::string handler_name_;
};

