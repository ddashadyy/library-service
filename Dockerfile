FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg-dev:latest AS builder

WORKDIR /app

COPY . .

RUN cmake -B build_release \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSERVER_FEATURE_GRPC=ON \
    -DUSERVER_FEATURE_POSTGRESQL=ON \
    -GNinja \
    && cmake --build build_release

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    libev4 \
    libyaml-cpp0.7 \
    libnghttp2-14 \
    libcurl4 \
    libpq5 \
    ca-certificates \
    tzdata \
    libatomic1 \
    libfmt8 \
    libcctz2 \
    && rm -rf /var/lib/apt/lists/*

ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

WORKDIR /app

COPY --from=builder /app/build_release/library-service /app/library-service
COPY --from=builder /app/configs /app/configs

RUN mkdir -p /var/log/library-service

CMD ["/app/library-service", "--config", "/app/configs/static_config.yaml", "--config_vars", "/app/configs/config_vars.yaml"]