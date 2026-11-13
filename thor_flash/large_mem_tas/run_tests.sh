#!/bin/sh
# Runs on the Thor after flashing (root: /dev/tee0 is root-only):
#   sudo sh ~/Desktop/josh/ARMHiT/scripts/FLASH/large_mem_tas/run_tests.sh
# .bss heap sweep 1MB..2GB (no PbMA syscall) in a 2GB OP-TEE region. 2048 must fail cleanly.
D=$(dirname "$0")/out
cp "$D"/*.ta /lib/optee_armtz/
echo "== heap sweep (TA_DATA_SIZE) =="
for mb in 1 2 4 8 12 16 32 64 128 256 512 768 1024 1536 1792 1920 2048; do
	"$D"/bigmem heap $mb
done
echo "== done =="
