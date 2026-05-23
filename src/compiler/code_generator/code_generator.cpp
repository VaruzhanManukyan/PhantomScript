#include "code_generator.hpp"
#include <algorithm>

#include "../parser/nodes/types/type_node.hpp"
#include "../parser/nodes/declarations/program_node.hpp"
#include "../parser/nodes/declarations/function_declaration.hpp"
#include "../parser/nodes/declarations/struct_declaration.hpp"
#include "../parser/nodes/declarations/enum_declaration.hpp"
#include "../parser/nodes/declarations/data_transfer_object_declaration.hpp"
#include "../parser/nodes/declarations/query_declaration.hpp"
#include "../parser/nodes/declarations/client_declaration.hpp"
#include "../parser/nodes/declarations/consumer_declaration.hpp"
#include "../parser/nodes/declarations/service_declaration.hpp"
#include "../parser/nodes/declarations/database_declaration.hpp"
#include "../parser/nodes/declarations/import_declaration.hpp"

#include "../parser/nodes/statements/expression_statement.hpp"
#include "../parser/nodes/statements/variable_declaration_statement.hpp"
#include "../parser/nodes/statements/return_statement.hpp"
#include "../parser/nodes/statements/if_statement.hpp"
#include "../parser/nodes/statements/while_statement.hpp"
#include "../parser/nodes/statements/for_statement.hpp"
#include "../parser/nodes/statements/for_in_statement.hpp"
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
#include "../parser/nodes/expressions/bool_literal_expression.hpp"
#include "../parser/nodes/expressions/null_literal_expression.hpp"
#include "../parser/nodes/expressions/int_literal_expression.hpp"
#include "../parser/nodes/expressions/double_literal_expression.hpp"
#include "../parser/nodes/expressions/string_literal_expression.hpp"
#include "../parser/nodes/expressions/array_literal_expression.hpp"
#include "../parser/nodes/expressions/identifier_expression.hpp"
#include "../parser/nodes/expressions/struct_instantiation_expression.hpp"

CompilationArtifacts CodeGenerator::generate(ProgramNode& program, const std::string& source_stem) {
    hpp_stream_.str("");
    hpp_stream_.clear();
    cpp_stream_.str("");
    cpp_stream_.clear();
    consumer_stream_.str("");
    consumer_stream_.clear();
    required_headers_.clear();
    function_param_types_.clear();
    function_param_names_.clear();
    all_parsed_databases_.clear();
    service_names_.clear();

    http_runtime_generated_ = false;
    uses_postgres_ = false;
    uses_rabbitmq_ = false;
    uses_service_ = false;
    indent_level_ = 0;

    required_headers_.insert("<string>");
    required_headers_.insert("<vector>");
    required_headers_.insert("<iostream>");
    required_headers_.insert("<variant>");
    required_headers_.insert("<stdexcept>");
    required_headers_.insert("<functional>");
    required_headers_.insert("<memory>");
    required_headers_.insert("<algorithm>");
    required_headers_.insert("<unordered_map>");
    required_headers_.insert("<thread>");

    program.accept(*this);

    if (uses_service_) {
        required_headers_.insert("<boost/asio.hpp>");
        required_headers_.insert("<boost/beast/core.hpp>");
        required_headers_.insert("<boost/beast/http.hpp>");
        required_headers_.insert("<boost/beast/version.hpp>");
        required_headers_.insert("<boost/asio/ip/tcp.hpp>");
        required_headers_.insert("<boost/asio/strand.hpp>");
    }

    std::stringstream final_hpp;
    final_hpp << "#pragma once\n";
    for (const auto& header : required_headers_) {
        final_hpp << "#include " << header << "\n";
    }
    final_hpp << "\n";

    final_hpp << "template<typename T, typename E>\nclass Result {\npublic:\n";
    final_hpp << "    struct OkValue { T value; };\n";
    final_hpp << "    struct ErrValue { E error; };\n";
    final_hpp << "    static Result Ok(T value) { return Result(OkValue{std::move(value)}); }\n";
    final_hpp << "    static Result Err(E error) { return Result(ErrValue{std::move(error)}); }\n";
    final_hpp << "    bool is_ok() const { return std::holds_alternative<OkValue>(data_); }\n";
    final_hpp << "    bool is_err() const { return std::holds_alternative<ErrValue>(data_); }\n";
    final_hpp <<
        "    T& unwrap() { if (!is_ok()) throw std::logic_error(\"unwrap() on Err\"); return std::get<OkValue>(data_).value; }\n";
    final_hpp <<
        "    const T& unwrap() const { if (!is_ok()) throw std::logic_error(\"unwrap() on Err\"); return std::get<OkValue>(data_).value; }\n";
    final_hpp <<
        "    E& unwrap_err() { if (!is_err()) throw std::logic_error(\"unwrap_err() on Ok\"); return std::get<ErrValue>(data_).error; }\n";
    final_hpp <<
        "    const E& unwrap_err() const { if (!is_err()) throw std::logic_error(\"unwrap_err() on Ok\"); return std::get<ErrValue>(data_).error; }\n";
    final_hpp <<
        "private:\n    std::variant<OkValue, ErrValue> data_;\n    explicit Result(OkValue v) : data_(std::move(v)) {}\n";
    final_hpp << "    explicit Result(ErrValue e) : data_(std::move(e)) {}\n};\n\n";

    if (uses_rabbitmq_) {
        final_hpp <<
            "class Publisher {\npublic:\n    void publish(const std::string&, const std::string&, const std::string&) {}\n};\n\n";
        final_hpp << "class ConsumerRuntime {\npublic:\n";
        final_hpp << "    template<typename EventTraits, typename Handler>\n";
        final_hpp << "    void subscribe(Handler&& handler) {\n";
        final_hpp << "        subscribe(EventTraits::routing_key, std::forward<Handler>(handler));\n";
        final_hpp << "    }\n";
        final_hpp <<
            "    void subscribe(const std::string& routing_key, std::function<void(const std::string&)> handler) {}\n";
        final_hpp << "};\n\n";
        final_hpp << "extern Publisher publisher;\n";
        final_hpp << "extern ConsumerRuntime consumer_runtime;\n\n";
    }

    final_hpp << hpp_stream_.str();

    std::stringstream final_cpp;
    final_cpp << "#include \"" << source_stem << ".hpp\"\n\n";

    if (uses_rabbitmq_) {
        final_cpp << "Publisher publisher;\nConsumerRuntime consumer_runtime;\n\n";
    }

    if (uses_postgres_ && !all_parsed_databases_.empty()) {
        final_cpp << "std::unique_ptr<" << all_parsed_databases_[0]->name_ << "> global_db;\n\n";
    }

    final_cpp << cpp_stream_.str();

    if (uses_service_) {
        final_cpp << "int main() {\n";
        final_cpp <<
            "    const std::size_t thread_count = std::max<std::size_t>(1, std::thread::hardware_concurrency());\n\n";
        final_cpp << "    net::io_context ioc;\n";
        final_cpp << "    net::signal_set signals(ioc, SIGINT, SIGTERM);\n";
        final_cpp <<
            "    signals.async_wait([&](const boost::system::error_code& ec, int) { if (!ec) ioc.stop(); });\n\n";

        if (uses_rabbitmq_) {
            final_cpp << consumer_stream_.str() << "\n";
        }

        if (uses_postgres_ && !all_parsed_databases_.empty()) {
            auto db_node = all_parsed_databases_[0];
            final_cpp << "    " << db_node->name_ << "Config db_config;\n";
            final_cpp << "    db_config.host = \"" << db_node->host_ << "\";\n";
            final_cpp << "    db_config.port = 5432;\n";
            final_cpp << "    db_config.name = \"" << db_node->db_name_ << "\";\n";
            final_cpp << "    db_config.user = \"phantom_developer\";\n";
            final_cpp << "    db_config.password = \"phantom_password\";\n";
            final_cpp << "    global_db = std::make_unique<" << db_node->name_ << ">(db_config);\n\n";
            final_cpp <<
                "    PQexec(global_db->raw(), \"CREATE TABLE IF NOT EXISTS users (id SERIAL PRIMARY KEY, name VARCHAR(255), email VARCHAR(255));\");\n\n";
        }

        for (const auto& svc_name : service_names_) {
            final_cpp << "    start_service_" << svc_name << "(ioc);\n";
        }

        final_cpp << "    std::cout << \"[Phantom Infrastructure] Global runtime started.\" << std::endl;\n";
        final_cpp << "    std::vector<std::thread> workers;\n";
        final_cpp << "    workers.reserve(thread_count);\n";
        final_cpp <<
            "    for (std::size_t i = 0; i < thread_count; ++i) workers.emplace_back([&ioc] { ioc.run(); });\n";
        final_cpp << "    for (auto& worker : workers) worker.join();\n";
        final_cpp << "    return 0;\n";
        final_cpp << "}\n";
    }

    CompilationArtifacts artifacts;
    artifacts.hpp_code = final_hpp.str();
    artifacts.cpp_code = final_cpp.str();
    artifacts.cmake_lists = generate_cmake();

    if (uses_service_ || uses_postgres_ || uses_rabbitmq_) {
        artifacts.dockerfile = generate_dockerfile();
        artifacts.docker_compose = generate_docker_compose();
    } else {
        artifacts.dockerfile = std::nullopt;
        artifacts.docker_compose = std::nullopt;
    }

    return artifacts;
}

