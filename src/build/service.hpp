#pragma once
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <functional>
#include <iostream>
#include <libpq-fe.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

template<typename T, typename E>
class Result {
public:
    struct OkValue { T value; };
    struct ErrValue { E error; };
    static Result Ok(T value) { return Result(OkValue{std::move(value)}); }
    static Result Err(E error) { return Result(ErrValue{std::move(error)}); }
    bool is_ok() const { return std::holds_alternative<OkValue>(data_); }
    bool is_err() const { return std::holds_alternative<ErrValue>(data_); }
    T& unwrap() { if (!is_ok()) throw std::logic_error("unwrap() on Err"); return std::get<OkValue>(data_).value; }
    const T& unwrap() const { if (!is_ok()) throw std::logic_error("unwrap() on Err"); return std::get<OkValue>(data_).value; }
    E& unwrap_err() { if (!is_err()) throw std::logic_error("unwrap_err() on Ok"); return std::get<ErrValue>(data_).error; }
    const E& unwrap_err() const { if (!is_err()) throw std::logic_error("unwrap_err() on Ok"); return std::get<ErrValue>(data_).error; }
private:
    std::variant<OkValue, ErrValue> data_;
    explicit Result(OkValue v) : data_(std::move(v)) {}
    explicit Result(ErrValue e) : data_(std::move(e)) {}
};

class Publisher {
public:
    void publish(const std::string&, const std::string&, const std::string&) {}
};

class ConsumerRuntime {
public:
    template<typename EventTraits, typename Handler>
    void subscribe(Handler&& handler) {
        subscribe(EventTraits::routing_key, std::forward<Handler>(handler));
    }
    void subscribe(const std::string& routing_key, std::function<void(const std::string&)> handler) {}
};

extern Publisher publisher;
extern ConsumerRuntime consumer_runtime;

struct UserCreated {
    int user_id;
    std::string email;
};

inline void to_json(nlohmann::json& j, const UserCreated& value) {
    j = nlohmann::json{
        {"user_id", value.user_id},
        {"email", value.email}
    };
}

inline void from_json(const nlohmann::json& j, UserCreated& value) {
    j.at("user_id").get_to(value.user_id);
    j.at("email").get_to(value.email);
}

struct EventTraits_UserCreated {
    static constexpr const char* routing_key = "UserCreated";
    static constexpr const char* exchange = "phantom.events";
};

struct PrimaryDbConfig {
    std::string host = "postgres";
    int port = 5443;
    std::string name = "main_db";
    std::string user;
    std::string password;
};

class PrimaryDb {
public:
    explicit PrimaryDb(const PrimaryDbConfig& config) {
        std::string conninfo = "host=" + config.host + " port=" + std::to_string(config.port) + " dbname=" + config.name;
        if (!config.user.empty()) conninfo += " user=" + config.user;
        if (!config.password.empty()) conninfo += " password=" + config.password;
        conn_ = PQconnectdb(conninfo.c_str());
        if (conn_ == nullptr || PQstatus(conn_) != CONNECTION_OK) {
            std::string msg = conn_ != nullptr ? PQerrorMessage(conn_) : "cannot allocate PGconn";
            if (conn_ != nullptr) PQfinish(conn_);
            throw std::runtime_error(msg);
        }
    }
    ~PrimaryDb() { if (conn_ != nullptr) PQfinish(conn_); }
    PGconn* raw() { return conn_; }
private:
    PGconn* conn_ = nullptr;
};

extern std::unique_ptr<PrimaryDb> global_db;

std::string insert_user(std::string name, std::string email);

std::string batch_create_users();

void handle_notification();

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class RequestContext {
public:
    explicit RequestContext(const http::request<http::string_body>& req, std::unordered_map<std::string, std::string> path_params = {})
        : request_(req), path_params_(std::move(path_params)) {}
    const http::request<http::string_body>& request() const { return request_; }
    std::string param(const std::string& name) const {
        auto it = path_params_.find(name);
        return it == path_params_.end() ? "" : it->second;
    }
    const std::string& body() const { return request_.body(); }
private:
    const http::request<http::string_body>& request_;
    std::unordered_map<std::string, std::string> path_params_;
};

class Router {
public:
    using Handler = std::function<http::response<http::string_body>(const RequestContext&)>;
    void add(http::verb method, std::string path, Handler handler) { routes_.push_back({method, std::move(path), std::move(handler)}); }
    http::response<http::string_body> dispatch(const http::request<http::string_body>& req) const {
        std::string target = std::string(req.target());
        size_t qpos = target.find('?');
        if (qpos != std::string::npos) target = target.substr(0, qpos);
        for (const auto& route : routes_) {
            std::unordered_map<std::string, std::string> params;
            if (route.method == req.method() && match_path(route.path, target, params)) {
                RequestContext ctx(req, std::move(params));
                return route.handler(ctx);
            }
        }
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.body() = "404 Not Found";
        res.prepare_payload();
        return res;
    }
private:
    struct RouteEntry { http::verb method; std::string path; Handler handler; };
    std::vector<RouteEntry> routes_;
    static std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> res;
        std::string curr;
        for (char c : path) {
            if (c == '/') { if (!curr.empty()) { res.push_back(curr); curr.clear(); } }
            else { curr += c; }
        }
        if (!curr.empty()) res.push_back(curr);
        return res;
    }
    static bool is_param(const std::string& s) { return s.size()>2 && s.front()=='{' && s.back()=='}'; }
    static std::string param_name(const std::string& s) { return s.substr(1, s.size()-2); }
    static bool match_path(const std::string& pattern, const std::string& path, std::unordered_map<std::string, std::string>& params) {
        auto p_parts = split_path(pattern);
        auto r_parts = split_path(path);
        if (p_parts.size() != r_parts.size()) return false;
        for (size_t i = 0; i < p_parts.size(); ++i) {
            if (is_param(p_parts[i])) params[param_name(p_parts[i])] = r_parts[i];
            else if (p_parts[i] != r_parts[i]) return false;
        }
        return true;
    }
};

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket&& socket, const Router& router) : stream_(std::move(socket)), router_(router) {}
    void run() { read_request(); }
private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    const Router& router_;
    void read_request() { http::async_read(stream_, buffer_, request_, [self = shared_from_this()](beast::error_code ec, size_t) { self->on_read(ec); }); }
    void on_read(beast::error_code ec) {
        if (ec == http::error::end_of_stream) { close(); return; }
        if (ec) return;
        response_ = router_.dispatch(request_);
        response_.keep_alive(request_.keep_alive());
        http::async_write(stream_, response_, [self = shared_from_this()](beast::error_code ec, size_t) { self->on_write(ec); });
    }
    void on_write(beast::error_code ec) { if (ec) return; if (response_.need_eof()) { close(); return; } read_request(); }
    void close() { beast::error_code ec; stream_.socket().shutdown(tcp::socket::shutdown_send, ec); }
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::any_io_executor ex, tcp::endpoint ep, Router r) : acceptor_(ex), router_(std::move(r)) {
        beast::error_code ec;
        acceptor_.open(ep.protocol(), ec);
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        acceptor_.bind(ep, ec);
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
    }
    void run() { do_accept(); }
private:
    void do_accept() {
        acceptor_.async_accept(net::make_strand(acceptor_.get_executor()),
            [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
                if (!ec) std::make_shared<HttpSession>(std::move(socket), self->router_)->run();
                self->do_accept();
            });
    }
    tcp::acceptor acceptor_;
    Router router_;
};

