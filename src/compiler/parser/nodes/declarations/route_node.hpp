#pragma once
#include <string>

struct RouteNode {
    std::string http_method_;
    std::string path_;
    std::string handler_name_;
};

