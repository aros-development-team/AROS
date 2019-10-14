FROM amigadev/docker-base:latest

ENV BUILD_EXT "linux-x86_64"
# ENV BUILD_CONFIGUREEXTRAS "--enable-debug"
ENV BUILD_ICONSET "default"
ENV BUILD_BINUTILSVER "2.32"
ENV BUILD_GCCVER "9.1.0"
# ENV BUILD_TOOLS_PATH "tools"
# ENV BUILD_PORTS_PATH "ports"
ENV BUILD_DIR "/tmp/build"

# Compile AROS here (won't compile in sources dir)
RUN mkdir -p $BUILD_DIR
WORKDIR /work

COPY ./ /work/

# Retrieve contribs: will be available in /work/contrib
# note: dir name MUST be "contrib"
RUN git clone https://github.com/AmigaPorts/AROS-contrib.git contrib

RUN cd $BUILD_DIR
RUN /work/configure --target=$BUILD_EXT $BUILD_CONFIGUREEXTRAS --enable-ccache --with-iconset=$BUILD_ICONSET --enable-build-type=nightly --with-serial-debug --with-binutils-version=$BUILD_BINUTILSVER --with-gcc-version=$BUILD_GCCVER
# --with-aros-toolchain-install=$BUILD_TOOLS_PATH
# --with-portssources=/externalsources

RUN make -j8 default-x11keymaptable
# RUN make -j8 contrib
RUN make distfiles
