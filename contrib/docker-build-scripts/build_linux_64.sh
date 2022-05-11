#!/usr/bin/env bash

## run with -
##  bash contrib/build-android/buildlinux_64.sh

export LC_ALL=C.UTF-8

export PACKAGES="clang llvm unzip"

# The root dir.
BASE_ROOT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ >/dev/null 2>&1 && pwd )
export BASE_ROOT_DIR
VER=$(head -n 20 $BASE_ROOT_DIR/configure.ac | grep -E 'define\(_CLIENT_VERSION_(MAJOR|MINOR|REVISION|BUILD)' |  grep -ohE '[0-9]' | tr -d '[:space:]')
export VER

echo "Setting specific values in env"
if [ -n "${FILE_ENV}" ]; then
  set -o errexit;
  source "${FILE_ENV}"
fi

echo "Fallback to default values in env (if not yet set)"
export MAKEJOBS=${MAKEJOBS:--j4}
export BASE_SCRATCH_DIR=${BASE_SCRATCH_DIR:-$BASE_ROOT_DIR/ci/scratch}
export HOST=${HOST:-$("$BASE_ROOT_DIR/depends/config.guess")}
export CONTAINER_NAME=xuez_build
export DOCKER_NAME_TAG=ubuntu:20.04
export DEBIAN_FRONTEND=noninteractive
export CCACHE_SIZE=${CCACHE_SIZE:-500M}
export CCACHE_TEMPDIR=${CCACHE_TEMPDIR:-/tmp/.ccache-temp}
export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
# The cache dir.
# This folder exists on the ci host and ci guest. Changes are propagated back and forth.
export CCACHE_DIR=${CCACHE_DIR:-$BASE_SCRATCH_DIR/.ccache}
# The depends dir.
# This folder exists on the ci host and ci guest. Changes are propagated back and forth.
export DEPENDS_DIR=${DEPENDS_DIR:-$BASE_ROOT_DIR/depends}
# Folder where the build result is put (bin and lib).
export BASE_OUTDIR=${BASE_OUTDIR:-$BASE_SCRATCH_DIR/out/$HOST}
# Folder where the build is done (dist and out-of-tree build).
export BASE_BUILD_DIR=${BASE_BUILD_DIR:-$BASE_SCRATCH_DIR/build}
export PREVIOUS_RELEASES_DIR=${PREVIOUS_RELEASES_DIR:-$BASE_ROOT_DIR/releases/$HOST}
export SDK_URL=${SDK_URL:-https://bitcoincore.org/depends-sources/sdks}
export DOCKER_PACKAGES=${DOCKER_PACKAGES:-build-essential libtool autotools-dev automake pkg-config bsdmainutils curl ca-certificates ccache python3 rsync git procps bison}
export GOAL=${GOAL:-install}
export DIR_QA_ASSETS=${DIR_QA_ASSETS:-${BASE_SCRATCH_DIR}/qa-assets}
export PATH=${BASE_ROOT_DIR}/ci/retry:$PATH
export CI_RETRY_EXE=${CI_RETRY_EXE:-"retry --"}
export BITCOIN_CONFIG="--disable-ccache --disable-tests --disable-gui-tests --disable-bench --disable-dependency-tracking --enable-suppress-external-warnings --enable-reduce-exports --enable-c++17"

# create the docker container

if [[ -z $(docker container ls --all | grep "$CONTAINER_NAME") ]]; then

	# Create folders that are mounted into the docker
	mkdir -p "${CCACHE_DIR}"
	mkdir -p "${PREVIOUS_RELEASES_DIR}"

	env | grep -E '^(BITCOIN_CONFIG|BASE_|QEMU_|CCACHE_|LC_ALL|BOOST_TEST_RANDOM|DEBIAN_FRONTEND|CONFIG_SHELL|PREVIOUS_RELEASES_DIR)' | tee /tmp/env

	echo "Creating $DOCKER_NAME_TAG container to run in"
	${CI_RETRY_EXE} docker pull "$DOCKER_NAME_TAG"

	docker run -idt \
	--mount type=bind,src=$BASE_ROOT_DIR,dst=/ro_base,readonly \
	--mount type=bind,src=$CCACHE_DIR,dst=$CCACHE_DIR \
	--mount type=bind,src=$DEPENDS_DIR,dst=$DEPENDS_DIR \
	--mount type=bind,src=$PREVIOUS_RELEASES_DIR,dst=$PREVIOUS_RELEASES_DIR \
	-w $BASE_ROOT_DIR \
	--env-file /tmp/env \
	--name $CONTAINER_NAME \
	$DOCKER_NAME_TAG
fi

export P_CI_DIR="$PWD"
DOCKER_EXEC () {
  docker exec $CONTAINER_NAME bash -c "export PATH=$BASE_SCRATCH_DIR/bins/:\$PATH && cd $P_CI_DIR && $*"
}

export -f DOCKER_EXEC

COPYBINS () {
  echo "Stripping built binary files..."
  DOCKER_EXEC "strip src/qt/xuez-qt && cp src/qt/xuez-qt . && tar czf $BUILDHOST-xuez-gui-$VER.tgz xuez-qt && rm xuez-qt"
  DOCKER_EXEC "strip src/xuezd src/xuez-cli src/xuez-tx src/xuez-wallet && mv src/xuezd src/xuez-cli src/xuez-tx src/xuez-wallet . "
  echo "Compressing built binary files..."
  DOCKER_EXEC "tar czf $BUILDHOST-xuez-cli-$VER.tgz xuezd xuez-cli xuez-tx xuez-wallet && rm xuezd xuez-cli xuez-tx xuez-wallet"
  echo "Copying built binary files from docker container..."
  docker cp $CONTAINER_NAME:$BASE_ROOT_DIR/$BUILDHOST-xuez-gui-$VER.tgz .
  docker cp $CONTAINER_NAME:$BASE_ROOT_DIR/$BUILDHOST-xuez-cli-$VER.tgz .
  DOCKER_EXEC rm -rf \$\(ls $BASE_ROOT_DIR/*xuez*tgz\)
}

MKDEBS () {
  ARCH=$(dpkg --print-architecture)
  echo "Building DEB package for $ARCH..."
  DEBPATH="xuez-wallet_${VER}_${ARCH}"
  mkdir -p $DEBPATH/DEBIAN $DEBPATH/usr/bin $DEBPATH/usr/share/pixmaps $DEBPATH/usr/share/applications
  cp contrib/docker-build-scripts/deb/control $DEBPATH/DEBIAN/
  cp contrib/docker-build-scripts/deb/xuez.desktop $DEBPATH/usr/share/applications/
  cp contrib/docker-build-scripts/deb/xuez.xpm $DEBPATH/usr/share/pixmaps
  tar xzf $BUILDHOST-xuez-gui-$VER.tgz -C $DEBPATH/usr/bin/
  sed -i 's/_package_/xuez-wallet/g' $DEBPATH/DEBIAN/control
  sed -i "s/_arch_/${ARCH}/g" $DEBPATH/DEBIAN/control
  echo -e "Description: Xuez Core Wallet - https://xuezcoin.com" >> $DEBPATH/DEBIAN/control
  echo -e "Depends: libc6 (>= 2.27), libfontconfig1 (>= 2.12.6), libfreetype6 (>= 2.6), libgcc-s1 (>= 3.4), libstdc++6 (>= 7), libxcb-icccm4 (>= 0.4.1), libxcb-image0 (>= 0.2.1), libxcb-keysyms1 (>= 0.4.0), libxcb-randr0 (>= 1.3), libxcb-render-util0, libxcb-render0, libxcb-shape0, libxcb-shm0 (>= 1.10), libxcb-sync1, libxcb-xfixes0, libxcb-xinerama0, libxcb-xkb1, libxcb1 (>= 1.8), libxkbcommon-x11-0 (>= 0.5.0), libxkbcommon0 (>= 0.5.0)" >> $DEBPATH/DEBIAN/control
  dpkg-deb --build --root-owner-group $DEBPATH
  rm -rf $DEBPATH
}

MKCLIDEBS () {
  ARCH=$(dpkg --print-architecture)
  echo "Building CLI DEB package for $ARCH..."
  DEBPATH="xuez-cli_${VER}_${ARCH}"
  mkdir -p $DEBPATH/DEBIAN $DEBPATH/usr/bin
  cp contrib/docker-build-scripts/deb/control $DEBPATH/DEBIAN/
  tar xzf $BUILDHOST-xuez-cli-$VER.tgz -C $DEBPATH/usr/bin/
  sed -i 's/_package_/xuez-cli/g' $DEBPATH/DEBIAN/control
  sed -i "s/_arch_/${ARCH}/g" $DEBPATH/DEBIAN/control
  echo -e "Description: Xuez Core CLI tools - https://xuezcoin.com" >> $DEBPATH/DEBIAN/control
  echo -e "Depends: libc6 (>= 2.29), libgcc-s1 (>= 3.0), libstdc++6 (>= 9)" >> $DEBPATH/DEBIAN/control
  dpkg-deb --build --root-owner-group $DEBPATH
  rm -rf $DEBPATH
}

DOCKER_EXEC echo "Free disk space:"
DOCKER_EXEC df -h

${CI_RETRY_EXE} DOCKER_EXEC apt-get update
${CI_RETRY_EXE} DOCKER_EXEC apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $DOCKER_PACKAGES

echo "Create/syncing $BASE_ROOT_DIR"
DOCKER_EXEC rsync -a /ro_base/ $BASE_ROOT_DIR

DEP_OPTS=""
BUILDHOST="x86_64-pc-linux-gnu"

MAKE_COMMAND="make $MAKEJOBS -C depends"
DOCKER_EXEC "$MAKE_COMMAND" HOST=$BUILDHOST

if [ "$1" == "clean" ]; then
  echo "Cleaning build dir $BASE_ROOT_DIR..."
  DOCKER_EXEC "[[ -f Makefile ]] && make distclean || make clean"
fi
DOCKER_EXEC "[[ ! -f configure ]] && ./autogen.sh"
DOCKER_EXEC "[[ -f configure ]] && ./configure $BITCOIN_CONFIG --prefix=$DEPENDS_DIR/$BUILDHOST"
DOCKER_EXEC "make $MAKEJOBS" && COPYBINS && MKDEBS && MKCLIDEBS
