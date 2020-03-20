# This Dockerfile installs all the external dependencies
# tag: gringofts:compile-env
FROM gringofts:dependencies
LABEL maintainer="jqi1@ebay.com"
WORKDIR /usr/external
COPY scripts/installDependencies.sh /usr/external
RUN bash installDependencies.sh
ENV PATH "$PATH:/usr/local/go/bin"