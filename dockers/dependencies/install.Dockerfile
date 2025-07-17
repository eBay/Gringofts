# This Dockerfile installs all the external dependencies
# tag: gringofts/compile:v3
FROM gringofts/dependencies:v3
LABEL maintainer="jingyichen@ebay.com"
WORKDIR /usr/external
COPY scripts/installDependencies.sh /usr/external
RUN bash installDependencies.sh
