#pragma once
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>

#include "type.hpp"

struct VariableSymbol {
    std::shared_ptr<Type> type;
    bool is_mutable;
};

struct FunctionSymbol {
    std::string name;
    std::shared_ptr<Type> return_type;
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> parameters;
};

struct StructSymbol {
    std::string name;
    TypeKind kind;
    std::unordered_map<std::string, std::shared_ptr<Type>> fields;
    std::vector<std::string> ordered_field_names;
};

struct EnumSymbol {
    std::string name;
    std::vector<std::string> variants;
};

class Environment {
private:
    std::vector<std::unordered_map<std::string, VariableSymbol>> scopes_;
    std::unordered_map<std::string, FunctionSymbol> functions_;
    std::unordered_map<std::string, StructSymbol> structs_;
    std::unordered_map<std::string, EnumSymbol> enums_;
    std::unordered_map<std::string, std::shared_ptr<Type>> databases_;
    std::unordered_map<std::string, std::shared_ptr<Type>> clients_;

public:
    Environment() { scopes_.emplace_back(); }

    void push_scope() { scopes_.emplace_back(); }
    void pop_scope() { if (scopes_.size() > 1) scopes_.pop_back(); }

    bool define_variable(const std::string& name, std::shared_ptr<Type> type, bool is_mutable) {
        if (scopes_.back().find(name) != scopes_.back().end()) return false;
        scopes_.back()[name] = {type, is_mutable};
        return true;
    }

    std::optional<VariableSymbol> lookup_variable(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return std::nullopt;
    }

    bool define_function(const FunctionSymbol& sym) {
        if (functions_.find(sym.name) != functions_.end()) return false;
        functions_[sym.name] = sym;
        return true;
    }

    std::optional<FunctionSymbol> lookup_function(const std::string& name) {
        auto found = functions_.find(name);
        if (found != functions_.end()) return found->second;
        return std::nullopt;
    }

    bool define_struct(const StructSymbol& sym) {
        if (structs_.find(sym.name) != structs_.end()) return false;
        structs_[sym.name] = sym;
        return true;
    }

    std::optional<StructSymbol> lookup_struct(const std::string& name) {
        auto found = structs_.find(name);
        if (found != structs_.end()) return found->second;
        return std::nullopt;
    }

    bool define_enum(const EnumSymbol& sym) {
        if (enums_.find(sym.name) != enums_.end()) return false;
        enums_[sym.name] = sym;
        return true;
    }

    std::optional<EnumSymbol> lookup_enum(const std::string& name) {
        auto found = enums_.find(name);
        if (found != enums_.end()) return found->second;
        return std::nullopt;
    }

    std::unordered_map<std::string, StructSymbol>& get_all_structs_mutable() {
        return structs_; 
    }

    void define_database(const std::string& name, std::shared_ptr<Type> type) { databases_[name] = type; }
    bool has_database(const std::string& name) { return databases_.find(name) != databases_.end(); }

    void define_client(const std::string& name, std::shared_ptr<Type> type) { clients_[name] = type; }
    bool has_client(const std::string& name) { return clients_.find(name) != clients_.end(); }

    const std::unordered_map<std::string, StructSymbol>& get_all_structs() const { return structs_; }
};