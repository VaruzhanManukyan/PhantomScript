#pragma once
#include <string>

struct RouteNode {
    std::string http_method;
    std::string path_;
    std::string handler_name_;
};

