FROM ubuntu:22.04

RUN apt update && apt upgrade
RUN apt install build-essential libssl-dev uuid-dev libsecp256k1-dev nodejs npm -y

COPY . .

RUN cd scripts && npm install && cd ..

RUN make
