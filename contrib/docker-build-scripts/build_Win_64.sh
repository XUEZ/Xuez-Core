#!/usr/bin/env bash

## run with -
##  bash contrib/build-docker-scripts/buildWin_64.sh

export LC_ALL=C.UTF-8

export PACKAGES="clang llvm unzip"

# The root dir.
BASE_ROOT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ >/dev/null 2>&1 && pwd )
export BASE_ROOT_DIR
VER=$(head -n 10 configure.ac |  sed -rn "s/define\(_CLIENT_VERSION_(MAJOR|MINOR|REVISION|BUILD),\s([0-9]?).*/\2./p" | tr -d '[:space:]' | sed 's/.$//')
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
export DEBIAN_FRONTEND=noninteractive
export CONTAINER_NAME=xuez_build
export DOCKER_NAME_TAG=ubuntu:18.04
export CCACHE_SIZE=${CCACHE_SIZE:-100M}
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
export DOCKER_PACKAGES=${DOCKER_PACKAGES:-build-essential libtool autotools-dev automake pkg-config bsdmainutils curl ca-certificates ccache python3 rsync git procps nsis g++-mingw-w64-x86-64 g++-mingw-w64-i686 wine-binfmt wine64 file}
export GOAL=${GOAL:-install}
export DIR_QA_ASSETS=${DIR_QA_ASSETS:-${BASE_SCRATCH_DIR}/qa-assets}
export PATH=${BASE_ROOT_DIR}/ci/retry:$PATH
export CI_RETRY_EXE=${CI_RETRY_EXE:-"retry --"}
export BITCOIN_CONFIG="--enable-reduce-exports --without-boost-process --enable-suppress-external-warnings --disable-dependency-tracking --disable-tests --disable-gui-tests --disable-bench --enable-c++17"

# create the docker container

if [[ -z $(docker container ls --all | grep "$CONTAINER_NAME") ]]; then

	# Create folders that are mounted into the docker
	mkdir -p "${CCACHE_DIR}"
	mkdir -p "${PREVIOUS_RELEASES_DIR}"

	export ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
	export LSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/lsan"
	export TSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tsan:halt_on_error=1:log_path=${BASE_SCRATCH_DIR}/sanitizer-output/tsan"
	export UBSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1:report_error_type=1"
	env | grep -E '^(BITCOIN_CONFIG|BASE_|QEMU_|CCACHE_|LC_ALL|BOOST_TEST_RANDOM|DEBIAN_FRONTEND|CONFIG_SHELL|(ASAN|LSAN|TSAN|UBSAN)_OPTIONS|PREVIOUS_RELEASES_DIR)' | tee /tmp/env
	if [[ $BITCOIN_CONFIG = *--with-sanitizers=*address* ]]; then # If ran with (ASan + LSan), Docker needs access to ptrace (https://github.com/google/sanitizers/issues/764)
	  DOCKER_ADMIN="--cap-add SYS_PTRACE"
	fi

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
  docker cp $CONTAINER_NAME:$BASE_ROOT_DIR/xuez-$VER-win64-setup.exe xuez-v$VER-win64-setup.exe
  DOCKER_EXEC rm -rf \$\(ls $BASE_ROOT_DIR/*setup*exe\)
}

DOCKER_EXEC echo "Free disk space:"
DOCKER_EXEC df -h

${CI_RETRY_EXE} DOCKER_EXEC apt-get update
${CI_RETRY_EXE} DOCKER_EXEC apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $DOCKER_PACKAGES

DOCKER_EXEC update-alternatives --set x86_64-w64-mingw32-g++ \$\(which x86_64-w64-mingw32-g++-posix\)

echo "Create/syncing $BASE_ROOT_DIR"
DOCKER_EXEC rsync -a /ro_base/ $BASE_ROOT_DIR

if [[ ! -d "${DEPENDS_DIR}/SDKs ${DEPENDS_DIR}/sdk-sources" ]]; then
  DOCKER_EXEC mkdir -p ${DEPENDS_DIR}/SDKs ${DEPENDS_DIR}/sdk-sources
fi

DEP_OPTS=""
BUILDHOST="x86_64-w64-mingw32"

#i686-pc-linux-gnu for Linux 32 bit
#x86_64-pc-linux-gnu for x86 Linux
#i686-w64-mingw32 for Win32
#x86_64-w64-mingw32 for Win64
#x86_64-apple-darwin16 for macOS
#arm-linux-gnueabihf for Linux ARM 32 bit
#aarch64-linux-gnu for Linux ARM 64 bit
#armv7a-linux-android for Android ARM 32 bit
#aarch64-linux-android for Android ARM 64 bit
#i686-linux-android for Android x86 32 bit
#x86_64-linux-android for Android x86 64 bit

MAKE_COMMAND="make $MAKEJOBS -C depends"
DOCKER_EXEC "$MAKE_COMMAND" HOST=$BUILDHOST

if [ "$1" == "clean" ]; then
  echo "Cleaning build dir $BASE_ROOT_DIR..."
  DOCKER_EXEC "[[ -f Makefile ]] && make distclean || make clean"
fi
DOCKER_EXEC "[[ ! -f configure ]] && ./autogen.sh"
DOCKER_EXEC "[[ -f configure ]] && ./configure $BITCOIN_CONFIG --prefix=$DEPENDS_DIR/$BUILDHOST"
DOCKER_EXEC "make $MAKEJOBS && make deploy" && COPYBINS
