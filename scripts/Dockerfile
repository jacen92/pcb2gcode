# Use resin image to "cross-compile" from x86 or ARM to ARM
# FROM resin/armv7hf-debian
FROM ubuntu:latest

ENV DEBIAN_FRONTEND noninteractive
ENV BUILD_DIR=/tmp/build
ENV PKG_DIR=/tmp/packages
RUN apt-get update \
&& apt-get install -y apt-utils nano htop \
                      python python-pip git \
                      build-essential automake autoconf autoconf-archive libtool \
                      libgtkmm-2.4-dev gerbv \
&& rm -rf /var/lib/apt/lists/*

# install boost from source (specific version is needed)
RUN curl -L -v "http://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz" | tar -C /tmp/ -xz && \
    cd /tmp/boost_1_58_0/ && ./bootstrap.sh --with-libraries=program_options && ./b2 variant=release && ./b2 install && ldconfig && \
    rm -rf /tmp/boost*

# ==============================================================================
# clone ( and first make )
RUN mkdir ${PKG_DIR} && mkdir ${BUILD_DIR} && cd ${BUILD_DIR} && git clone https://github.com/jacen92/pcb2gcode.git && \
    cd pcb2gcode && git checkout scripts && git config --global user.email root@exemple.net \
    && autoreconf -fvi && ./configure && make -j4

WORKDIR $BUILD_DIR
VOLUME ["${PKG_DIR}"]
CMD ["/bin/bash"]
