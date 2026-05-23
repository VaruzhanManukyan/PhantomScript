#include <iostream>
#include <algorithm>
#include <cctype>

#include "analyzer.hpp"
#include "../parser/nodes/declarations/program_node.hpp"
#include "../parser/nodes/declarations/import_declaration.hpp"
#include "../parser/nodes/declarations/function_declaration.hpp"
#include "../parser/nodes/declarations/struct_declaration.hpp"
#include "../parser/nodes/declarations/enum_declaration.hpp"
#include "../parser/nodes/declarations/data_transfer_object_declaration.hpp"
#include "../parser/nodes/declarations/query_declaration.hpp"
#include "../parser/nodes/declarations/client_declaration.hpp"
#include "../parser/nodes/declarations/consumer_declaration.hpp"
#include "../parser/nodes/declarations/service_declaration.hpp"
#include "../parser/nodes/declarations/database_declaration.hpp"
#include "../parser/nodes/statements/expression_statement.hpp"
#include "../parser/nodes/statements/variable_declaration_statement.hpp"
#include "../parser/nodes/statements/return_statement.hpp"
#include "../parser/nodes/statements/if_statement.hpp"
#include "../parser/nodes/statements/while_statement.hpp"
#include "../parser/nodes/statements/for_statement.hpp"
#include "../parser/nodes/statements/for_in_statement.hpp"
#include "../parser/nodes/statements/break_statement.hpp"
#include "../parser/nodes/statements/continue_statement.hpp"
#include "../parser/nodes/statements/match_statement.hpp"
#include "../parser/nodes/statements/print_statement.hpp"
#include "../parser/nodes/statements/block_statement.hpp"
#include "../parser/nodes/statements/publish_statement.hpp"
#include "../parser/nodes/expressions/assignment_expression.hpp"
#include "../parser/nodes/expressions/binary_expression.hpp"
#include "../parser/nodes/expressions/unary_expression.hpp"
#include "../parser/nodes/expressions/call_expression.hpp"
#include "../parser/nodes/expressions/member_access_expression.hpp"
#include "../parser/nodes/expressions/index_access_expression.hpp"
#include "../parser/nodes/expressions/cast_expression.hpp"
#include "../parser/nodes/expressions/identifier_expression.hpp"
#include "../parser/nodes/expressions/struct_instantiation_expression.hpp"
#include "../parser/nodes/expressions/array_literal_expression.hpp"

void SemanticAnalyzer::report_error(const std::string& message, std::int32_t line, std::int32_t column) {
    std::cerr << "SEMANTIC ERROR [" << line << ":" << column << "]: " << message << std::endl;

    errors_.emplace_back(message, line, column);
    throw SemanticException(message, line, column);
}
std::shared_ptr<Type> SemanticAnalyzer::resolve_type_node(TypeNode* node) {
    if (!node) return std::make_shared<Type>(Type{TypeKind::VOID, "void", {}});

    auto t = std::make_shared<Type>();
    t->name = node->base_name_;

    if (node->base_name_ == "int") t->kind = TypeKind::INT;
    else if (node->base_name_ == "double") t->kind = TypeKind::DOUBLE;
    else if (node->base_name_ == "string") t->kind = TypeKind::STRING;
    else if (node->base_name_ == "bool") t->kind = TypeKind::BOOL;
    else if (node->base_name_ == "void") t->kind = TypeKind::VOID;
    else if (node->base_name_ == "option") t->kind = TypeKind::OPTION;
    else if (node->base_name_ == "result") t->kind = TypeKind::RESULT;
    else if (node->base_name_ == "response") t->kind = TypeKind::RESPONSE;
    else if (node->base_name_ == "array") t->kind = TypeKind::ARRAY;
    else if (node->base_name_ == "list") t->kind = TypeKind::LIST;
    else if (node->base_name_ == "map") t->kind = TypeKind::MAP;
    else if (node->base_name_ == "set") t->kind = TypeKind::SET;
    else {
        if (env_.lookup_enum(node->base_name_)) {
            t->kind = TypeKind::ENUM;
        }
        else if (env_.has_database(node->base_name_)) {
            t->kind = TypeKind::DATABASE;
        }
        else {
            auto struct_sym = env_.lookup_struct(node->base_name_);
            if (struct_sym) {
                t->kind = struct_sym->kind;
            } else {
                report_error("Type resolution failure: unknown token or undefined structural schema context name '" + node->base_name_ + "'", node->line_, node->column_);
                t->kind = TypeKind::UNKNOWN;
            }
        }
    }

    for (auto& child : node->generics_) {
        t->generic_args.push_back(resolve_type_node(child.get()));
    }
    return t;
}

