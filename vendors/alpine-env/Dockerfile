FROM alpine:3.8

LABEL maintainer="lanyuhang@baidu.com"

# RUN echo "http://mirrors.aliyun.com/alpine/v3.8/main" > /etc/apk/repositories \
#   && echo "http://mirrors.aliyun.com/alpine/v3.8/community" >> /etc/apk/repositories

RUN apk add --no-cache bash curl tar xz vim alpine-sdk openjdk8 curl-dev zlib-dev libexecinfo-dev linux-headers

ENV JAVA_HOME=/usr/lib/jvm/java-1.8-openjdk

ADD apache-maven-3.6.3-bin.tar.gz /usr/local

ADD cmake-3.17.1-Linux-musl.tar.gz /usr/local

ENV PATH=/usr/local/cmake-3.17.1-Linux-musl/bin:/usr/local/apache-maven-3.6.3/bin:$PATH

CMD /bin/bash