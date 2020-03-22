# This Dockerfile downloads all the external dependencies
# tag: gringofts/dependencies:v1
FROM ubuntu:16.04
LABEL maintainer="jqi1@ebay.com"
WORKDIR /usr/external
COPY scripts/downloadDependencies.sh /usr/external
RUN bash downloadDependencies.sh