bool SemanticAnalyzer::analyze(ProgramNode& program) {
    errors_.clear();

    try {
        for (const auto& decl : program.declarations_) {
            if (auto* s = dynamic_cast<StructDeclaration*>(decl.get())) {
                env_.define_struct(StructSymbol{s->name_, TypeKind::STRUCT, {}, {}});
            } else if (auto* d = dynamic_cast<DataTransferObjectDeclaration*>(decl.get())) {
                TypeKind kind = d->is_event_ ? TypeKind::EVENT : TypeKind::CONTRACT;
                env_.define_struct(StructSymbol{d->name_, kind, {}, {}});
            } else if (auto* e = dynamic_cast<EnumDeclaration*>(decl.get())) {
                env_.define_enum(EnumSymbol{e->name_, e->variants_});
            } else if (auto* db = dynamic_cast<DatabaseDeclaration*>(decl.get())) {
                visit(*db);
            } else if (auto* c = dynamic_cast<ClientDeclaration*>(decl.get())) {
                visit(*c);
            }
            else if (auto* svc = dynamic_cast<ServiceDeclaration*>(decl.get())) {
                for (const auto& nested_db : svc->database_) {
                    visit(*nested_db);
                }
            }
        }

        for (const auto& decl : program.declarations_) {
            if (auto* s = dynamic_cast<StructDeclaration*>(decl.get())) {
                visit(*s);
            } else if (auto* d = dynamic_cast<DataTransferObjectDeclaration*>(decl.get())) {
                visit(*d);
            }
        }

        check_cyclic_dependencies();

        for (const auto& decl : program.declarations_) {
            if (auto* f = dynamic_cast<FunctionDeclaration*>(decl.get())) {
                FunctionSymbol sym;
                sym.name = f->name_;
                sym.return_type = resolve_type_node(f->return_type_.get());
                for (auto& p : f->parameters_) {
                    sym.parameters.push_back({p.name_, resolve_type_node(p.type_.get())});
                }
                env_.define_function(sym);
            } else if (auto* q = dynamic_cast<QueryDeclaration*>(decl.get())) {
                FunctionSymbol sym;
                sym.name = q->name_;
                sym.return_type = resolve_type_node(q->return_type_.get());
                for (auto& p : q->parameters_) {
                    sym.parameters.push_back({p.name_, resolve_type_node(p.type_.get())});
                }
                env_.define_function(sym);
            }
        }

        program.accept(*this);

    } catch (const SemanticException& ex) {
        return false;
    }

    return errors_.empty();
}

void SemanticAnalyzer::check_cyclic_dependencies() {
    std::unordered_map<std::string, std::vector<std::string>> graph;
    for (const auto& [name, def] : env_.get_all_structs()) {
        for (const auto& [f_name, f_type] : def.fields) {
            if (f_type->kind == TypeKind::STRUCT || f_type->kind == TypeKind::CONTRACT || f_type->kind == TypeKind::EVENT) {
                graph[name].push_back(f_type->name);
            }
        }
    }

    std::unordered_map<std::string, int> state;
    for (const auto& [node, _] : env_.get_all_structs()) {
        if (state[node] == 0) {
            if (dfs_cycle_check(node, state, graph)) {
                report_error("Cyclic dependency identified within compilation schema: structural loop containing '" + node + "'. Break the dependency using IDs.", 1, 1);
                return;
            }
        }
    }
}

bool SemanticAnalyzer::dfs_cycle_check(const std::string& node, std::unordered_map<std::string, int>& state, const std::unordered_map<std::string, std::vector<std::string>>& graph) {
    state[node] = 1;
    auto found = graph.find(node);
    if (found != graph.end()) {
        for (const auto& neighbor : found->second) {
            if (state[neighbor] == 1) return true;
            if (state[neighbor] == 0 && dfs_cycle_check(neighbor, state, graph)) return true;
        }
    }
    state[node] = 2;
    return false;
}