std::string CodeGenerator::indent() const {
    return std::string(indent_level_ * 4, ' ');
}

std::string CodeGenerator::default_cpp_value(const std::string& cpp_type) const {
    if (cpp_type == "int") return "0";
    if (cpp_type == "double") return "0.0";
    if (cpp_type == "bool") return "false";
    if (cpp_type == "std::string") return "\"\"";
    if (cpp_type == "void") return "";
    if (cpp_type.rfind("std::optional<", 0) == 0) return "std::nullopt";
    return "{}";
}

std::string CodeGenerator::build_default_args(const std::string& fn_name) const {
    auto it = function_param_types_.find(fn_name);
    if (it == function_param_types_.end() || it->second.empty()) return "";
    std::string args;
    for (std::size_t i = 0; i < it->second.size(); ++i) {
        if (i > 0) args += ", ";
        args += default_cpp_value(it->second[i]);
    }
    return args;
}

std::string CodeGenerator::translate_type(TypeNode* type_node) {
    if (!type_node) return "void";
    std::string base = type_node->base_name_;
    if (base == "int") return "int";
    if (base == "double") return "double";
    if (base == "string") return "std::string";
    if (base == "bool") return "bool";
    if (base == "void") return "void";

    if (base == "array") return "std::vector<" + translate_type(type_node->generics_[0].get()) + ">";
    if (base == "list") {
        required_headers_.insert("<list>");
        return "std::list<" + translate_type(type_node->generics_[0].get()) + ">";
    }
    if (base == "map") {
        required_headers_.insert("<unordered_map>");
        return "std::unordered_map<" + translate_type(type_node->generics_[0].get()) + ", " + translate_type(
            type_node->generics_[1].get()) + ">";
    }
    if (base == "set") {
        required_headers_.insert("<unordered_set>");
        return "std::unordered_set<" + translate_type(type_node->generics_[0].get()) + ">";
    }
    if (base == "option") {
        required_headers_.insert("<optional>");
        return "std::optional<" + translate_type(type_node->generics_[0].get()) + ">";
    }
    if (base == "result") {
        std::string t = translate_type(type_node->generics_[0].get());
        std::string e = type_node->generics_.size() > 1 ? translate_type(type_node->generics_[1].get()) : "std::string";
        return "Result<" + t + ", " + e + ">";
    }
    return base;
}

void CodeGenerator::visit(ProgramNode& node) {
    for (const auto& decl : node.declarations_) {
        if (dynamic_cast<ImportDeclaration*>(decl.get())) decl->accept(*this);
    }
    for (const auto& decl : node.declarations_) {
        if (dynamic_cast<DataTransferObjectDeclaration*>(decl.get()) ||
            dynamic_cast<EnumDeclaration*>(decl.get()) ||
            dynamic_cast<StructDeclaration*>(decl.get()) ||
            dynamic_cast<DatabaseDeclaration*>(decl.get())) {
            decl->accept(*this);
        }
    }
    for (const auto& decl : node.declarations_) {
        if (dynamic_cast<FunctionDeclaration*>(decl.get()) ||
            dynamic_cast<QueryDeclaration*>(decl.get()) ||
            dynamic_cast<ClientDeclaration*>(decl.get()) ||
            dynamic_cast<ConsumerDeclaration*>(decl.get())) {
            decl->accept(*this);
        }
    }
    for (const auto& decl : node.declarations_) {
        if (dynamic_cast<ServiceDeclaration*>(decl.get())) {
            decl->accept(*this);
        }
    }
}

