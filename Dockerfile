FROM debian:12-slim

RUN apt-get update &&\
    apt-get install -y \
        autoconf \
        build-essential \
        curl \
        git \
        libcurl4-gnutls-dev \
        libgcrypt20-dev \
        libgnutls28-dev \
        libjansson4 \
        libjansson-dev \
        libjwt-dev \
        libmicrohttpd-dev \
        libsqlite3-dev \
        libulfius-dev \
        libulfius2.7 \
        sqlite3 \
        tar \
        wget \
        zlib1g-dev

WORKDIR /opt

RUN git clone https://github.com/mbaraa/ctemplate &&\
    cd ctemplate &&\
    make &&\
    make install

# RUN git clone https://github.com/benmcollins/libjwt &&\
#     cd libjwt &&\
#       autoreconf -i &&\
#       ./configure --without-openssl &&\
#     make  &&\
#     make install

RUN wget https://nodejs.org/dist/v20.18.0/node-v20.18.0-linux-x64.tar.xz &&\
    tar xpf node-v20.18.0-linux-x64.tar.xz -C /opt &&\
    ln -s /opt/node-v20.18.0-linux-x64/bin/node /usr/local/bin/node &&\
    ln -s /opt/node-v20.18.0-linux-x64/bin/npx /usr/local/bin/npx &&\
    ln -s /opt/node-v20.18.0-linux-x64/bin/npm /usr/local/bin/npm

WORKDIR /app/danktodo
COPY . .

RUN make

EXPOSE 8080

CMD ["./danktodo"]
