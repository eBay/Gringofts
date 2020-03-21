# This Dockerfile installs all the external dependencies
# tag: gringofts/compile:v1
FROM gringofts/dependencies:v1
LABEL maintainer="jqi1@ebay.com"
WORKDIR /usr/external
COPY scripts/installDependencies.sh /usr/external
RUN bash installDependencies.sh
