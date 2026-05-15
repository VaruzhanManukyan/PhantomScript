#pragma once
#include <string>
#include <cstdint>

#include "declaration.hpp"

struct DatabaseDeclaration : IDeclaration {
    std::string name_;
    std::string engine_;
    std::string host_;
    std::int32_t port_;
    std::string db_name_;
};