void CodeGenerator::visit(ImportDeclaration& node) {
    std::string cpp_path = node.path_;
    cpp_path.erase(std::remove(cpp_path.begin(), cpp_path.end(), '\"'), cpp_path.end());
    size_t pos = cpp_path.find(".ps");
    if (pos != std::string::npos) cpp_path.replace(pos, 3, ".hpp");
    hpp_stream_ << "#include \"" << cpp_path << "\"\n";
}

void CodeGenerator::visit(StructDeclaration& node) {
    hpp_stream_ << "struct " << node.name_ << " {\n";
    for (const auto& field : node.fields_) hpp_stream_ << "    " << translate_type(field.return_type_.get()) << " " <<
        field.name_ << ";\n";
    hpp_stream_ << "};\n\n";
}

void CodeGenerator::visit(DataTransferObjectDeclaration& node) {
    required_headers_.insert("<nlohmann/json.hpp>");
    hpp_stream_ << "struct " << node.name_ << " {\n";
    for (const auto& field : node.fields_) hpp_stream_ << "    " << translate_type(field.return_type_.get()) << " " <<
        field.name_ << ";\n";
    hpp_stream_ << "};\n\n";

    hpp_stream_ << "inline void to_json(nlohmann::json& j, const " << node.name_ <<
        "& value) {\n    j = nlohmann::json{\n";
    for (size_t i = 0; i < node.fields_.size(); ++i) {
        hpp_stream_ << "        {\"" << node.fields_[i].name_ << "\", value." << node.fields_[i].name_ << "}";
        if (i + 1 < node.fields_.size()) hpp_stream_ << ",";
        hpp_stream_ << "\n";
    }
    hpp_stream_ << "    };\n}\n\n";

    hpp_stream_ << "inline void from_json(const nlohmann::json& j, " << node.name_ << "& value) {\n";
    for (const auto& field : node.fields_) hpp_stream_ << "    j.at(\"" << field.name_ << "\").get_to(value." << field.
        name_ << ");\n";
    hpp_stream_ << "}\n\n";

    if (node.is_event_) {
        hpp_stream_ << "struct EventTraits_" << node.name_ << " {\n";
        hpp_stream_ << "    static constexpr const char* routing_key = \"" << node.name_ << "\";\n";
        hpp_stream_ << "    static constexpr const char* exchange = \"phantom.events\";\n};\n\n";
    }
}

void CodeGenerator::visit(EnumDeclaration& node) {
    hpp_stream_ << "enum class " << node.name_ << " {\n";
    for (const auto& variant : node.variants_) hpp_stream_ << "    " << variant << ",\n";
    hpp_stream_ << "};\n\n";
}

void CodeGenerator::visit(FunctionDeclaration& node) {
    std::string ret_t = translate_type(node.return_type_.get());
    std::vector<std::string> param_types;
    std::vector<std::string> param_names;
    for (const auto& p : node.parameters_) {
        param_names.push_back(p.name_);
        param_types.push_back(translate_type(p.type_.get()));
    }
    function_param_types_[node.name_] = std::move(param_types);
    function_param_names_[node.name_] = std::move(param_names);

    hpp_stream_ << ret_t << " " << node.name_ << "(";
    cpp_stream_ << ret_t << " " << node.name_ << "(";

    for (size_t i = 0; i < node.parameters_.size(); ++i) {
        TypeNode* pt = node.parameters_[i].type_.get();
        std::string type_str = translate_type(pt);
        bool is_collection = pt->base_name_ == "array" || pt->base_name_ == "list" || pt->base_name_ == "map" || pt->
            base_name_ == "set";

        if (is_collection) cpp_stream_ << type_str << " " << node.parameters_[i].name_;
        else cpp_stream_ << "const " << type_str << "& " << node.parameters_[i].name_;

        if (i + 1 < node.parameters_.size()) cpp_stream_ << ", ";
    }
    hpp_stream_ << ");\n\n";
    cpp_stream_ << ") ";

    if (node.body_) node.body_->accept(*this);
    else cpp_stream_ << "{}";
    cpp_stream_ << "\n\n";
}

void CodeGenerator::visit(QueryDeclaration& node) {
    uses_postgres_ = true;
    required_headers_.insert("<libpq-fe.h>");

    std::string ret_t = translate_type(node.return_type_.get());

    hpp_stream_ << ret_t << " " << node.name_ << "(";
    for (size_t i = 0; i < node.parameters_.size(); ++i) {
        hpp_stream_ << translate_type(node.parameters_[i].type_.get()) << " " << node.parameters_[i].name_;
        if (i + 1 < node.parameters_.size()) hpp_stream_ << ", ";
    }
    hpp_stream_ << ");\n\n";

    cpp_stream_ << ret_t << " " << node.name_ << "(";
    for (size_t i = 0; i < node.parameters_.size(); ++i) {
        cpp_stream_ << translate_type(node.parameters_[i].type_.get()) << " " << node.parameters_[i].name_;
        if (i + 1 < node.parameters_.size()) cpp_stream_ << ", ";
    }
    cpp_stream_ << ") {\n";

    for (const auto& param : node.parameters_) {
        if (param.type_->base_name_ == "int" || param.type_->base_name_ == "double") {
            cpp_stream_ << "    std::string " << param.name_ << "_text = std::to_string(" << param.name_ << ");\n";
        } else {
            cpp_stream_ << "    std::string " << param.name_ << "_text = " << param.name_ << ";\n";
        }
    }

    cpp_stream_ << "    const char* values[" << std::max<size_t>(1, node.parameters_.size()) << "] = {\n";
    for (size_t i = 0; i < node.parameters_.size(); ++i) {
        cpp_stream_ << "        " << node.parameters_[i].name_ << "_text.c_str()";
        if (i + 1 < node.parameters_.size()) cpp_stream_ << ",";
        cpp_stream_ << "\n";
    }
    cpp_stream_ << "    };\n\n";

    std::string processed_sql = node.sql_query_;
    size_t p_idx = 1;
    for (const auto& param : node.parameters_) {
        std::string target_ph = "$" + param.name_;
        size_t pos = 0;
        while ((pos = processed_sql.find(target_ph, pos)) != std::string::npos) {
            processed_sql.replace(pos, target_ph.length(), "$" + std::to_string(p_idx));
            pos += 2;
        }
        p_idx++;
    }

    cpp_stream_ << "    PGresult* raw_result = PQexecParams(global_db->raw(), R\"SQL(" << processed_sql << ")SQL\", " <<
        node.parameters_.size() << ", nullptr, " << (node.parameters_.empty() ? "nullptr" : "values") <<
        ", nullptr, nullptr, 0);\n";
    cpp_stream_ << "    std::unique_ptr<PGresult, decltype(&PQclear)> result(raw_result, &PQclear);\n\n";
    cpp_stream_ <<
        "    if (result == nullptr || (PQresultStatus(result.get()) != PGRES_TUPLES_OK && PQresultStatus(result.get()) != PGRES_COMMAND_OK)) {\n";
    cpp_stream_ << "        throw std::runtime_error(PQerrorMessage(global_db->raw()));\n    }\n\n";

    if (node.return_type_->base_name_ == "string") {
        cpp_stream_ << "    if (PQntuples(result.get()) > 0) return PQgetvalue(result.get(), 0, 0);\n";
        cpp_stream_ << "    return \"OK\";\n";
    } else if (node.return_type_->base_name_ == "int") {
        cpp_stream_ << "    if (PQntuples(result.get()) > 0) return std::stoi(PQgetvalue(result.get(), 0, 0));\n";
        cpp_stream_ << "    return 0;\n";
    } else {
        cpp_stream_ << "    return {};\n";
    }
    cpp_stream_ << "}\n\n";
}

