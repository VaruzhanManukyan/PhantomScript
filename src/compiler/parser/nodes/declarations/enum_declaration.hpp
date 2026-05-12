#pragma once
#include <string>
#include <vector>

#include "declaration.hpp"

struct EnumDeclaration : IDeclaration {
    std::string name_;
    std::vector<std::string> variants_;
};