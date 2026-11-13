#!/usr/bin/env python3
"""Check that a built flash package contains the files we meant to flash.

usage: python3 -I check_package.py <package dir> <tos.img> <hafnium.fip> <BCT misc .dts>
  <package dir> = Linux_for_Tegra/unified_flash/out (or out_<set>)

Checks:
  - the hafnium fip bytes are inside flash-images/hafnium_t264_sigheader.fip.encrypt
  - the tos.img bytes are inside the lz4-compressed flash-images/sp_t264_aligned_blob_w_bin_sigheader.fip.encrypt
  - prints the TZDRAM size + option B lines of the BCT dts the package was built from (Linux_for_Tegra/bootloader copy)
  - A_MB1_BCT, A_secure-hv, A_sec-partition are all on QSPI in FileToFlash.txt
Needs: lz4 (installed by l4t_flash_prerequisites.sh).
"""
import os
import subprocess
import sys

pkg, tos, haf, bct = sys.argv[1:5]
img = os.path.join(pkg, 'bsp_images/flash_workspace/flash-images')
ok = True


def check(label, cond):
    global ok
    ok &= cond
    print(('OK   ' if cond else 'FAIL ') + label)


h = open(os.path.join(img, 'hafnium_t264_sigheader.fip.encrypt'), 'rb').read()
check('hafnium fip inside package: ' + haf, open(haf, 'rb').read() in h)

sp = open(os.path.join(img, 'sp_t264_aligned_blob_w_bin_sigheader.fip.encrypt'), 'rb').read()
i = sp.find(bytes.fromhex('04224d18'))  # lz4 frame magic
d = subprocess.run(['lz4', '-dc'], input=sp[i:], capture_output=True).stdout if i >= 0 else b''
check('tos.img inside package (lz4 at %d): %s' % (i, tos), len(d) > 0 and open(tos, 'rb').read() in d)

print('BCT dts used (%s):' % bct)
for line in open(bct):
    if 'size = <' in line or 'staged' in line:
        print('     ' + line.strip())

parts = {}
for line in open(os.path.join(img, 'FileToFlash.txt')):
    f = line.split()
    if len(f) > 1:
        parts[f[1]] = f[0]
for p in ('A_MB1_BCT', 'A_secure-hv', 'A_sec-partition'):
    check('%s on QSPI (%s)' % (p, parts.get(p)), parts.get(p, '').endswith('.spi'))

print('ALL OK' if ok else 'SOMETHING IS WRONG - do not flash')
sys.exit(0 if ok else 1)