void CodeGenerator::visit(DatabaseDeclaration& node) {
    uses_postgres_ = true;
    all_parsed_databases_.push_back(&node);
    required_headers_.insert("<libpq-fe.h>");
    required_headers_.insert("<memory>");

    hpp_stream_ << "struct " << node.name_ << "Config {\n";
    hpp_stream_ << "    std::string host = \"" << node.host_ << "\";\n";
    hpp_stream_ << "    int port = " << node.port_ << ";\n";
    hpp_stream_ << "    std::string name = \"" << node.db_name_ << "\";\n";
    hpp_stream_ << "    std::string user;\n    std::string password;\n};\n\n";

    hpp_stream_ << "class " << node.name_ << " {\npublic:\n";
    hpp_stream_ << "    explicit " << node.name_ << "(const " << node.name_ << "Config& config) {\n";
    hpp_stream_ <<
        "        std::string conninfo = \"host=\" + config.host + \" port=\" + std::to_string(config.port) + \" dbname=\" + config.name;\n";
    hpp_stream_ << "        if (!config.user.empty()) conninfo += \" user=\" + config.user;\n";
    hpp_stream_ << "        if (!config.password.empty()) conninfo += \" password=\" + config.password;\n";
    hpp_stream_ << "        conn_ = PQconnectdb(conninfo.c_str());\n";
    hpp_stream_ << "        if (conn_ == nullptr || PQstatus(conn_) != CONNECTION_OK) {\n";
    hpp_stream_ <<
        "            std::string msg = conn_ != nullptr ? PQerrorMessage(conn_) : \"cannot allocate PGconn\";\n";
    hpp_stream_ << "            if (conn_ != nullptr) PQfinish(conn_);\n";
    hpp_stream_ << "            throw std::runtime_error(msg);\n        }\n    }\n";
    hpp_stream_ << "    ~" << node.name_ << "() { if (conn_ != nullptr) PQfinish(conn_); }\n";
    hpp_stream_ << "    PGconn* raw() { return conn_; }\n";
    hpp_stream_ << "private:\n    PGconn* conn_ = nullptr;\n};\n\n";

    hpp_stream_ << "extern std::unique_ptr<" << node.name_ << "> global_db;\n\n";
}

void CodeGenerator::visit(ClientDeclaration& node) {
    required_headers_.insert("<boost/asio.hpp>");
    required_headers_.insert("<boost/beast.hpp>");
    required_headers_.insert("<nlohmann/json.hpp>");

    hpp_stream_ << "class " << node.name_ << " {\npublic:\n";
    hpp_stream_ << "    " << node.name_ << "(net::any_io_executor ex) : executor_(ex), host_(\"" << node.host_ <<
        "\"), port_(\"" << node.port_ << "\") {}\n\n";

    for (const auto& req : node.requests_) {
        std::string ret_t = translate_type(req.return_type_.get());
        hpp_stream_ << "    " << ret_t << " " << req.local_function_name_ << "(";
        for (size_t i = 0; i < req.parameters_.size(); ++i) {
            hpp_stream_ << "int " << req.parameters_[i].name_;
            if (i + 1 < req.parameters_.size()) hpp_stream_ << ", ";
        }
        hpp_stream_ << ") {\n        try {\n";
        hpp_stream_ <<
            "            net::io_context ioc;\n            tcp::resolver resolver(ioc);\n            beast::tcp_stream stream(ioc);\n";
        hpp_stream_ <<
            "            auto const results = resolver.resolve(host_, port_);\n            stream.connect(results);\n";
        hpp_stream_ << "            std::string target = \"/users/\" + std::to_string(id);\n";
        hpp_stream_ << "            http::request<http::string_body> req{http::verb::get, target, 11};\n";
        hpp_stream_ <<
            "            req.set(http::field::host, host_);\n            req.set(http::field::user_agent, \"PhantomScript-Client\");\n";
        hpp_stream_ << "            http::write(stream, req);\n";
        hpp_stream_ << "            beast::flat_buffer buffer;\n            http::response<http::string_body> res;\n";
        hpp_stream_ << "            http::read(stream, buffer, res);\n";
        hpp_stream_ <<
            "            beast::error_code ec;\n            stream.socket().shutdown(tcp::socket::shutdown_both, ec);\n";
        hpp_stream_ << "            if (res.result() != http::status::ok) return " << ret_t <<
            "::Err(\"HTTP Error\");\n";
        hpp_stream_ << "            auto json_resp = nlohmann::json::parse(res.body());\n";

        if (req.return_type_->base_name_ == "result") {
            std::string inner_v = translate_type(req.return_type_->generics_[0].get());
            hpp_stream_ << "            return " << ret_t << "::Ok(json_resp.get<" << inner_v << ">());\n";
        } else hpp_stream_ << "            return json_resp.get<" << ret_t << ">();\n";
        hpp_stream_ << "        } catch (const std::exception& e) {\n            return " << ret_t <<
            "::Err(e.what());\n        }\n    }\n";
    }
    hpp_stream_ <<
        "private:\n    net::any_io_executor executor_;\n    std::string host_;\n    std::string port_;\n};\n\n";
}

