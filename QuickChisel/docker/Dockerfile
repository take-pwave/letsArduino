# Defaults if not specified in --build-arg
ARG sbt_version=1.3.10
ARG sbt_home=/usr/local/sbt

FROM alpine:latest as verilator_build
ARG VERILATOR_VERSION=4.016
ENV VERILATOR_ARCHIVE=/tmp/verilator.tgz
ENV VERILATOR_DIRECTORY=/tmp/verilator-${VERILATOR_VERSION}
RUN apk --no-cache add ca-certificates wget make autoconf g++ flex flex-dev bison git python3
# RUN wget -O ${VERILATOR_ARCHIVE} https://www.veripool.org/ftp/verilator-${VERILATOR_VERSION}.tgz
# RUN tar x -C /tmp -zf ${VERILATOR_ARCHIVE}
RUN git clone https://github.com/verilator/verilator.git ${VERILATOR_DIRECTORY}
RUN cd ${VERILATOR_DIRECTORY} && autoconf && \
    ./configure && make && \
    make install -j

FROM openjdk:jre-alpine as sbt_build
ARG sbt_version
ARG sbt_home

# Cleverly placed here for caching reasons
RUN apk add --no-cache --update bash

# Download and extract from archive
RUN apk add --no-cache --update wget
RUN mkdir -pv "$sbt_home"
RUN wget -qO - "https://github.com/sbt/sbt/releases/download/v$sbt_version/sbt-$sbt_version.tgz" >/tmp/sbt.tgz
RUN tar xzf /tmp/sbt.tgz -C "$sbt_home" --strip-components=1


# Make a clean image (i.e., without extra packages)
FROM openjdk:jre-alpine as release
ARG sbt_home

# sbt requires bash at run-time.
RUN apk add --no-cache --update bash

COPY --from=verilator_build /usr/local/bin/verilator* /usr/local/bin/
COPY --from=verilator_build /usr/local/share/verilator /usr/local/share/verilator
COPY --from=verilator_build /usr/local/share/man/man1/verilator* /usr/local/share/man/man1/

COPY --from=sbt_build $sbt_home $sbt_home
RUN ln -sv "$sbt_home"/bin/sbt /usr/bin/

# This triggers a bunch of useful downloads.
RUN sbt sbtVersion
RUN apk add --no-cache --update perl make g++ 
