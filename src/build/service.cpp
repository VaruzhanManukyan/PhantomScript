#include "service.hpp"

Publisher publisher;
ConsumerRuntime consumer_runtime;

std::unique_ptr<PrimaryDb> global_db;

std::string insert_user(std::string name, std::string email) {
    std::string name_text = name;
    std::string email_text = email;
    const char* values[2] = {
        name_text.c_str(),
        email_text.c_str()
    };

    PGresult* raw_result = PQexecParams(global_db->raw(), R"SQL(INSERT INTO users ( name , email ) VALUES ( $1 , $2 ) RETURNING id ;)SQL", 2, nullptr, values, nullptr, nullptr, 0);
    std::unique_ptr<PGresult, decltype(&PQclear)> result(raw_result, &PQclear);

    if (result == nullptr || (PQresultStatus(result.get()) != PGRES_TUPLES_OK && PQresultStatus(result.get()) != PGRES_COMMAND_OK)) {
        throw std::runtime_error(PQerrorMessage(global_db->raw()));
    }

    if (PQntuples(result.get()) > 0) return PQgetvalue(result.get(), 0, 0);
    return "OK";
}

std::string batch_create_users(const std::string& prefix) {
    int i = 1;
    while ((i <= 10)) {
        if ((i == 5)) {
            i += 1;
            continue;
        }
        if ((i == 9)) {
            break;
        }
        std::string generated_name = ((prefix + "_") + std::to_string(i));
        std::string generated_email = (((prefix + "_") + std::to_string(i)) + "@example.com");
        insert_user(generated_name, generated_email);
            UserCreated event{
        .user_id = i,
        .email = generated_email,
    };
    nlohmann::json payload = event;
    publisher.publish(EventTraits_UserCreated::exchange, EventTraits_UserCreated::routing_key, payload.dump());
        i += 1;
    }
    return "Batch execution finalized inside PhantomScript loop engine.";
}

void handle_notification(const UserCreated& msg) {
    std::cout << "Sending email" << '\n';
}

void start_service_UserService(net::io_context& ioc) {
    Router router;

    router.add(http::verb::post, "/users/batch",
        [](const RequestContext& ctx) -> http::response<http::string_body> {
            http::response<http::string_body> res{http::status::ok, ctx.request().version()};
            res.set(http::field::server, "PhantomService");
            try {
                auto body_json = nlohmann::json::parse(ctx.body());
                auto result = batch_create_users(body_json["prefix"].get<std::string>());
                nlohmann::json j = result;
                if (j.is_string()) {
                    res.set(http::field::content_type, "text/plain");
                    res.body() = j.get<std::string>();
                } else {
                    res.set(http::field::content_type, "application/json");
                    res.body() = j.dump();
                }
                res.prepare_payload();
                return res;
            } catch (const std::exception& e) {
                res.result(http::status::bad_request);
                res.body() = "Invalid request body";
                res.prepare_payload();
                return res;
            }
        });

    auto listener = std::make_shared<Listener>(
        net::make_strand(ioc),
        tcp::endpoint{net::ip::make_address("0.0.0.0"), 8081},
        std::move(router)
    );
    listener->run();
}

int main() {
    const std::size_t thread_count = std::max<std::size_t>(1, std::thread::hardware_concurrency());

    net::io_context ioc;
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code& ec, int) { if (!ec) ioc.stop(); });

    consumer_runtime.subscribe<EventTraits_UserCreated>(
        [&](const std::string& payload) {
            try {
                auto j = nlohmann::json::parse(payload);
                UserCreated event = j.get<UserCreated>();
                handle_notification(event);
            } catch (const std::exception& e) {
                std::cerr << "Consumer parse error: " << e.what() << '\n';
            }
        });

    PrimaryDbConfig db_config;
    db_config.host = "postgres";
    db_config.port = 5432;
    db_config.name = "main_db";
    db_config.user = "phantom_developer";
    db_config.password = "phantom_password";
    global_db = std::make_unique<PrimaryDb>(db_config);

    PQexec(global_db->raw(), "CREATE TABLE IF NOT EXISTS users (id SERIAL PRIMARY KEY, name VARCHAR(255), email VARCHAR(255));");

    start_service_UserService(ioc);
    std::cout << "[Phantom Infrastructure] Global runtime started." << std::endl;
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) workers.emplace_back([&ioc] { ioc.run(); });
    for (auto& worker : workers) worker.join();
    return 0;
}
