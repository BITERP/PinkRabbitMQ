FROM ubuntu:22.04

RUN apt update && apt install -y build-essential cmake libssl-dev libevent-dev uuid-dev\
    python3 python3-pip

RUN python3 -m pip install pytest requests cryptography PyYAML

COPY build.sh /build.sh

ENTRYPOINT /bin/bash /build.sh