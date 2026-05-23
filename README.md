# PhantomScript

**PhantomScript** is a statically-typed, backend-focused programming language that compiles to production-ready C++20. It is designed to eliminate boilerplate when building HTTP microservices, PostgreSQL integrations, and RabbitMQ event-driven systems — while keeping the generated output readable and fully editable.

> Write clean, expressive service definitions. Get working, idiomatic C++ out.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Installation & Building the Compiler](#installation--building-the-compiler)
- [Language Reference](#language-reference)
  - [Primitive Types](#primitive-types)
  - [Data Structures](#data-structures)
  - [Variables](#variables)
  - [Functions](#functions)
  - [Structs](#structs)
  - [Enums](#enums)
  - [Option](#option)
  - [Result](#result)
  - [Control Flow](#control-flow)
  - [Match](#match)
  - [Type Casting](#type-casting)
  - [Contracts and Events](#contracts-and-events)
  - [Database](#database)
  - [Queries](#queries)
  - [Services](#services)
  - [Publish / Consumer](#publish--consumer)
  - [Client](#client)
  - [Import](#import)
- [Code Generation](#code-generation)
  - [Type Mapping](#type-mapping)
  - [Generated Output Structure](#generated-output-structure)
  - [HTTP Runtime](#http-runtime)
  - [PostgreSQL Integration](#postgresql-integration)
  - [RabbitMQ Integration](#rabbitmq-integration)
  - [Docker Support](#docker-support)
- [Full Example](#full-example)
- [Compiler Internals](#compiler-internals)
  - [Lexer](#lexer)
  - [Parser](#parser)
  - [Semantic Analyzer](#semantic-analyzer)
  - [Code Generator](#code-generator)
- [Error Handling](#error-handling)
- [Roadmap](#roadmap)

---

## Overview

PhantomScript was created with one goal: let you define a backend service in a small, structured language and have the compiler produce a complete, compilable C++ project — including HTTP server setup, database connection classes, query functions, event publishing, message consumers, Docker infrastructure, and CMake build files.

The language borrows syntax ideas from Rust, TypeScript, and Go, but is opinionated specifically about backend service architecture. It is not a general-purpose language. Every keyword exists to serve that one purpose.

**What PhantomScript gives you from a single `.ps` file:**

- A `.hpp` header with all structs, enums, contracts, events, database classes, and declarations
- A `.cpp` implementation with all function bodies, query implementations, HTTP service setup, and a `main()`
- A `CMakeLists.txt` configured for Boost.Beast + Boost.Asio + nlohmann/json + libpq
- A `Dockerfile` using a two-stage build
- A `docker-compose.yml` with PostgreSQL and RabbitMQ services pre-configured

---

## Architecture

The compiler is a classic four-stage pipeline:

```
Source (.ps)
    │
    ▼
┌─────────┐
│  Lexer  │  — Tokenizes source into a flat token stream
└─────────┘
    │
    ▼
┌─────────┐
│ Parser  │  — Builds a typed AST using recursive descent
└─────────┘
    │
    ▼
┌──────────────────┐
│ SemanticAnalyzer │  — Type-checks, resolves symbols, detects cycles
└──────────────────┘
    │
    ▼
┌───────────────┐
│ CodeGenerator │  — Walks AST via Visitor, emits C++20
└───────────────┘
    │
    ▼
build/
  ├── service.hpp
  ├── service.cpp
  ├── CMakeLists.txt
  ├── Dockerfile
  └── docker-compose.yml
```

All AST nodes implement an `accept(IAstVisitor&)` method. Both `SemanticAnalyzer` and `CodeGenerator` are `IAstVisitor` implementations, meaning the same AST tree is walked twice — once for validation, once for emission.

---

## Installation & Building the Compiler

### Prerequisites

- C++20-capable compiler (GCC 11+, Clang 13+)
- CMake 3.20+
- Boost (Asio, Beast, System, Thread)
- nlohmann/json
- libpq (PostgreSQL client library)

### Build

```bash
git clone https://github.com/your-org/phantomscript.git
cd phantomscript
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

This produces the `phantom` compiler binary.

### Usage

```bash
phantom service.ps
```

Multiple files:

```bash
phantom auth.ps users.ps orders.ps
```

Output lands in `./build/` relative to the working directory. Each input file produces its own `.hpp` and `.cpp` pair. Shared infrastructure files (`CMakeLists.txt`, `Dockerfile`, `docker-compose.yml`) are written once, reflecting the union of all features used across all compiled files.

---

## Language Reference

### Primitive Types

| PhantomScript | Generated C++   |
|---------------|-----------------|
| `int`         | `int`           |
| `double`      | `double`        |
| `bool`        | `bool`          |
| `string`      | `std::string`   |
| `void`        | `void`          |

### Data Structures

| PhantomScript    | Generated C++                        |
|------------------|--------------------------------------|
| `array[T]`       | `std::vector<T>`                     |
| `list[T]`        | `std::list<T>`                       |
| `map[K, V]`      | `std::unordered_map<K, V>`           |
| `set[T]`         | `std::unordered_set<T>`              |
| `option[T]`      | `std::optional<T>`                   |
| `result[T, E]`   | `Result<T, E>` (custom template)     |

Generic parameters use square brackets: `array[string]`, `map[string, int]`.

### Variables

Variables are declared with either `mut` (mutable) or `const` (immutable), followed by the name, a colon, the type, and an optional initializer.

```ps
mut counter: int = 0;
const pi: double = 3.14159;
mut name: string = "phantom";
mut tags: array[string] = ["backend", "compiled"];
```

Generated C++:

```cpp
int counter = 0;
const double pi = 3.14159;
std::string name = "phantom";
std::vector<std::string> tags = {"backend", "compiled"};
```

### Functions

Functions are declared with `fn`, take typed parameters, and declare their return type with `->`.

```ps
fn add(a: int, b: int) -> int {
    return a + b;
}

fn greet(name: string) -> string {
    return "Hello, " + name;
}
```

Generated C++:

```cpp
int add(const int& a, const int& b) {
    return a + b;
}

std::string greet(const std::string& name) {
    return "Hello, " + name;
}
```

Collection parameters (`array`, `list`, `map`, `set`) are passed by value rather than const reference, allowing the callee to move or modify them.

The `body` keyword marks a parameter as being deserialized from the HTTP request body when the function is used as a route handler:

```ps
fn create_user(body req: CreateUserRequest) -> result[UserResponse, string] {
    // req is automatically deserialized from JSON request body
}
```

### Structs

```ps
struct Point {
    x: double;
    y: double;
}

struct User {
    id: int;
    name: string;
    email: string;
}
```

Generated C++:

```cpp
struct Point {
    double x;
    double y;
};

struct User {
    int id;
    std::string name;
    std::string email;
};
```

Struct instantiation:

```ps
mut p: Point = Point {
    x: 1.0;
    y: 2.5;
};
```

Generated C++:

```cpp
Point p = Point { .x = 1.0, .y = 2.5 };
```

The semantic analyzer checks that all field names exist in the struct definition and that each value's type matches the declared field type.

### Enums

```ps
enum Status {
    PENDING,
    ACTIVE,
    DISABLED
}
```

Generated C++:

```cpp
enum class Status {
    PENDING,
    ACTIVE,
    DISABLED,
};
```

Enum variants are accessed with dot notation in PhantomScript (`Status.ACTIVE`) and translate to `Status::ACTIVE` in C++.

### Option

`option[T]` represents a value that may or may not be present.

```ps
mut age: option[int] = null;
mut nickname: option[string] = "phantom";
```

Generated C++:

```cpp
std::optional<int> age = std::nullopt;
std::optional<std::string> nickname = std::string("phantom");
```

Use `match` to safely unwrap an option (see [Match](#match)).

### Result

`result[T, E]` represents either a successful value of type `T` or an error of type `E`. It is implemented as a custom `Result<T, E>` template class emitted into every generated header.

```ps
fn divide(a: int, b: int) -> result[int, string] {
    if (b == 0) {
        return err("division by zero");
    }
    return ok(a / b);
}
```

Generated C++:

```cpp
Result<int, std::string> divide(const int& a, const int& b) {
    if ((b == 0)) {
        return Result<int, std::string>::Err("division by zero");
    }
    return Result<int, std::string>::Ok((a / b));
}
```

The `Result<T, E>` class provides `is_ok()`, `is_err()`, `unwrap()`, and `unwrap_err()` methods.

### Control Flow

**if / else if / else**

```ps
if (x > 0) {
    print("positive");
} else if (x == 0) {
    print("zero");
} else {
    print("negative");
}
```

**while**

```ps
mut i: int = 0;
while (i < 10) {
    i += 1;
}
```

**for (C-style)**

```ps
for (mut i: int = 0; i < 10; i += 1) {
    print(i);
}
```

**for-in (range iteration)**

```ps
mut items: array[string] = ["a", "b", "c"];
for item in items {
    print(item);
}
```

Generated C++:

```cpp
for (const auto& item : items) {
    std::cout << item << '\n';
}
```

**break / continue**

Both are supported and validated — the semantic analyzer rejects `break` or `continue` used outside of a loop.

### Match

`match` is PhantomScript's primary branching construct for algebraic types. It works on `option`, `result`, and `enum` values.

**Match on option:**

```ps
mut age: option[int] = 25;

match age {
    Some(value) => {
        print(value);
    }
    None => {
        print("no age");
    }
}
```

Generated C++:

```cpp
if (age.has_value()) {
    auto value = *age;
    std::cout << value << '\n';
} else {
    std::cout << "no age" << '\n';
}
```

**Match on result:**

```ps
match divide(10, 2) {
    Ok(number) => {
        print(number);
    }
    Err(error) => {
        print(error);
    }
}
```

Generated C++:

```cpp
if (match_subject.is_ok()) {
    auto number = match_subject.unwrap();
    std::cout << number << '\n';
} else {
    auto error = match_subject.unwrap_err();
    std::cout << error << '\n';
}
```

**Match on enum:**

```ps
match role {
    Role.ADMIN => {
        print("admin panel");
    }
    Role.USER => {
        print("user panel");
    }
}
```

Generated C++:

```cpp
switch (match_subject) {
    case Role::ADMIN: {
        std::cout << "admin panel" << '\n';
        break;
    }
    case Role::USER: {
        std::cout << "user panel" << '\n';
        break;
    }
}
```

### Type Casting

```ps
mut x: int = 42;
mut s: string = x as string;
mut d: double = x as double;
```

Casting to `string` generates `std::to_string(x)`. All other casts generate `static_cast<TargetType>(x)`.

### Contracts and Events

`contract` defines a data transfer object — a struct that gets JSON serialization/deserialization automatically generated for it, used for HTTP request and response bodies.

`event` defines a message payload — identical in structure to a contract, but also gets a `EventTraits_<Name>` struct generated for use with the RabbitMQ consumer runtime.

```ps
contract CreateUserRequest {
    name: string;
    email: string;
    age: int;
}

contract UserResponse {
    id: int;
    email: string;
}

event UserCreated {
    user_id: int;
    email: string;
}
```

Generated C++ (contracts and events both produce):

```cpp
struct CreateUserRequest {
    std::string name;
    std::string email;
    int age;
};

inline void to_json(nlohmann::json& j, const CreateUserRequest& value) {
    j = nlohmann::json{
        {"name", value.name},
        {"email", value.email},
        {"age", value.age}
    };
}

inline void from_json(const nlohmann::json& j, CreateUserRequest& value) {
    j.at("name").get_to(value.name);
    j.at("email").get_to(value.email);
    j.at("age").get_to(value.age);
}
```

Events additionally generate:

```cpp
struct EventTraits_UserCreated {
    static constexpr const char* routing_key = "UserCreated";
    static constexpr const char* exchange = "phantom.events";
};
```

### Database

The `database` declaration configures a PostgreSQL connection. It generates a config struct, a connection-managing class that wraps `PGconn*`, and an `extern` declaration for the global database instance that `main()` will initialize.

```ps
database PrimaryDb {
    engine: "postgres";
    host: "postgres";
    port: 5432;
    name: "main_db";
}
```

Generated C++ (in header):

```cpp
struct PrimaryDbConfig {
    std::string host = "postgres";
    int port = 5432;
    std::string name = "main_db";
    std::string user;
    std::string password;
};

class PrimaryDb {
public:
    explicit PrimaryDb(const PrimaryDbConfig& config);
    ~PrimaryDb();
    PGconn* raw();
private:
    PGconn* conn_ = nullptr;
};

extern std::unique_ptr<PrimaryDb> global_db;
```

The constructor builds a libpq connection string, calls `PQconnectdb`, and throws `std::runtime_error` on failure. The destructor calls `PQfinish`.

### Queries

`query` declarations define parameterized SQL functions. Parameters are mapped to libpq positional placeholders (`$1`, `$2`, ...) automatically. In the SQL body, you reference parameters with `$param_name` and the compiler replaces them with the correct positional index.

```ps
query insert_user(name: string, email: string) -> string {
    sql:
        INSERT INTO users (name, email) VALUES ($name, $email) RETURNING id;
}

query get_user_by_id(id: int) -> string {
    sql:
        SELECT name FROM users WHERE id = $id;
}
```

The semantic analyzer validates that every `$variable` in the SQL body corresponds to a declared parameter. It also validates that the return type references valid types.

Generated C++ converts string parameters directly to `std::string` temporaries, converts numeric types with `std::to_string`, builds the `values[]` array, calls `PQexecParams`, and returns the appropriate value from the result set.

### Services

A `service` is the top-level declaration that wires everything together. It configures the HTTP server, references a database, declares routes (individually or in groups), and lists which events it publishes.

```ps
service UserService {
    server {
        host: "0.0.0.0";
        port: 8081;
    }

    database PrimaryDb;

    route POST "/users" -> create_user;
    route GET  "/health" -> health_check;

    group "/api/v1" {
        route GET  "/users/{id}" -> get_user;
        route PUT  "/users/{id}" -> update_user;
        route DELETE "/users/{id}" -> delete_user;
    }

    publish UserCreated;
}
```

Route handlers referenced here must be declared as `fn` somewhere in the compiled file set. The semantic analyzer verifies every handler name resolves to a known function. Every event listed under `publish` must be a declared `event` type.

Path parameters in routes (e.g. `{id}`) are extracted automatically by the generated router and accessible via `ctx.param("id")` inside the handler.

The code generator produces a `start_service_<Name>(net::io_context&)` function that creates a `Router`, registers all route handlers as lambdas, creates a `Listener`, and calls `listener->run()`. The `main()` function calls all `start_service_*` functions and then runs the thread pool.

### Publish / Consumer

**Publishing** an event from inside a function body:

```ps
publish UserCreated {
    user_id: user.id;
    email: user.email;
};
```

Generated C++:

```cpp
UserCreated event {
    .user_id = user.id,
    .email = user.email,
};
nlohmann::json payload = event;
publisher.publish(
    EventTraits_UserCreated::exchange,
    EventTraits_UserCreated::routing_key,
    payload.dump()
);
```

The semantic analyzer verifies that all field names exist on the event type and that the values have matching types.

**Consuming** an event — binding a handler function to an incoming event type:

```ps
consumer UserCreated -> handle_notification;
```

The semantic analyzer checks that `UserCreated` is a declared `event` and that `handle_notification` is a declared function.

Generated code in `main()`:

```cpp
consumer_runtime.subscribe<EventTraits_UserCreated>(
    [&](const std::string& payload) {
        try {
            auto j = nlohmann::json::parse(payload);
            UserCreated event = j.get<UserCreated>();
            handle_notification(event);
        } catch (const std::exception& e) {
            std::cerr << "Consumer parse error: " << e.what() << '\n';
        }
    }
);
```

### Client

`client` declares an HTTP client for calling an external service. Each `request` entry inside becomes a method on the generated client class.

```ps
client OrderService {
    host: "order-service";
    port: 8082;

    request GET "/orders/{id}" -> get_order(id: int) -> result[string, string];
}
```

The generated class wraps Boost.Beast synchronous HTTP calls. It can be injected into function parameters by name.

### Import

```ps
import "shared_types.ps";
```

Translates to:

```cpp
#include "shared_types.hpp"
```

The `.ps` extension is replaced with `.hpp` automatically.

---

## Code Generation

### Type Mapping

| PhantomScript      | Generated C++                          | Header Required         |
|--------------------|----------------------------------------|-------------------------|
| `int`              | `int`                                  | —                       |
| `double`           | `double`                               | —                       |
| `bool`             | `bool`                                 | —                       |
| `string`           | `std::string`                          | `<string>`              |
| `void`             | `void`                                 | —                       |
| `array[T]`         | `std::vector<T>`                       | `<vector>`              |
| `list[T]`          | `std::list<T>`                         | `<list>`                |
| `map[K, V]`        | `std::unordered_map<K, V>`             | `<unordered_map>`       |
| `set[T]`           | `std::unordered_set<T>`               | `<unordered_set>`       |
| `option[T]`        | `std::optional<T>`                     | `<optional>`            |
| `result[T, E]`     | `Result<T, E>`                         | emitted inline          |
| `struct Foo`       | `struct Foo`                           | emitted inline          |
| `contract Foo`     | `struct Foo` + JSON serializers        | `<nlohmann/json.hpp>`   |
| `event Foo`        | `struct Foo` + JSON + EventTraits      | `<nlohmann/json.hpp>`   |
| `database Foo`     | class wrapping `PGconn*`               | `<libpq-fe.h>`          |
| `enum Foo`         | `enum class Foo`                       | —                       |

### Generated Output Structure

```
build/
├── service.hpp        — All declarations, types, runtime classes
├── service.cpp        — All implementations, main()
├── CMakeLists.txt     — Complete CMake project
├── Dockerfile         — Two-stage Docker build
└── docker-compose.yml — PostgreSQL + RabbitMQ + app
```

### HTTP Runtime

Every file that declares at least one `service` gets the full HTTP runtime emitted into its header. This consists of four classes:

**`RequestContext`** — Wraps the incoming Boost.Beast request. Provides `request()`, `body()`, and `param(name)` for accessing path parameters.

**`Router`** — Stores a list of `(verb, pattern, handler)` triples. `dispatch()` walks the list, matches the path (including `{param}` segments), builds a `RequestContext`, and calls the handler. Returns 404 if no route matches.

**`HttpSession`** — Manages the async read → dispatch → write lifecycle for a single TCP connection using `shared_from_this` and Boost.Beast async operations. Handles keep-alive connections by looping back to `read_request()` after a write completes.

**`Listener`** — Binds a TCP acceptor on the configured address and port. Each accepted socket spawns a new `HttpSession` on a strand.

The threading model: one `io_context`, N worker threads (`std::thread::hardware_concurrency()`), all calling `ioc.run()`. Signal handling (`SIGINT`, `SIGTERM`) calls `ioc.stop()`.

### PostgreSQL Integration

When any `database` or `query` declaration is present:

- `libpq-fe.h` is added to required headers
- `PostgreSQL::PostgreSQL` is added to CMake `target_link_libraries`
- The global `std::unique_ptr<DbName> global_db` is defined in the `.cpp` file
- `main()` constructs the config struct, sets credentials, calls `std::make_unique<DbName>(config)`
- A `CREATE TABLE IF NOT EXISTS` statement is executed after connection for any tables implied by queries

Query functions use `PQexecParams` with text-format parameters and `std::unique_ptr<PGresult, decltype(&PQclear)>` for RAII result management.

### RabbitMQ Integration

When any `publish` statement or `consumer` declaration is present:

- `uses_rabbitmq_` is set in the generator
- A stub `Publisher` class (with a `publish(exchange, routing_key, payload)` method) and `ConsumerRuntime` class are emitted
- `EventTraits_<Name>` structs carry the `routing_key` and `exchange` constants
- `main()` wires all consumers via `consumer_runtime.subscribe<EventTraits_Name>(...)`

The stubs are designed to be replaced with real AMQP client implementations without changing the generated interface.

### Docker Support

When any infrastructure feature (service, database, RabbitMQ) is used, the generator produces:

**Dockerfile** — Two-stage build: builder stage installs all dependencies and compiles, final stage copies only the binary and runtime libraries.

**docker-compose.yml** — Includes:
- A PostgreSQL service (Alpine image) with healthcheck using `pg_isready`
- A RabbitMQ service (management Alpine image) with healthcheck using `rabbitmq-diagnostics ping`
- The application service with `depends_on` using `condition: service_healthy` for both

**CMakeLists.txt** — Sets C++20, finds Boost, nlohmann/json, and optionally PostgreSQL, globs all `.cpp` sources, and links everything.

---

## Full Example

### PhantomScript source (`service.ps`)

```ps
event UserCreated {
    user_id: int;
    email: string;
}

database PrimaryDb {
    engine: "postgres";
    host: "postgres";
    port: 5432;
    name: "main_db";
}

query insert_user(name: string, email: string) -> string {
    sql:
        INSERT INTO users (name, email) VALUES ($name, $email) RETURNING id;
}

service UserService {
    server {
        host: "0.0.0.0";
        port: 8081;
    }
    database PrimaryDb;
    route POST "/users/batch" -> batch_create_users;
    publish UserCreated;
}

fn batch_create_users(prefix: string) -> string {
    mut i: int = 1;
    while (i <= 10) {
        if (i == 5) {
            i += 1;
            continue;
        }
        if (i == 9) {
            break;
        }
        mut generated_name: string = prefix + "_" + (i as string);
        mut generated_email: string = prefix + "_" + (i as string) + "@example.com";
        insert_user(generated_name, generated_email);
        publish UserCreated {
            user_id: i;
            email: generated_email;
        };
        i += 1;
    }
    return "Batch execution finalized.";
}

fn handle_notification(msg: UserCreated) -> void {
    print("Sending email");
}

consumer UserCreated -> handle_notification;
```

### Compile

```bash
phantom service.ps
```

### Run with Docker Compose

```bash
cd build
docker compose up --build
```

The service starts on port `8081`. Test it:

```bash
curl -X POST http://localhost:8081/users/batch \
  -H "Content-Type: application/json" \
  -d '{"prefix": "test"}'
```

---

## Compiler Internals

### Lexer

`Lexer` reads from a `std::istream` character by character using a simple state machine (`LexerState`). States: `START`, `IN_IDENTIFIER`, `IN_NUMBER`, `IN_STRING`, `IN_STRING_ESCAPE`, `IN_COMMENT_SINGLE`, `IN_COMMENT_MULTI`.

- Identifiers and keywords share the `IN_IDENTIFIER` state; on completion the lexeme is looked up in the keyword map
- Numbers track whether a `.` has been seen to distinguish `INT_LITERAL` from `DOUBLE_LITERAL`
- Strings handle `\n`, `\t`, `\r`, `\\`, `\"` escape sequences
- Multi-character operators (`==`, `!=`, `<=`, `>=`, `->`, `=>`, `+=`, `-=`, `*=`, `/=`, `%=`, `&&`, `||`) are handled with a peek-ahead on the next character
- Line and column numbers are tracked for all tokens and surfaced in error messages

### Parser

`Parser` is a hand-written recursive descent parser. The grammar is expressed as a set of `parse_*` methods that mirror the language structure.

Expression precedence (lowest to highest):
1. Assignment (`=`, `+=`, `-=`, `*=`, `/=`, `%=`)
2. Logical OR (`||`)
3. Logical AND (`&&`)
4. Equality (`==`, `!=`)
5. Comparison (`<`, `>`, `<=`, `>=`)
6. Term (`+`, `-`)
7. Factor (`*`, `/`, `%`)
8. Unary (`!`, `-`)
9. Call / member access / index access / cast (postfix chain)
10. Primary (literals, identifiers, struct instantiation, grouped expressions, array literals)

On parse errors a `ParseException` is thrown, caught at the top-level `parse()` loop, and `synchronize()` is called to skip tokens until a safe restart point (semicolon or a known top-level keyword).

### Semantic Analyzer

`SemanticAnalyzer` walks the AST twice conceptually (via the `analyze()` method which sequences multiple passes before the final `program.accept(*this)`):

**Pass 1 — Pre-registration:** Registers all struct names, contract names, event names, enum names, database names, and client names into the environment without resolving field types yet. This allows forward references between types.

**Pass 2 — Field resolution:** Resolves all struct and DTO field types now that all type names are known.

**Pass 3 — Cycle detection:** Runs DFS over the struct dependency graph to find circular struct containment (e.g. `A` contains `B` which contains `A`). Reports an error and suggests breaking the cycle with IDs.

**Pass 4 — Function registration:** Registers all function signatures (including query functions) so that call expressions can be validated in the final pass.

**Final pass — Full traversal:** Visits all function bodies, statements, and expressions. Tracks `current_expression_type_` as a side-channel output from expression visitors, `current_function_return_type_` for return type checking, and `loop_depth_` for break/continue validation.

The `Environment` class manages a stack of scopes (for block scoping of variables) plus flat maps for functions, structs, enums, databases, and clients.

### Code Generator

`CodeGenerator` maintains three output streams:

- `hpp_stream_` — header declarations (types, classes, function signatures, the HTTP runtime)
- `cpp_stream_` — implementation bodies
- `consumer_stream_` — RabbitMQ subscription setup code to be inserted into `main()`

A `required_headers_` set accumulates `#include` directives as features are used. The final `generate()` call assembles the streams in the correct order: `#pragma once`, includes, the `Result<T,E>` template, optional Publisher/ConsumerRuntime stubs, then the hpp content; and for the cpp: the `#include` of the header, global definitions, implementation bodies, and finally `main()`.

The `translate_type()` method converts `TypeNode` trees to C++ type strings recursively, adding entries to `required_headers_` as needed.

---

## Error Handling

### Lexer Errors

- Unknown character: `Syntax Error: Unknown symbol 'X' at line N, column M`
- Unterminated string: `Unterminated string literal at line N`
- Unterminated block comment: `Unterminated multi-line comment at line N`

### Parse Errors

Parse errors are recoverable. The parser prints the error and calls `synchronize()`, which discards tokens until it reaches a semicolon or a known top-level keyword, then continues parsing. This means a single file can report multiple parse errors in one compilation run.

Example: `Expect ';' after expression at line 42`

### Semantic Errors

Semantic errors are fatal for the current file — on the first error, analysis stops and the file is skipped. Examples:

- `Compilation Type Error: cannot bind expression type 'int' to variable 'name' matching target specification 'string'`
- `Undeclared variable context resolution failure: object identifier symbol 'x' is out of scope`
- `Cyclic dependency identified within compilation schema: structural loop containing 'A'. Break the dependency using IDs.`
- `Consumer target binds to an unmapped event block: OrderPlaced`
- `SQL Variable compilation error: $user_id does not exist in query parameters.`

---

## Roadmap

- String interpolation (`"Hello, {name}"`)
- Generic functions (`fn identity[T](x: T) -> T`)
- Interface / trait declarations
- Async/await syntax that maps to coroutine-based Boost.Asio handlers
- Named route groups with middleware
- Proper multi-file linking (shared environment across compiled files)
- Language server protocol (LSP) support for editor integration
- `phantom run` — compile and execute in one step
- Test block syntax (`test "name" { ... }`) compiling to a separate test binary
