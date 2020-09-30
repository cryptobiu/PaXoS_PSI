FROM ubuntu:latest

#################################################
# Install system dependencies.
#################################################

RUN apt-get -y update
RUN apt-get -y install cmake
RUN apt-get -y install wget
RUN apt-get -y install build-essential
RUN apt-get -y install m4
RUN apt-get -y install git

WORKDIR /paxos

#################################################
# Install local dependencies.
#################################################

COPY thirdparty/ /paxos/thirdparty/
RUN cd thirdparty/linux && bash gmp.get
RUN cd thirdparty/linux && bash gf2x.get
RUN cd thirdparty/linux && bash ntl.get

#################################################
# Build libOTe.
#################################################

COPY libOTe/ /paxos/libOTe/
RUN cd libOTe/cryptoTools/thirdparty/linux/ && bash boost.get
RUN cd libOTe/cryptoTools/thirdparty/linux/ && bash miracl.get
RUN cd libOTe && cmake . -DCMAKE_BUILD_TYPE=Release && make

#################################################
# Build libSCAPI.
#################################################

RUN apt-get install -y libssl-dev libgmp3-dev liblog4cpp5-dev zlib1g-dev
RUN cd $HOME && git clone https://github.com/cryptobiu/libscapi.git
# Use the same commit as the PaXoS experiments.
RUN cd $HOME/libscapi && git checkout 3505b6f4d9ee8382ac3ffe43bf0cef16e27add65
# Remove libscapi's packaged versions of NTL and libOTe, and instead link in our versions.
RUN rm -rf $HOME/libscapi/lib/NTL $HOME/libscapi/lib/libOTe
RUN ln -s /paxos/thirdparty/linux/ntl $HOME/libscapi/lib/NTL
RUN ln -s /paxos/libOTe $HOME/libscapi/lib/libOTe
# Copy libraries to where libscapi's build files expect them.
RUN cp $HOME/libscapi/lib/NTL/src/libntl.a $HOME/libscapi/lib/NTL/src/ntl.a
RUN mkdir -p $HOME/libscapi/lib/libOTe/cryptoTools/thirdparty/miracl/source/
RUN cp -r $HOME/libscapi/lib/libOTe/cryptoTools/thirdparty/linux/miracl/miracl/source/libmiracl.a $HOME/libscapi/lib/libOTe/cryptoTools/thirdparty/miracl/source/
# Build the remaining libraries.
RUN cd $HOME/libscapi && make libs
# Build libscapi, passing in the path to libOTe's version of Boost.
RUN cd $HOME/libscapi && rm makefile && cmake . -DCMAKE_CXX_FLAGS=-I/paxos/libOTe/cryptoTools/thirdparty/linux/boost/ && make scapi -j32

#################################################
# Build linbox.
#################################################

RUN apt-get -y install pkg-config
RUN apt-get -y install libopenblas-dev
RUN wget https://raw.githubusercontent.com/linbox-team/linbox/master/linbox-auto-install.sh
# Install NTL. Since NTL is built with GMP's directory as the prefix, this will install it into /paxos/thirdparty/linux/gmp/.
RUN cd thirdparty/linux/ntl/src && make install
# Fetch and build linbox.
RUN bash linbox-auto-install.sh --stable=yes --with-ntl=/paxos/thirdparty/linux/gmp/ --with-blas-libs="-lopenblas" --make-flags=-j32 --prefix=/usr

#################################################
# Build libxxhash.
#################################################

COPY frontend/ /paxos/frontend/
RUN cd frontend/xxHash/cmake_unofficial && cmake . -DCMAKE_BUILD_TYPE=Release && make

#################################################
# Compile PSI library.
#################################################

COPY libPSI/ /paxos/libPSI/
COPY CMakeLists.txt /paxos/
RUN cmake . -DCMAKE_BUILD_TYPE=Release && make -j32