void SemanticAnalyzer::visit(ProgramNode& node) {
    for (const auto& decl : node.declarations_) {
        if (dynamic_cast<FunctionDeclaration*>(decl.get()) ||
            dynamic_cast<ServiceDeclaration*>(decl.get()) ||
            dynamic_cast<ConsumerDeclaration*>(decl.get()) ||
            dynamic_cast<QueryDeclaration*>(decl.get()) ||
            dynamic_cast<ImportDeclaration*>(decl.get())) {
            decl->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(ImportDeclaration& node) {
    if (node.path_.empty() || node.path_ == "\"\"") {
        report_error("Import compilation error: specified import path is empty.", node.line_, node.column_);
    }
}

void SemanticAnalyzer::visit(FunctionDeclaration& node) {
    env_.push_scope();
    current_function_return_type_ = resolve_type_node(node.return_type_.get());

    for (auto& param : node.parameters_) {
        auto p_type = resolve_type_node(param.type_.get());
        env_.define_variable(param.name_, p_type, false);
    }

    if (node.body_) {
        node.body_->accept(*this);
    }
    env_.pop_scope();
}

void SemanticAnalyzer::visit(StructDeclaration& node) {
    auto& structs = env_.get_all_structs_mutable();
    auto it = structs.find(node.name_);

    if (it != structs.end()) {
        for (auto& field : node.fields_) {
            it->second.fields[field.name_] = resolve_type_node(field.return_type_.get());
            it->second.ordered_field_names.push_back(field.name_);
        }
    }
}

void SemanticAnalyzer::visit(DataTransferObjectDeclaration& node) {
    auto& structs = env_.get_all_structs_mutable();
    auto it = structs.find(node.name_);

    if (it != structs.end()) {
        for (auto& field : node.fields_) {
            it->second.fields[field.name_] = resolve_type_node(field.return_type_.get());
            it->second.ordered_field_names.push_back(field.name_);
        }
    }
}

void SemanticAnalyzer::visit(EnumDeclaration& node) {
    EnumSymbol sym{node.name_, node.variants_};
    env_.define_enum(sym);
}

void SemanticAnalyzer::visit(DatabaseDeclaration& node) {
    auto db_type = std::make_shared<Type>(Type{TypeKind::DATABASE, node.name_, {}});
    env_.define_database(node.name_, db_type);
}

void SemanticAnalyzer::visit(QueryDeclaration& node) {
    for (auto& param : node.parameters_) {
        auto p_type = resolve_type_node(param.type_.get());
        if (p_type->kind == TypeKind::STRUCT || p_type->kind == TypeKind::CONTRACT) {
            if (!env_.lookup_struct(p_type->name)) {
                report_error("Query '" + node.name_ + "' parameter '" + param.name_ + "' uses an unresolved type: " + p_type->name, param.type_->line_, param.type_->column_);
            }
        }
    }

    auto ret_type = resolve_type_node(node.return_type_.get());
    if (ret_type->kind == TypeKind::OPTION || ret_type->kind == TypeKind::RESULT) {
        for (auto& generic_arg : ret_type->generic_args) {
            if (generic_arg->kind == TypeKind::STRUCT && !env_.lookup_struct(generic_arg->name)) {
                report_error("Query '" + node.name_ + "' returns unresolved schema: " + generic_arg->name, node.return_type_->line_, node.return_type_->column_);
            }
        }
    }

    std::string sql = node.sql_query_;
    std::size_t pos = 0;
    while ((pos = sql.find('$', pos)) != std::string::npos) {
        std::size_t end_pos = pos + 1;
        while (end_pos < sql.size() && (std::isalnum(sql[end_pos]) || sql[end_pos] == '_')) end_pos++;
        std::string var_name = sql.substr(pos + 1, end_pos - (pos + 1));
        if (!var_name.empty()) {
            bool param_exists = false;
            for (const auto& param : node.parameters_) {
                if (param.name_ == var_name) { param_exists = true; break; }
            }
            if (!param_exists) {
                report_error("SQL Variable compilation error: $" + var_name + " does not exist in query parameters.", node.line_, node.column_);
            }
        }
        pos = end_pos;
    }
}

void SemanticAnalyzer::visit(ClientDeclaration& node) {
    auto client_type = std::make_shared<Type>(Type{TypeKind::CLIENT, node.name_, {}});
    env_.define_client(node.name_, client_type);

    for (auto& req : node.requests_) {
        FunctionSymbol sym;
        sym.name = node.name_ + "." + req.local_function_name_;
        sym.return_type = resolve_type_node(req.return_type_.get());
        for (auto& p : req.parameters_) {
            sym.parameters.push_back({p.name_, resolve_type_node(p.type_.get())});
        }
        env_.define_function(sym);
    }
}

void SemanticAnalyzer::visit(ConsumerDeclaration& node) {
    auto ev = env_.lookup_struct(node.event_name_);
    if (!ev || ev->kind != TypeKind::EVENT) {
        report_error("Consumer target binds to an unmapped event block: " + node.event_name_, node.line_, node.column_);
    }
    auto handler = env_.lookup_function(node.handler_function_name_);
    if (!handler) {
        report_error("Consumer handler maps to an unresolved function signature: " + node.handler_function_name_, node.line_, node.column_);
    }
}

void SemanticAnalyzer::visit(ServiceDeclaration& node) {
    auto check_route = [&](const std::string& handler_name) {
        auto handler = env_.lookup_function(handler_name);
        if (!handler) {
            report_error("Service microservice route maps to an unresolved functional endpoint handler: " + handler_name, node.line_, node.column_);
        }
    };

    for (const auto& route : node.flat_routes_) {
        check_route(route->handler_name_);
    }
    for (const auto& group : node.route_groups_) {
        for (const auto& route : group->routes_) {
            check_route(route->handler_name_);
        }
    }
    for (const auto& event_name : node.published_events_) {
        auto ev = env_.lookup_struct(event_name);
        if (!ev || ev->kind != TypeKind::EVENT) {
            report_error("Service declaration states event publishing broadcast for an unresolved event node structure: " + event_name, node.line_, node.column_);
        }
    }
}

void SemanticAnalyzer::visit(ExpressionStatement& node) {
    node.expression_->accept(*this);
}

void SemanticAnalyzer::visit(VariableDeclarationStatement& node) {
    auto target_type = resolve_type_node(node.type_.get());
    if (node.initializer_) {
        node.initializer_->accept(*this);
        if (*target_type != *current_expression_type_ && current_expression_type_->kind != TypeKind::UNKNOWN) {
            report_error("Compilation Type Error: cannot bind expression type '" +
                         current_expression_type_->to_string() + "' to variable allocation '" +
                         node.name_ + "' matching target specification '" + target_type->to_string() + "'", node.line_, node.column_);
        }
    }
    if (!env_.define_variable(node.name_, target_type, node.is_mut_)) {
        report_error("Redefinition Error: structural variable allocation naming collision with identifier '" + node.name_ + "'", node.line_, node.column_);
    }
}

void SemanticAnalyzer::visit(ReturnStatement& node) {
    if (node.expression_) {
        node.expression_->accept(*this);
        if (*current_expression_type_ != *current_function_return_type_ && current_expression_type_->kind != TypeKind::UNKNOWN) {
            report_error("Return type mismatch: function expected output matching structure '" +
                         current_function_return_type_->to_string() + "' but evaluation produced '" +
                         current_expression_type_->to_string() + "'", node.line_, node.column_);
        }
    } else {
        if (current_function_return_type_->kind != TypeKind::VOID) {
            report_error("Non-void functional definition execution block requires a strict output return expression statement evaluation.", node.line_, node.column_);
        }
    }
}

void SemanticAnalyzer::visit(IfStatement& node) {
    if (node.expression_) {
        node.expression_->accept(*this);
        if (current_expression_type_->kind != TypeKind::BOOL) {
            report_error("Branch logic conditional expressions must resolve to a valid boolean evaluation primitive.", node.line_, node.column_);
        }
    }
    if (node.then_branch_) node.then_branch_->accept(*this);
    if (node.else_branch_) node.else_branch_->accept(*this);
}

void SemanticAnalyzer::visit(WhileStatement& node) {
    if (node.condition_) node.condition_->accept(*this);

    loop_depth_++;
    if (node.body_) node.body_->accept(*this);
    loop_depth_--;
}

void SemanticAnalyzer::visit(ForStatement& node) {
    if (node.init_) node.init_->accept(*this);
    if (node.condition_) node.condition_->accept(*this);
    if (node.increment_) node.increment_->accept(*this);

    loop_depth_++;
    if (node.body_) node.body_->accept(*this);
    loop_depth_--;
}

void SemanticAnalyzer::visit(ForInStatement& node) {
    if (node.iterable_) node.iterable_->accept(*this);

    loop_depth_++;
    if (node.body_) node.body_->accept(*this);
    loop_depth_--;
}

void SemanticAnalyzer::visit(BreakStatement& node) {
    if (loop_depth_ <= 0) {
        report_error("Semantic Error: 'break' statement is not allowed outside of a loop block", node.line_, node.column_);
    }
}

void SemanticAnalyzer::visit(ContinueStatement& node) {
    if (loop_depth_ <= 0) {
        report_error("Semantic Error: 'continue' statement is not allowed outside of a loop block", node.line_, node.column_);
    }
}
void SemanticAnalyzer::visit(MatchStatement& node) {
    node.expression_->accept(*this);
    auto subject_type = current_expression_type_;

    for (auto& c : node.match_cases_) {
        env_.push_scope();
        if (subject_type->kind == TypeKind::OPTION) {
            if (c.pattern_name_ == "Some" && c.bound_variable_ && !subject_type->generic_args.empty()) {
                env_.define_variable(*c.bound_variable_, subject_type->generic_args[0], false);
            }
        } else if (subject_type->kind == TypeKind::RESULT) {
            if (c.pattern_name_ == "Ok" && c.bound_variable_ && !subject_type->generic_args.empty()) {
                env_.define_variable(*c.bound_variable_, subject_type->generic_args[0], false);
            } else if (c.pattern_name_ == "Err" && c.bound_variable_ && subject_type->generic_args.size() > 1) {
                env_.define_variable(*c.bound_variable_, subject_type->generic_args[1], false);
            }
        }
        if (c.body_) c.body_->accept(*this);
        env_.pop_scope();
    }
}

void SemanticAnalyzer::visit(PrintStatement& node) {
    if (node.expression_) {
        node.expression_->accept(*this);
    }
}

void SemanticAnalyzer::visit(BlockStatement& node) {
    env_.push_scope();
    for (const auto& stmt : node.statements_) {
        stmt->accept(*this);
    }
    env_.pop_scope();
}

void SemanticAnalyzer::visit(PublishStatement& node) {
    auto ev = env_.lookup_struct(node.event_name_);
    if (!ev || ev->kind != TypeKind::EVENT) {
        report_error("Publish broker compilation error: event structural target layout schema unresolved for definition: " + node.event_name_, node.line_, node.column_);
        return;
    }

    for (auto& f : node.fields_) {
        f.value_->accept(*this);
        auto it = ev->fields.find(f.name_);
        if (it == ev->fields.end()) {
            report_error("Broker schema field injection layout violation: event definition '" + node.event_name_ + "' contains no member naming '" + f.name_ + "'", node.line_, node.column_);
            continue;
        }
        if (*current_expression_type_ != *it->second) {
            report_error("Type failure inside message queue field payload initialization for member '" + f.name_ + "'", node.line_, node.column_);
        }
    }
}

void SemanticAnalyzer::visit(AssignmentExpression& node) {
    if (node.target_) {
        if (auto* ident = dynamic_cast<IdentifierExpression*>(node.target_.get())) {
            auto var = env_.lookup_variable(ident->name_);
            if (var && !var->is_mutable) {
                report_error("Immutability Protection Rule Violation: cannot execute mutation assignment sequence on constant value '" + ident->name_ + "'", node.line_, node.column_);
            }
        }
        node.target_->accept(*this);
    }
    auto lhs = current_expression_type_;

    if (node.value_) {
        node.value_->accept(*this);
    }
    auto rhs = current_expression_type_;

    if (*lhs != *rhs && lhs->kind != TypeKind::UNKNOWN) {
        report_error("Incompatible type boundaries matched across mutation assignment operation: '" + lhs->to_string() + "' vs '" + rhs->to_string() + "'", node.line_, node.column_);
    }
}

void SemanticAnalyzer::visit(BinaryExpression& node) {
    node.left_->accept(*this);
    auto lhs = current_expression_type_;
    node.right_->accept(*this);
    auto rhs = current_expression_type_;

    if (*lhs != *rhs && lhs->kind != TypeKind::UNKNOWN && rhs->kind != TypeKind::UNKNOWN) {
        report_error("Binary operator execution failure: type schemas do not match.", node.line_, node.column_);
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
        return;
    }

    if (node.operator_ == "==" || node.operator_ == "!=" || node.operator_ == "<" || node.operator_ == ">" || node.operator_ == "<=" || node.operator_ == ">=") {
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::BOOL, "bool", {}});
    } else {
        current_expression_type_ = lhs;
    }
}

void SemanticAnalyzer::visit(UnaryExpression& node) {
    node.right_->accept(*this);
}

void SemanticAnalyzer::visit(CallExpression& node) {
    if (auto* ident = dynamic_cast<IdentifierExpression*>(node.callee_.get())) {

        if (ident->name_ == "ok" || ident->name_ == "err") {
            if (node.arguments_.size() != 1) {
                report_error("Monadic compiler constructor expects exactly 1 expression argument.", node.line_, node.column_);
            }
            node.arguments_[0]->accept(*this);

            auto res_type = std::make_shared<Type>();
            res_type->kind = TypeKind::RESULT;
            res_type->name = "result";

            if (current_function_return_type_ && current_function_return_type_->kind == TypeKind::RESULT && current_function_return_type_->generic_args.size() == 2) {
                res_type->generic_args = current_function_return_type_->generic_args;
            } else {
                res_type->generic_args.push_back(current_expression_type_);
                res_type->generic_args.push_back(std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}}));
            }
            current_expression_type_ = res_type;
            return;
        }

        auto func = env_.lookup_function(ident->name_);
        if (!func) {
            report_error("Call evaluation target resolves to an unmapped signature symbol allocation: " + ident->name_, node.line_, node.column_);
            current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
            return;
        }
        current_expression_type_ = func->return_type;
    } else if (auto* mem = dynamic_cast<MemberAccessExpression*>(node.callee_.get())) {
        mem->object_->accept(*this);
        auto obj_t = current_expression_type_;

        if (obj_t->kind == TypeKind::DATABASE || obj_t->kind == TypeKind::CLIENT) {
            auto rpc = env_.lookup_function(obj_t->name + "." + mem->member_name_);
            if (rpc) {
                current_expression_type_ = rpc->return_type;
                return;
            }
        }
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
    }
}

