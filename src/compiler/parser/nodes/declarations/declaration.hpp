#pragma once
#include "../statements/statement.hpp"

struct IDeclaration : IStatement {
    virtual ~IDeclaration() = default;
};