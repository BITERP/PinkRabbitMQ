FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential cmake libssl-dev libevent-dev uuid-dev\
    python3 python3-pip

RUN python3 -m pip install pytest requests cryptography PyYAML

COPY build.sh /build.sh

ENTRYPOINT ["/build.sh"]
CMD ["Debug"]
