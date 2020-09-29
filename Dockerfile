FROM ubuntu:latest

# Install system dependencies.
RUN apt-get -y update
RUN apt-get -y install cmake
RUN apt-get -y install wget
RUN apt-get -y install build-essential
RUN apt-get -y install m4
RUN apt-get -y install git

WORKDIR /paxos

# Install local dependencies.
COPY thirdparty/ /paxos/thirdparty/
RUN cd thirdparty/linux && bash gmp.get
RUN cd thirdparty/linux && bash gf2x.get
RUN cd thirdparty/linux && bash ntl.get

# Build libOTe.
COPY libOTe/ /paxos/libOTe/
RUN cd libOTe/cryptoTools/thirdparty/linux/ && bash boost.get
RUN cd libOTe/cryptoTools/thirdparty/linux/ && bash miracl.get
RUN cd libOTe && cmake . -DCMAKE_BUILD_TYPE=Release && make

# Build libSCAPI.
RUN apt-get install -y libssl-dev libgmp3-dev liblog4cpp5-dev zlib1g-dev
RUN cd $HOME && git clone https://github.com/cryptobiu/libscapi.git
RUN apt-get -y install libboost-all-dev
RUN cd $HOME/libscapi && make libs libscapi
RUN rm -r /usr/lib/x86_64-linux-gnu/cmake/

# Build linbox.
RUN apt-get -y install pkg-config
RUN apt-get -y install libopenblas-dev
RUN apt-get -y install libntl-dev
RUN wget https://raw.githubusercontent.com/linbox-team/linbox/master/linbox-auto-install.sh
RUN bash linbox-auto-install.sh --stable=yes --make-flags=-j32 --prefix=/usr

# Build libxxhash.
COPY frontend/ /paxos/frontend/
RUN cd frontend/xxHash/cmake_unofficial && cmake . -DCMAKE_BUILD_TYPE=Release && make

# Compile PSI library.
COPY libPSI/ /paxos/libPSI/
COPY CMakeLists.txt /paxos/
RUN cmake . -DCMAKE_BUILD_TYPE=Release && make -j32