ARG IOTA_BLOCKCHAIN_PLUGIN_VERSION=1.1.0

FROM --platform=linux/amd64 ubuntu:24.10
ARG IOTA_BLOCKCHAIN_PLUGIN_VERSION

# Install necessary build dependencies
RUN apt update && apt install -y \
    build-essential \
    cmake \
    libssl-dev \
    libwebsockets-dev \
    libcurl4-openssl-dev \
    libcbor-dev \
    python3 \
    python3-dev \
    python3-pip \
    wget \
    libmosquitto-dev \
    mosquitto \
    mosquitto-dev

RUN python3.12 -m pip install iota-sdk==1.1.4 --break-system-packages

# Build the mosquitto-iota-blockchain-plugin from source
RUN wget https://github.com/iotinga/mosquitto-iota-blockchain-plugin/archive/refs/tags/v${IOTA_BLOCKCHAIN_PLUGIN_VERSION}.tar.gz -O mosquitto-iota-blockchain-plugin-${IOTA_BLOCKCHAIN_PLUGIN_VERSION}.tar.gz
RUN tar xzvf mosquitto-iota-blockchain-plugin-${IOTA_BLOCKCHAIN_PLUGIN_VERSION}.tar.gz
RUN cd mosquitto-iota-blockchain-plugin-${IOTA_BLOCKCHAIN_PLUGIN_VERSION} \
    && mkdir build && cd build \
    && cmake .. \
    && make \
    && make install \
    && cd ../.. \
    && rm -rf mosquitto-iota-blockchain-plugin-${IOTA_BLOCKCHAIN_PLUGIN_VERSION} mosquitto-iota-blockchain-plugin-${IOTA_BLOCKCHAIN_PLUGIN_VERSION}.tar.gz


# Create mosquitto directories
RUN mkdir -p /mosquitto/config /mosquitto/data /mosquitto/log

# Configure mosquitto to use the plugin
COPY mosquitto-default.conf /mosquitto/config/mosquitto.conf

# Set up entry point
VOLUME ["/mosquitto/data", "/mosquitto/log"]
EXPOSE 1883
CMD ["mosquitto", "-c", "/mosquitto/config/mosquitto.conf"]