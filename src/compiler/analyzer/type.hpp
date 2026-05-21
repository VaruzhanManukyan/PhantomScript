#pragma once
#include <memory>
#include <string>
#include <vector>

#include "enums/type_kind.hpp"

struct Type {
    TypeKind kind;
    std::string name;
    std::vector<std::shared_ptr<Type>> generic_args;

    bool operator==(const Type& other) const {
        if (kind != other.kind || name != other.name ||
            generic_args.size() != other.generic_args.size()) {
            return false;
        }

        for (std::size_t i = 0; i < generic_args.size(); ++i) {
            if (*generic_args[i] != *other.generic_args[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Type& other) const {
        return !(*this == other);
    }

    std::string to_string() const {
        if (generic_args.empty()) return name;
        std::string result = name + "[";
        for (std::size_t i = 0; i < generic_args.size(); ++i) {
            result += generic_args[i]->to_string();
            if (i + 1 < generic_args.size()) {
                result += ", ";
            }
        }
        result += "]";
        return result;
    }
};
