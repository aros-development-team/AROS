FROM amigadev/docker-base:latest

ENV BUILD_EXT "linux-x86_64"
ENV BUILD_ICONSET "default"
ENV BUILD_BINUTILSVER "2.32"
ENV BUILD_GCCVER "9.1.0"
ENV BUILD_DIR "/tmp/build"

WORKDIR /work

COPY ./ /work/

# Retrieve contribs: will be available in /work/contrib
# note: dir name MUST be "contrib"
RUN git clone https://github.com/AmigaPorts/AROS-contrib.git contrib \
    && mkdir -p $BUILD_DIR \
    && cd $BUILD_DIR \
    && /work/configure --target=$BUILD_EXT $BUILD_CONFIGUREEXTRAS --enable-ccache \
      --with-iconset=$BUILD_ICONSET --enable-build-type=nightly \
      --with-serial-debug --with-binutils-version=$BUILD_BINUTILSVER \
      --with-gcc-version=$BUILD_GCCVER \
    && make -j$(getconf _NPROCESSORS_ONLN) crosstools-toolchain \
    && make -j$(getconf _NPROCESSORS_ONLN) default-x11keymaptable \
    && make -j$(getconf _NPROCESSORS_ONLN) contrib \
    && make distfiles
