FROM reg.choncholas.com/research/mpc3-dev:latest

RUN apt-get update && apt-get install -y sudo libgmp-dev libssl-dev libboost-all-dev libssl-dev xxd


ADD . / app/mpc3

WORKDIR /app
#RUN git clone --recurse-submodules https://github.com/james-choncholas/mpc3

RUN cd mpc3 && \
    mkdir -p build && \
    cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build ./build
