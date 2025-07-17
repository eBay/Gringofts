# This Dockerfile downloads all the external dependencies
# tag: gringofts/dependencies:v3
FROM ubuntu:22.04
LABEL maintainer="jingyichen@ebay.com"
WORKDIR /usr/external
COPY scripts/downloadDependencies.sh /usr/external
RUN bash downloadDependencies.sh