void SemanticAnalyzer::visit(MemberAccessExpression& node) {
    node.object_->accept(*this);
    auto obj_type = current_expression_type_;

    if (obj_type->kind == TypeKind::ARRAY ||
        obj_type->kind == TypeKind::LIST  ||
        obj_type->kind == TypeKind::SET   ||
        obj_type->kind == TypeKind::MAP) {
        if (node.member_name_ == "length") {
            current_expression_type_ = std::make_shared<Type>(Type{TypeKind::INT, "int", {}});
            return;
        }
        }

    if (obj_type->kind == TypeKind::STRUCT || obj_type->kind == TypeKind::CONTRACT || obj_type->kind == TypeKind::EVENT) {
        auto struct_sym = env_.lookup_struct(obj_type->name);
        if (struct_sym) {
            auto field_it = struct_sym->fields.find(node.member_name_);
            if (field_it != struct_sym->fields.end()) {
                current_expression_type_ = field_it->second;
                return;
            }
        }
    }

    report_error("Member selection evaluation context resolution failure: member identifier '" + node.member_name_ + "' is unmapped inside the target layout configuration.", node.line_, node.column_);
}

void SemanticAnalyzer::visit(IndexAccessExpression& node) {
    node.object_->accept(*this);
    auto obj_type = current_expression_type_;

    if (obj_type->kind != TypeKind::ARRAY &&
        obj_type->kind != TypeKind::LIST  &&
        obj_type->kind != TypeKind::MAP) {
        report_error(
            "Index access requires array, list, or map type, got '" +
            obj_type->to_string() + "'.",
            node.line_, node.column_);
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
        return;
        }

    node.index_->accept(*this);

    if (obj_type->kind == TypeKind::MAP && obj_type->generic_args.size() >= 2) {
        current_expression_type_ = obj_type->generic_args[1];
    } else if (!obj_type->generic_args.empty()) {
        current_expression_type_ = obj_type->generic_args[0];
    } else {
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
    }
}

