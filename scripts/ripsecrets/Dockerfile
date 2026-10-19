FROM ubuntu:24.04

ARG RIPSECRETS_VERSION=0.1.11
ARG RIPSECRETS_SHA256=9daf017dfdea242a58f672450a2526f0406211afe9e872c740adc49b42feccf5

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates curl \
    && rm -rf /var/lib/apt/lists/* \
    && curl -fsSL -o /tmp/ripsecrets.tar.gz \
        "https://github.com/sirwart/ripsecrets/releases/download/v${RIPSECRETS_VERSION}/ripsecrets-${RIPSECRETS_VERSION}-x86_64-unknown-linux-gnu.tar.gz" \
    && echo "${RIPSECRETS_SHA256}  /tmp/ripsecrets.tar.gz" | sha256sum -c - \
    && tar --warning=no-unknown-keyword -xzf /tmp/ripsecrets.tar.gz -C /tmp \
    && install -m 0755 "/tmp/ripsecrets-${RIPSECRETS_VERSION}-x86_64-unknown-linux-gnu/ripsecrets" /usr/local/bin/ripsecrets \
    && rm -rf /tmp/ripsecrets*

ENTRYPOINT ["ripsecrets"]
