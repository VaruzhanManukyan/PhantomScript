FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential cmake git libboost-all-dev nlohmann-json3-dev libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . --config Release

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y libboost-system1.74.0 libboost-thread1.74.0 libpq5 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/build/phantom_service .
EXPOSE 8081
CMD ["./phantom_service"]