void SemanticAnalyzer::visit(CastExpression& node) {
    if (node.value_) {
        node.value_->accept(*this);
    }
    current_expression_type_ = resolve_type_node(node.type_.get());
}

void SemanticAnalyzer::visit(BoolLiteralExpression& node) {
    current_expression_type_ = std::make_shared<Type>(Type{TypeKind::BOOL, "bool", {}});
}

void SemanticAnalyzer::visit(NullLiteralExpression& node) {
    current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "null", {}});
}

void SemanticAnalyzer::visit(IntLIteralExpression& node) {
    current_expression_type_ = std::make_shared<Type>(Type{TypeKind::INT, "int", {}});
}

void SemanticAnalyzer::visit(DoubleLiteralExpression& node) {
    current_expression_type_ = std::make_shared<Type>(Type{TypeKind::DOUBLE, "double", {}});
}

void SemanticAnalyzer::visit(StringLiteralExpression& node) {
    current_expression_type_ = std::make_shared<Type>(Type{TypeKind::STRING, "string", {}});
}

void SemanticAnalyzer::visit(ArrayLiteralExpression& node) {
    std::shared_ptr<Type> elem_type = nullptr;

    for (auto& elem : node.elements_) {
        elem->accept(*this);
        if (!elem_type) {
            elem_type = current_expression_type_;
        } else if (*current_expression_type_ != *elem_type) {
            report_error(
                "Array literal contains mixed types: expected '" +
                elem_type->to_string() + "' but got '" +
                current_expression_type_->to_string() + "'.",
                node.line_, node.column_);
        }
    }

    auto arr_type = std::make_shared<Type>();
    arr_type->kind = TypeKind::ARRAY;
    arr_type->name = "array";
    if (elem_type) {
        arr_type->generic_args.push_back(elem_type);
    }
    current_expression_type_ = arr_type;
}

