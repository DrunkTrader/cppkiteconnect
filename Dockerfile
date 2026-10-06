FROM ubuntu:24.04@sha256:534baea6a22c03a63003dbc8dbe78fe34bc0d7e595d9a9dc9834884ff530eb55
RUN apt-get update && \
    apt-get install -y --no-install-recommends cmake g++ make libssl-dev \
        libboost1.83-dev ca-certificates && \
    rm -rf /var/lib/apt/lists/*
# Initialize recorded submodules on the host before docker build. The container
# consumes the supplied source; it does not clone/update dependencies in it.
WORKDIR /app
COPY . /app
RUN cmake -S . -B /opt/kitepp-build -DBUILD_EXAMPLES=ON && \
    cmake --build /opt/kitepp-build --parallel 2
ENTRYPOINT ["sh", "-c", "/opt/kitepp-build/example${EXAMPLE_NUMBER:-1}"]