void CodeGenerator::visit(ConsumerDeclaration& node) {
    uses_rabbitmq_ = true;
    consumer_stream_ << "    consumer_runtime.subscribe<EventTraits_" << node.event_name_ << ">(\n";
    consumer_stream_ << "        [&](const std::string& payload) {\n";
    consumer_stream_ << "            try {\n";
    consumer_stream_ << "                auto j = nlohmann::json::parse(payload);\n";
    consumer_stream_ << "                " << node.event_name_ << " event = j.get<" << node.event_name_ << ">();\n";
    consumer_stream_ << "                " << node.handler_function_name_ << "(event);\n";
    consumer_stream_ << "            } catch (const std::exception& e) {\n";
    consumer_stream_ << "                std::cerr << \"Consumer parse error: \" << e.what() << '\\n';\n";
    consumer_stream_ << "            }\n";
    consumer_stream_ << "        });\n";
}

void CodeGenerator::visit(ServiceDeclaration& node) {
    uses_service_ = true;
    service_names_.push_back(node.name_);

    generate_http_runtime();

    cpp_stream_ << "void start_service_" << node.name_ << "(net::io_context& ioc) {\n";
    cpp_stream_ << "    Router router;\n\n";

    for (const auto& route : node.flat_routes_) {
        std::string method_lower = route->http_method_;
        for (char& c : method_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        cpp_stream_ << "    router.add(http::verb::" << method_lower << ", \"" << route->path_ << "\",\n";
        cpp_stream_ << "        [](const RequestContext& ctx) -> http::response<http::string_body> {\n";
        cpp_stream_ <<
            "            http::response<http::string_body> res{http::status::ok, ctx.request().version()};\n";
        cpp_stream_ << "            res.set(http::field::server, \"PhantomService\");\n";

        auto it = function_param_types_.find(route->handler_name_);
        if (it == function_param_types_.end() || it->second.empty()) {
            cpp_stream_ << "            auto result = " << route->handler_name_ << "();\n";
            cpp_stream_ << "            nlohmann::json j = result;\n";
            cpp_stream_ <<
                "            if (j.is_string()) {\n                res.set(http::field::content_type, \"text/plain\");\n                res.body() = j.get<std::string>();\n            } else {\n                res.set(http::field::content_type, \"application/json\");\n                res.body() = j.dump();\n            }\n";
            cpp_stream_ << "            res.prepare_payload();\n            return res;\n";
        } else {
            auto name_it = function_param_names_.find(route->handler_name_);
            const auto& param_names = (name_it != function_param_names_.end())
                                          ? name_it->second
                                          : std::vector<std::string>{};

            cpp_stream_ << "            try {\n";
            cpp_stream_ << "                auto body_json = nlohmann::json::parse(ctx.body());\n";
            cpp_stream_ << "                auto result = " << route->handler_name_ << "(";
            for (size_t i = 0; i < it->second.size(); ++i) {
                std::string param_name = (i < param_names.size()) ? param_names[i] : "param" + std::to_string(i);
                cpp_stream_ << "body_json[\"" << param_name << "\"].get<" << it->second[i] << ">()";
                if (i + 1 < it->second.size()) cpp_stream_ << ", ";
            }
            cpp_stream_ << ");\n";

            cpp_stream_ << "                nlohmann::json j = result;\n";
            cpp_stream_ <<
                "                if (j.is_string()) {\n                    res.set(http::field::content_type, \"text/plain\");\n                    res.body() = j.get<std::string>();\n                } else {\n                    res.set(http::field::content_type, \"application/json\");\n                    res.body() = j.dump();\n                }\n";
            cpp_stream_ << "                res.prepare_payload();\n                return res;\n";

            cpp_stream_ << "            } catch (const std::exception& e) {\n";
            cpp_stream_ <<
                "                res.result(http::status::bad_request);\n                res.body() = \"Invalid request body\";\n                res.prepare_payload();\n                return res;\n";
            cpp_stream_ << "            }\n";
        }
        cpp_stream_ << "        });\n\n";
    }

    cpp_stream_ << "    auto listener = std::make_shared<Listener>(\n";
    cpp_stream_ << "        net::make_strand(ioc),\n";
    cpp_stream_ << "        tcp::endpoint{net::ip::make_address(\"0.0.0.0\"), 8081},\n";
    cpp_stream_ << "        std::move(router)\n    );\n";
    cpp_stream_ << "    listener->run();\n";
    cpp_stream_ << "}\n\n";
}

void CodeGenerator::generate_http_runtime() {
    if (http_runtime_generated_) return;
    http_runtime_generated_ = true;

    hpp_stream_ <<
        "namespace beast = boost::beast;\nnamespace http = beast::http;\nnamespace net = boost::asio;\nusing tcp = net::ip::tcp;\n\n";

    hpp_stream_ << "class RequestContext {\npublic:\n";
    hpp_stream_ <<
        "    explicit RequestContext(const http::request<http::string_body>& req, std::unordered_map<std::string, std::string> path_params = {})\n";
    hpp_stream_ << "        : request_(req), path_params_(std::move(path_params)) {}\n";
    hpp_stream_ << "    const http::request<http::string_body>& request() const { return request_; }\n";
    hpp_stream_ << "    std::string param(const std::string& name) const {\n";
    hpp_stream_ <<
        "        auto it = path_params_.find(name);\n        return it == path_params_.end() ? \"\" : it->second;\n    }\n";
    hpp_stream_ << "    const std::string& body() const { return request_.body(); }\n";
    hpp_stream_ <<
        "private:\n    const http::request<http::string_body>& request_;\n    std::unordered_map<std::string, std::string> path_params_;\n};\n\n";

    hpp_stream_ <<
        "class Router {\npublic:\n    using Handler = std::function<http::response<http::string_body>(const RequestContext&)>;\n";
    hpp_stream_ <<
        "    void add(http::verb method, std::string path, Handler handler) { routes_.push_back({method, std::move(path), std::move(handler)}); }\n";
    hpp_stream_ <<
        "    http::response<http::string_body> dispatch(const http::request<http::string_body>& req) const {\n";
    hpp_stream_ <<
        "        std::string target = std::string(req.target());\n        size_t qpos = target.find('?');\n        if (qpos != std::string::npos) target = target.substr(0, qpos);\n";
    hpp_stream_ <<
        "        for (const auto& route : routes_) {\n            std::unordered_map<std::string, std::string> params;\n";
    hpp_stream_ << "            if (route.method == req.method() && match_path(route.path, target, params)) {\n";
    hpp_stream_ <<
        "                RequestContext ctx(req, std::move(params));\n                return route.handler(ctx);\n            }\n        }\n";
    hpp_stream_ <<
        "        http::response<http::string_body> res{http::status::not_found, req.version()};\n        res.body() = \"404 Not Found\";\n        res.prepare_payload();\n        return res;\n    }\n";
    hpp_stream_ <<
        "private:\n    struct RouteEntry { http::verb method; std::string path; Handler handler; };\n    std::vector<RouteEntry> routes_;\n";
    hpp_stream_ <<
        "    static std::vector<std::string> split_path(const std::string& path) {\n        std::vector<std::string> res;\n        std::string curr;\n        for (char c : path) {\n";
    hpp_stream_ <<
        "            if (c == '/') { if (!curr.empty()) { res.push_back(curr); curr.clear(); } }\n            else { curr += c; }\n        }\n";
    hpp_stream_ << "        if (!curr.empty()) res.push_back(curr);\n        return res;\n    }\n";
    hpp_stream_ <<
        "    static bool is_param(const std::string& s) { return s.size()>2 && s.front()=='{' && s.back()=='}'; }\n";
    hpp_stream_ << "    static std::string param_name(const std::string& s) { return s.substr(1, s.size()-2); }\n";
    hpp_stream_ <<
        "    static bool match_path(const std::string& pattern, const std::string& path, std::unordered_map<std::string, std::string>& params) {\n";
    hpp_stream_ <<
        "        auto p_parts = split_path(pattern);\n        auto r_parts = split_path(path);\n        if (p_parts.size() != r_parts.size()) return false;\n";
    hpp_stream_ <<
        "        for (size_t i = 0; i < p_parts.size(); ++i) {\n            if (is_param(p_parts[i])) params[param_name(p_parts[i])] = r_parts[i];\n            else if (p_parts[i] != r_parts[i]) return false;\n        }\n        return true;\n    }\n};\n\n";

    hpp_stream_ << "class HttpSession : public std::enable_shared_from_this<HttpSession> {\npublic:\n";
    hpp_stream_ <<
        "    HttpSession(tcp::socket&& socket, const Router& router) : stream_(std::move(socket)), router_(router) {}\n    void run() { read_request(); }\n";
    hpp_stream_ <<
        "private:\n    beast::tcp_stream stream_;\n    beast::flat_buffer buffer_;\n    http::request<http::string_body> request_;\n    http::response<http::string_body> response_;\n    const Router& router_;\n";
    hpp_stream_ <<
        "    void read_request() { http::async_read(stream_, buffer_, request_, [self = shared_from_this()](beast::error_code ec, size_t) { self->on_read(ec); }); }\n";
    hpp_stream_ <<
        "    void on_read(beast::error_code ec) {\n        if (ec == http::error::end_of_stream) { close(); return; }\n        if (ec) return;\n        response_ = router_.dispatch(request_);\n        response_.keep_alive(request_.keep_alive());\n";
    hpp_stream_ <<
        "        http::async_write(stream_, response_, [self = shared_from_this()](beast::error_code ec, size_t) { self->on_write(ec); });\n    }\n";
    hpp_stream_ <<
        "    void on_write(beast::error_code ec) { if (ec) return; if (response_.need_eof()) { close(); return; } read_request(); }\n";
    hpp_stream_ <<
        "    void close() { beast::error_code ec; stream_.socket().shutdown(tcp::socket::shutdown_send, ec); }\n};\n\n";

    hpp_stream_ << "class Listener : public std::enable_shared_from_this<Listener> {\npublic:\n";
    hpp_stream_ <<
        "    Listener(net::any_io_executor ex, tcp::endpoint ep, Router r) : acceptor_(ex), router_(std::move(r)) {\n        beast::error_code ec;\n        acceptor_.open(ep.protocol(), ec);\n        acceptor_.set_option(net::socket_base::reuse_address(true), ec);\n        acceptor_.bind(ep, ec);\n        acceptor_.listen(net::socket_base::max_listen_connections, ec);\n    }\n";
    hpp_stream_ <<
        "    void run() { do_accept(); }\nprivate:\n    void do_accept() {\n        acceptor_.async_accept(net::make_strand(acceptor_.get_executor()),\n";
    hpp_stream_ <<
        "            [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {\n                if (!ec) std::make_shared<HttpSession>(std::move(socket), self->router_)->run();\n                self->do_accept();\n            });\n    }\n    tcp::acceptor acceptor_;\n    Router router_;\n};\n\n";
}

void CodeGenerator::visit(BlockStatement& node) {
    cpp_stream_ << "{\n";
    indent_level_++;
    for (const auto& stmt : node.statements_) {
        cpp_stream_ << indent();
        stmt->accept(*this);
    }
    indent_level_--;
    cpp_stream_ << indent() << "}";
}

void CodeGenerator::visit(PublishStatement& node) {
    uses_rabbitmq_ = true;
    cpp_stream_ << "    " << node.event_name_ << " event{\n";
    for (const auto& f : node.fields_) {
        cpp_stream_ << "        ." << f.name_ << " = ";
        f.value_->accept(*this);
        cpp_stream_ << ",\n";
    }
    cpp_stream_ << "    };\n    nlohmann::json payload = event;\n";
    cpp_stream_ << "    publisher.publish(EventTraits_" << node.event_name_ << "::exchange, EventTraits_" << node.
        event_name_ << "::routing_key, payload.dump());\n";
}

void CodeGenerator::visit(ExpressionStatement& node) {
    node.expression_->accept(*this);
    cpp_stream_ << ";\n";
}

void CodeGenerator::visit(VariableDeclarationStatement& node) {
    if (!node.is_mut_) cpp_stream_ << "const ";
    cpp_stream_ << translate_type(node.type_.get()) << " " << node.name_;
    if (node.initializer_) {
        cpp_stream_ << " = ";
        node.initializer_->accept(*this);
    }
    cpp_stream_ << ";\n";
}

void CodeGenerator::visit(ReturnStatement& node) {
    cpp_stream_ << "return ";
    if (node.expression_) node.expression_->accept(*this);
    cpp_stream_ << ";\n";
}

void CodeGenerator::visit(IfStatement& node) {
    cpp_stream_ << "if (";
    node.expression_->accept(*this);
    cpp_stream_ << ") ";
    node.then_branch_->accept(*this);
    if (node.else_branch_) {
        cpp_stream_ << " else ";
        node.else_branch_->accept(*this);
    }
    cpp_stream_ << "\n";
}

void CodeGenerator::visit(WhileStatement& node) {
    cpp_stream_ << "while (";
    node.condition_->accept(*this);
    cpp_stream_ << ") ";
    if (node.body_) node.body_->accept(*this);
    cpp_stream_ << "\n";
}

void CodeGenerator::visit(ForStatement& node) {
    cpp_stream_ << "for (";
    if (node.init_) {
        if (auto* vd = dynamic_cast<VariableDeclarationStatement*>(node.init_.get())) {
            if (!vd->is_mut_) cpp_stream_ << "const ";
            cpp_stream_ << translate_type(vd->type_.get()) << " " << vd->name_;
            if (vd->initializer_) {
                cpp_stream_ << " = ";
                vd->initializer_->accept(*this);
            }
        } else if (auto* es = dynamic_cast<ExpressionStatement*>(node.init_.get())) {
            es->expression_->accept(*this);
        }
    }
    cpp_stream_ << "; ";
    if (node.condition_) node.condition_->accept(*this);
    cpp_stream_ << "; ";
    if (node.increment_) node.increment_->accept(*this);
    cpp_stream_ << ") ";
    if (node.body_) node.body_->accept(*this);
    cpp_stream_ << "\n";
}

void CodeGenerator::visit(ForInStatement& node) {
    cpp_stream_ << "for (const auto& " << node.variable_name_ << " : ";
    node.iterable_->accept(*this);
    cpp_stream_ << ") ";
    if (node.body_) node.body_->accept(*this);
    cpp_stream_ << "\n";
}

void CodeGenerator::visit(BreakStatement& node) {
    cpp_stream_ << "break;\n";
}

void CodeGenerator::visit(ContinueStatement& node) {
    cpp_stream_ << "continue;\n";
}

void CodeGenerator::visit(PrintStatement& node) {
    if (auto* ident = dynamic_cast<IdentifierExpression*>(node.expression_.get())) {
        cpp_stream_ << "{\n" << indent() << "    bool __first = true;\n" << indent() << "    std::cout << '[';\n" <<
            indent() << "    for (const auto& __el : " << ident->name_ << ") {\n" << indent() <<
            "        if (!__first) std::cout << \", \";\n" << indent() << "        std::cout << __el;\n" << indent() <<
            "        __first = false;\n" << indent() << "    }\n" << indent() << "    std::cout << \"]\\n\";\n" <<
            indent() << "}\n";
        return;
    }
    cpp_stream_ << "std::cout << ";
    node.expression_->accept(*this);
    cpp_stream_ << " << '\\n';\n";
}

void CodeGenerator::visit(MatchStatement& node) {
    cpp_stream_ << "    [&]() {\n        auto&& match_subject = ";
    node.expression_->accept(*this);
    cpp_stream_ << ";\n";

    bool is_res = false, is_opt = false;
    for (const auto& c : node.match_cases_) {
        if (c.pattern_name_ == "Ok" || c.pattern_name_ == "Err") is_res = true;
        if (c.pattern_name_ == "Some" || c.pattern_name_ == "None") is_opt = true;
    }

    if (is_opt) {
        for (const auto& c : node.match_cases_) {
            if (c.pattern_name_ == "Some") {
                cpp_stream_ << "        if (match_subject.has_value()) {\n";
                if (c.bound_variable_) cpp_stream_ << "            auto " << *c.bound_variable_ <<
                    " = *match_subject;\n";
                c.body_->accept(*this);
                cpp_stream_ << "\n        }\n";
            } else {
                cpp_stream_ << "        else {\n";
                c.body_->accept(*this);
                cpp_stream_ << "\n        }\n";
            }
        }
    } else if (is_res) {
        for (const auto& c : node.match_cases_) {
            if (c.pattern_name_ == "Ok") {
                cpp_stream_ << "        if (match_subject.is_ok()) {\n";
                if (c.bound_variable_) cpp_stream_ << "            auto " << *c.bound_variable_ <<
                    " = match_subject.unwrap();\n";
                c.body_->accept(*this);
                cpp_stream_ << "\n        }\n";
            } else {
                cpp_stream_ << "        else {\n";
                if (c.bound_variable_) cpp_stream_ << "            auto " << *c.bound_variable_ <<
                    " = match_subject.unwrap_err();\n";
                c.body_->accept(*this);
                cpp_stream_ << "\n        }\n";
            }
        }
    } else {
        cpp_stream_ << "        switch (match_subject) {\n";
        for (const auto& c : node.match_cases_) {
            std::string pat = c.pattern_name_;
            std::replace(pat.begin(), pat.end(), '.', ':');
            cpp_stream_ << "            case " << pat << ": {\n";
            c.body_->accept(*this);
            cpp_stream_ << "\n                break;\n            }\n";
        }
        cpp_stream_ << "        }\n";
    }
    cpp_stream_ << "    }()";
}

void CodeGenerator::visit(AssignmentExpression& node) {
    node.target_->accept(*this);
    cpp_stream_ << " " << node.operator_ << " ";
    node.value_->accept(*this);
}

void CodeGenerator::visit(BinaryExpression& node) {
    cpp_stream_ << "(";
    node.left_->accept(*this);
    cpp_stream_ << " " << node.operator_ << " ";
    node.right_->accept(*this);
    cpp_stream_ << ")";
}

void CodeGenerator::visit(UnaryExpression& node) {
    cpp_stream_ << node.operator_;
    node.right_->accept(*this);
}

void CodeGenerator::visit(CallExpression& node) {
    if (auto* ident = dynamic_cast<IdentifierExpression*>(node.callee_.get())) {
        if (ident->name_ == "ok") {
            cpp_stream_ << "Result<auto, auto>::Ok(";
            node.arguments_[0]->accept(*this);
            cpp_stream_ << ")";
            return;
        }
        if (ident->name_ == "err") {
            cpp_stream_ << "Result<auto, auto>::Err(";
            node.arguments_[0]->accept(*this);
            cpp_stream_ << ")";
            return;
        }
    }
    node.callee_->accept(*this);
    cpp_stream_ << "(";
    for (size_t i = 0; i < node.arguments_.size(); ++i) {
        node.arguments_[i]->accept(*this);
        if (i + 1 < node.arguments_.size()) cpp_stream_ << ", ";
    }
    cpp_stream_ << ")";
}

void CodeGenerator::visit(MemberAccessExpression& node) {
    node.object_->accept(*this);
    if (node.member_name_ == "length") cpp_stream_ << ".size()";
    else cpp_stream_ << "." << node.member_name_;
}

void CodeGenerator::visit(IndexAccessExpression& node) {
    node.object_->accept(*this);
    cpp_stream_ << "[";
    node.index_->accept(*this);
    cpp_stream_ << "]";
}

void CodeGenerator::visit(CastExpression& node) {
    if (node.type_->base_name_ == "string") {
        cpp_stream_ << "std::to_string(";
        node.value_->accept(*this);
        cpp_stream_ << ")";
    } else {
        cpp_stream_ << "static_cast<" << translate_type(node.type_.get()) << ">(";
        node.value_->accept(*this);
        cpp_stream_ << ")";
    }
}

void CodeGenerator::visit(IdentifierExpression& node) { cpp_stream_ << node.name_; }

void CodeGenerator::visit(StructInstantiationExpression& node) {
    cpp_stream_ << node.struct_name_ << " { ";
    for (size_t i = 0; i < node.fields_.size(); ++i) {
        cpp_stream_ << "." << node.fields_[i].name_ << " = ";
        node.fields_[i].value_->accept(*this);
        if (i + 1 < node.fields_.size()) cpp_stream_ << ", ";
    }
    cpp_stream_ << " }";
}

void CodeGenerator::visit(BoolLiteralExpression& node) { cpp_stream_ << (node.value_ ? "true" : "false"); }
void CodeGenerator::visit(NullLiteralExpression& node) { cpp_stream_ << "std::nullopt"; }
void CodeGenerator::visit(IntLIteralExpression& node) { cpp_stream_ << node.value_; }
void CodeGenerator::visit(DoubleLiteralExpression& node) { cpp_stream_ << node.value_; }
void CodeGenerator::visit(StringLiteralExpression& node) { cpp_stream_ << "\"" << node.value_ << "\""; }

void CodeGenerator::visit(ArrayLiteralExpression& node) {
    cpp_stream_ << "{";
    for (std::size_t i = 0; i < node.elements_.size(); ++i) {
        node.elements_[i]->accept(*this);
        if (i + 1 < node.elements_.size()) cpp_stream_ << ", ";
    }
    cpp_stream_ << "}";
}

std::string CodeGenerator::generate_dockerfile() const {
    std::stringstream ds;
    ds << "FROM ubuntu:22.04 AS builder\nENV DEBIAN_FRONTEND=noninteractive\n";
    ds << "RUN apt-get update && apt-get install -y build-essential cmake git libboost-all-dev nlohmann-json3-dev";
    if (uses_postgres_) ds << " libpq-dev";
    ds << " && rm -rf /var/lib/apt/lists/*\nWORKDIR /app\nCOPY . .\n";
    ds << "RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . --config Release\n\n";
    ds << "FROM ubuntu:22.04\nENV DEBIAN_FRONTEND=noninteractive\n";
    ds << "RUN apt-get update && apt-get install -y libboost-system1.74.0 libboost-thread1.74.0";
    if (uses_postgres_) ds << " libpq5";
    ds <<
        " && rm -rf /var/lib/apt/lists/*\nWORKDIR /app\nCOPY --from=builder /app/build/phantom_service .\nEXPOSE 8081\nCMD [\"./phantom_service\"]\n";
    return ds.str();
}

std::string CodeGenerator::generate_docker_compose() const {
    std::stringstream dc;
    dc << "version: '3.8'\n\nservices:\n";

    if (uses_postgres_) {
        for (const auto& db_node : all_parsed_databases_) {
            dc << "  " << db_node->host_ << ":\n";
            dc << "    image: " << db_node->engine_ << ":15-alpine\n";
            dc << "    container_name: phantom_" << db_node->host_ << "\n";
            dc << "    environment:\n";
            dc << "      POSTGRES_DB: " << db_node->db_name_ << "\n";
            dc << "      POSTGRES_USER: phantom_developer\n";
            dc << "      POSTGRES_PASSWORD: phantom_password\n";
            dc << "    ports:\n      - \"" << db_node->port_ << ":5432\"\n";
            dc << "    healthcheck:\n";
            dc << "      test: [\"CMD-SHELL\", \"pg_isready -U phantom_developer -d " << db_node->db_name_ <<
                " -p 5432\"]\n";
            dc << "      interval: 5s\n      timeout: 5s\n      retries: 5\n\n";
        }
    }
    if (uses_rabbitmq_) {
        dc << "  rabbitmq:\n    image: rabbitmq:3-management-alpine\n    container_name: phantom_rabbitmq\n";
        dc << "    ports:\n      - \"5672:5672\"\n      - \"15672:15672\"\n    healthcheck:\n";
        dc <<
            "      test: [\"CMD-SHELL\", \"rabbitmq-diagnostics -q ping\"]\n      interval: 5s\n      timeout: 5s\n      retries: 5\n\n";
    }
    dc <<
        "  app:\n    build:\n      context: .\n      dockerfile: Dockerfile\n    container_name: phantom_backend_app\n    ports:\n      - \"8081:8081\"\n";
    if (uses_postgres_ || uses_rabbitmq_) {
        dc << "    depends_on:\n";
        if (uses_postgres_ && !all_parsed_databases_.empty()) {
            dc << "      " << all_parsed_databases_[0]->host_ << ":\n        condition: service_healthy\n";
        }
        if (uses_rabbitmq_) {
            dc << "      rabbitmq:\n        condition: service_healthy\n";
        }
    }
    return dc.str();
}

std::string CodeGenerator::generate_cmake() const {
    std::stringstream cm;
    cm << "cmake_minimum_required(VERSION 3.20)\nproject(PhantomService LANGUAGES CXX)\n\n";
    cm <<
        "set(CMAKE_CXX_STANDARD 20)\nset(CMAKE_CXX_STANDARD_REQUIRED ON)\nset(CMAKE_POSITION_INDEPENDENT_CODE ON)\n\n";
    cm << "find_package(Boost REQUIRED COMPONENTS system thread)\nfind_package(nlohmann_json REQUIRED)\n\n";
    if (uses_postgres_) cm << "find_package(PostgreSQL REQUIRED)\n\n";
    cm << "file(GLOB SOURCES \"*.cpp\")\nadd_executable(phantom_service ${SOURCES})\n\n";
    cm << "target_include_directories(phantom_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})\n\n";
    cm << "target_link_libraries(phantom_service PRIVATE Boost::system Boost::thread nlohmann_json::nlohmann_json\n";
    if (uses_postgres_) cm << "    PostgreSQL::PostgreSQL\n";
    cm << ")\n";
    return cm.str();
}