void SemanticAnalyzer::visit(IdentifierExpression& node) {
    auto var = env_.lookup_variable(node.name_);
    if (var) {
        current_expression_type_ = var->type;
        return;
    }
    if (env_.has_database(node.name_)) {
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::DATABASE, node.name_, {}});
        return;
    }
    if (env_.has_client(node.name_)) {
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::CLIENT, node.name_, {}});
        return;
    }
    report_error("Undeclared variable context resolution failure: object identifier symbol '" + node.name_ + "' is out of scope compilation limits.", node.line_, node.column_);
    current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
}

void SemanticAnalyzer::visit(StructInstantiationExpression& node) {
    auto struct_sym = env_.lookup_struct(node.struct_name_);
    if (!struct_sym) {
        report_error("Attempt to instantiate an unresolved structural schema layout: " + node.struct_name_, node.line_, node.column_);
        current_expression_type_ = std::make_shared<Type>(Type{TypeKind::UNKNOWN, "unknown", {}});
        return;
    }

    for (auto& f : node.fields_) {
        f.value_->accept(*this);
        auto field_schema_it = struct_sym->fields.find(f.name_);
        if (field_schema_it == struct_sym->fields.end()) {
            report_error("Field injection layout mismatch: structure '" + node.struct_name_ + "' has no definition matching field named '" + f.name_ + "'", node.line_, node.column_);
            continue;
        }
        if (*current_expression_type_ != *field_schema_it->second && current_expression_type_->kind != TypeKind::UNKNOWN) {
            report_error("Structure initialization type error: field '" + f.name_ + "' expected '" + field_schema_it->second->to_string() + "' but received '" + current_expression_type_->to_string() + "'", node.line_, node.column_);
        }
    }

    auto t = std::make_shared<Type>();
    t->name = node.struct_name_;
    t->kind = struct_sym->kind;
    current_expression_type_ = t;
}