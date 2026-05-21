#pragma once
#include "node_metadata.hpp"

class IASTNode : public NodeMetadata {
public:
    explicit IASTNode(std::int32_t line, std::int32_t column)
        : NodeMetadata(line, column) {}

    virtual ~IASTNode() = default;
};