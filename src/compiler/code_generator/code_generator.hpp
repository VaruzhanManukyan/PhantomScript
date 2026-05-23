#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <set>
#include <optional>
#include <unordered_map>

#include "../analyzer/ast_visitor.hpp"

class ProgramNode;
class TypeNode;

struct CompilationArtifacts {
    std::string hpp_code;
    std::string cpp_code;
    std::optional<std::string> dockerfile;
    std::optional<std::string> docker_compose;
    std::optional<std::string> cmake_lists;
};

class CodeGenerator : public IAstVisitor {
private:
    std::stringstream hpp_stream_;
    std::stringstream cpp_stream_;
    std::stringstream consumer_stream_;
    std::set<std::string> required_headers_;
    std::unordered_map<std::string, std::vector<std::string>> function_param_types_;
    std::unordered_map<std::string, std::vector<std::string>> function_param_names_;
    std::vector<DatabaseDeclaration*> all_parsed_databases_;

    bool http_runtime_generated_ = false;
    std::vector<std::string> service_names_;
    int indent_level_ = 0;

    bool uses_postgres_ = false;
    bool uses_rabbitmq_ = false;
    bool uses_service_ = false;

    std::string indent() const;
    std::string translate_type(TypeNode* type_node);

    std::string default_cpp_value(const std::string& cpp_type) const;
    std::string build_default_args(const std::string& fn_name) const;

    std::string generate_dockerfile() const;
    std::string generate_docker_compose() const;
    std::string generate_cmake() const;
    void generate_http_runtime();

public:
    CodeGenerator() = default;
    virtual ~CodeGenerator() = default;

    CompilationArtifacts generate(ProgramNode& program, const std::string& source_stem = "output");

    void visit(ProgramNode& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(EnumDeclaration& node) override;
    void visit(DataTransferObjectDeclaration& node) override;
    void visit(QueryDeclaration& node) override;
    void visit(ClientDeclaration& node) override;
    void visit(ConsumerDeclaration& node) override;
    void visit(ServiceDeclaration& node) override;
    void visit(DatabaseDeclaration& node) override;
    void visit(ImportDeclaration& node) override;

    void visit(ExpressionStatement& node) override;
    void visit(VariableDeclarationStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ForInStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(MatchStatement& node) override;
    void visit(PrintStatement& node) override;
    void visit(BlockStatement& node) override;
    void visit(PublishStatement& node) override;

    void visit(AssignmentExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(IndexAccessExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(BoolLiteralExpression& node) override;
    void visit(NullLiteralExpression& node) override;
    void visit(IntLIteralExpression& node) override;
    void visit(DoubleLiteralExpression& node) override;
    void visit(StringLiteralExpression& node) override;
    void visit(ArrayLiteralExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(StructInstantiationExpression& node) override;
};