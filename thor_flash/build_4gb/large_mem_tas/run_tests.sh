#!/bin/sh
# Runs on the Thor after flashing (root: /dev/tee0 is root-only):
#   sudo sh <this dir>/run_tests.sh
# .bss heap sweep 1MB..4GB in a 4GB-2MB (0xFFE00000) OP-TEE region. 4096 must fail cleanly; top sizes near 4GB may fail (core + other TAs share the region).
D=$(dirname "$0")/out
cp "$D"/*.ta /lib/optee_armtz/
echo "== heap sweep (TA_DATA_SIZE) =="
for mb in 1 1024 2048 3072 3584 3840 3968 4000 4032 4064 4096; do
	"$D"/bigmem heap $mb
done
echo "== done =="
