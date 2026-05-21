#pragma once
#include <cstdint>

class NodeMetadata {
public:
    std::int32_t line_ = 0;
    std::int32_t column_ = 0;
    
    explicit NodeMetadata(std::int32_t line, std::int32_t column) :
        line_(line), column_(column) {};

    virtual ~NodeMetadata() = default;
};