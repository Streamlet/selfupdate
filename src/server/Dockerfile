FROM golang:1.20-alpine AS build
# go build
ENV BUILD_DIR=/tmp/build
RUN mkdir -p ${BUILD_DIR}
COPY ./ ${BUILD_DIR}
ENV GOPROXY=https://goproxy.cn,direct
RUN cd ${BUILD_DIR} && GOOS=linux GOARCH=amd64 go build -ldflags='-s -w'
# organize directories
ENV INSTALL_DIR=/usr/local/selfupdate
RUN mkdir -p ${INSTALL_DIR}
RUN cp ${BUILD_DIR}/selfupdate_server ${INSTALL_DIR}/


FROM alpine:latest
COPY --from=build /usr/local/selfupdate /usr/local/selfupdate
RUN mkdir -p /etc/selfupdate
WORKDIR /usr/local/selfupdate
CMD [ "./selfupdate_server --port 80 --config /etc/selfupdate" ]
