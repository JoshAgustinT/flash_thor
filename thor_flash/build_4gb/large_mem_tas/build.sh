#!/bin/bash
# Build the bigmem host app + every TA size variant into ./out/
# usage: ./build.sh            (needs ../optee-srcs built first)
# No-syscall variant: every TA uses the plain .bss heap (TA_DATA_SIZE), no TA_DATA_GIBIBYTE.
set -e
cd "$(dirname "$0")"
# Thor: native gcc, dev kit + libteec headers from ../optee-srcs (the patched 2GB build)
OPTEE=$(realpath ../optee-srcs/optee)
export CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
export TA_DEV_KIT_DIR=$OPTEE/build/t264/export-ta_arm64
TEEC_EXPORT=$OPTEE/install/t264/usr

# 4GB-2MB OP-TEE region (0xFFE00000): sweep up to it; 4096 (> region) must fail cleanly
HEAP_MBS="1 1024 2048 3072 3584 3840 3968 4000 4032 4064 4096"

rm -rf out && mkdir -p out
make -C host CC=${CROSS_COMPILE}gcc TEEC_EXPORT=$TEEC_EXPORT
cp host/bigmem out/

build_ta() { # $1 = N MiB
	local uuid
	uuid=$(printf "b16e0000-0000-0000-0000-00000000%04x" "$1")
	make -C ta BINARY=$uuid O=$PWD/out/obj-$uuid HEAP_MB=$1 >/dev/null
	cp out/obj-$uuid/$uuid.ta out/
}
for n in $HEAP_MBS; do build_ta "$n"; done
rm -rf out/obj-*
ls out
