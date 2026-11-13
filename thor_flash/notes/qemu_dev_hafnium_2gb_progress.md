# Large TA memory, 2GB OP-TEE region, Hafnium (SPMC at S-EL2) — the model for the FIRST Thor flash

Goal: the exact config we will flash first on the Jetson Thor (t264, R39.2.1), rehearsed on QEMU.
= ../qemu_dev_hafnium (no-syscall large TA memory, OP-TEE as FF-A SP under Hafnium) with:
  - the OP-TEE region shrunk 34GB -> 2GB (S = 0x80000000), and
  - the Thor's OP-TEE knobs: 14 threads, 38-bit VA (tegra default), ASLR on (tegra forces it),
    40 L2 tables/TA thread, prealloc L3, 2MB core heap, 8MB core VA.
If this passes on QEMU, the Thor flash differs only in who provides the region (BCT/MB2 instead of
QEMU/TF-A BL2). Part 2 below is the Thor procedure, written to be followed in a fresh session.

# Part 1 — QEMU rehearsal (DONE 2026-11-12, all pass)

## Files here
- pinned_manifest.xml ......... repo revisions (same as qemu_dev / qemu_dev_hafnium)
- hafnium_optee_os.diff ....... optee_os, 7 files +38/-23 (S2M + 64-bit fixes + bget). SAME FILE THE THOR USES.
- hafnium_2gb_build.diff ...... build/: qemu_v8.mk knobs + 3 manifests at 2GB
- hafnium_tfa.diff ............ trusted-firmware-a, QEMU-only (BL2 64-bit load-address, 38-bit map)
- hafnium_2gb_qemu.diff ....... qemu virt.c: 2GB secure-only RAM @ 0x3000000000 (stands in for the carve-out)
- hosttools/poetry ............ stand-in for Poetry (TF-A sptool step), see ../qemu_dev_hafnium/progress.md
- large_mem_tas/ .............. bigmem host app + TA (plain .bss heap = TA_DATA_SIZE), sweep 1..2048 MB
- run_guest.exp ............... headless boot + run a command, logs/<name>_{nw,sw}.log

## Rebuild from scratch (machine wiped)
> mkdir -p ~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium_2gb/optee && cd ~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium_2gb/optee
> repo init -u https://github.com/OP-TEE/manifest.git -m qemu_v8.xml
> repo sync -j14 -m ~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium_2gb/pinned_manifest.xml
> git -C optee_os apply ../../hafnium_optee_os.diff
> git -C trusted-firmware-a apply ../../hafnium_tfa.diff
> git -C qemu apply ../../hafnium_2gb_qemu.diff
> git -C build apply ../../hafnium_2gb_build.diff
> cd build && make toolchains -j2
> curl -L https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.8/LLVM-20.1.8-Linux-ARM64.tar.xz | tar -xJ -C ../toolchains && mv ../toolchains/LLVM-20.1.8-Linux-ARM64 ../toolchains/clang-20.1.8
> cd ~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium_2gb && python3 -m pip install --target pylib 'fdt>=0.3.0,<0.4'
> cd optee/build && PATH=~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium_2gb/hosttools:$PATH make SPMC_AT_EL=2 CLANG_BUILD_VER=20.1.8 -j14
> cd ../.. && ./large_mem_tas/build.sh
(This run: toolchains were cp -a'd from ../qemu_dev_hafnium/optee/toolchains = the same downloads.)

## Log
- 2026-11-12 — fresh repo sync from pinned_manifest.xml + the diffs above (also proves the
  "wipe and rebuild" path works). Build rc=0 (logs/build.log).
  Effective config (optee_os/out/arm/conf.mk): CFG_NUM_THREADS=14 CFG_NUM_TA_L2_TBLS=40
  CFG_LPAE_ADDR_SPACE_BITS=38 CFG_CORE_ASLR=y CFG_CORE_PHYS_RELOCATABLE=y CFG_CORE_SEL2_SPMC=y
  CFG_CORE_PREALLOC_EL0_TBLS=y CFG_CORE_HEAP_SIZE=2097152 CFG_TEE_RAM_VA_SIZE=0x00800000
  CFG_TZDRAM_START=0x3000004000 CFG_TZDRAM_SIZE=0x7fffc000
- Core VA budget (NEW, corrects ../FLASH checklist item 1): with CFG_CORE_PHYS_RELOCATABLE (Thor too)
  the TA L2 tables are NOT the static xlat_tables_ul1 array but boot_mem_alloc() at boot, taken
  from the free core VA (__vcore_free_start..__vcore_free_end). So too small a CFG_TEE_RAM_VA_SIZE
  = BOOT PANIC, not a link error. Numbers here (tee.elf symbols + sw log):
    image (text..nozi) 0x3000004000..0x30002cd000 = 2.8MB; free VA = 5.2MB of the 8MB
    boot "Allocated 2359680 bytes" (14 x 40 x 4KB = 2.24MB L2 tables + ~65KB other)
    "Releasing 3071616 bytes" = 2.9MB spare.
  => 8MB is fine for 14 threads; 28 threads would need ~4.6MB of the 5.2MB (tight but fits).
- heap sweep (logs/large2g_{nw,sw}.log):
> ./run_guest.exp large2g "optee_example_hello_world; sh /mnt/host/large_mem_tas/run_tests.sh" SPMC_AT_EL=2
  sw: "Physical secure memory base 0x3000000000 size 0x80000000". hello_world OK.
  heap 1..1920 MB ALL PASS (1920MB: open 34s, touch 0.33s), 2048 FAIL cleanly 0xffff000c
  (OUT_OF_MEMORY: region 2GB - core - L3 tables). No panics.
- isolation + regression (logs/iso_xtest_{nw,sw}.log):
> ./run_guest.exp iso_xtest "sh -c 'devmem 0x3000000000 64'; echo DEVMEM_RC=\$?; xtest" SPMC_AT_EL=2
  devmem -> "Bus error", rc 135. xtest: 152 test cases / 43572 subtests, 0 failed, 0 skipped.
  0 core panics ("Panic at"); 330 "panic" lines = TA panics xtest provokes on purpose.
  => DONE. This is the config to flash.

# Part 2 — Thor flash procedure (S = 2GB). Follow top to bottom.

Background (details/rationale in ../FLASH/progress.md, read its "Warnings" + "Recovery" first):
- Board: Jetson AGX Thor devkit, L4T R39.2.1, slot A. Flash from the x86 Ubuntu host with the
  R39.2.1 BSP (Linux_for_Tegra/), board in recovery mode over USB-C.
- NEVER burn fuses (no odmfuse.sh, no --fuse). Flash QSPI only ("internal"). Slot B stays stock.
- Chain: BCT (MB1) sizes the TZDRAM carve-out -> MB2 allocates each SP's mem_size inside it and
  patches load addresses -> Hafnium maps OP-TEE per t264_spmc.dtb -> OP-TEE uses its manifest's
  load-address + mem-size as ONE region (core at start, rest = TA pool), exactly as on QEMU.
- Only OP-TEE is compiled. Hafnium/ATF/MB1/MB2 are NVIDIA binaries; we only change 3 sizes.

## The 2GB values (all four must agree)
| where                                   | stock            | 2GB                         | QEMU analogue |
|-----------------------------------------|------------------|-----------------------------|---------------|
| BCT aux_info@CARVEOUT_TZDRAM size       | 80MB (0x5000000) | <0x0 0x83000000> = 80MB-32MB+2GB | qemu virt.c RAM |
| t264_spmc.dtb op-tee vm1 mem_size       | 0x2000000        | <0x80000000>                | spmc_el2_manifest.dts |
| tegra264-optee.dts mem-size             | <0x02000000>     | <0x80000000> (keep 1 cell)  | optee_sp_manifest.dts |
| plat-tegra/conf.mk t264 CFG_TZDRAM_SIZE | 0x02000000       | 0x80000000                  | qemu_v8.mk |
(TZDRAM also holds Hafnium, StMM 8MB, mods 1MB, BL31 shmem: hence 80MB - 32MB + S.)
mem-size: OP-TEE reads 1 or 2 cells (dt_getprop_as_number, same in NVIDIA's boot.c). 2GB fits
1 cell, so keep NVIDIA's format; only > 4GB needs 2 cells (then also an MB2 unknown).

## Step 0 — stock baseline (x86 host), proves the flash path
> tar xpf Jetson_Linux_R39.2.1_aarch64.tbz2
> sudo tar xpf Tegra_Linux_Sample-Root-Filesystem_R39.2.1_aarch64.tbz2 -C Linux_for_Tegra/rootfs
> cd Linux_for_Tegra && sudo ./tools/l4t_flash_prerequisites.sh && sudo ./apply_binaries.sh
> cp bootloader/tos-optee_t264.img bootloader/tos-optee_t264.img.orig
> cp bootloader/hafnium_t264.fip bootloader/hafnium_t264.fip.orig
> cp bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts{,.orig}
> sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
  Board boots as before -> path works. (BSP download URL in ../FLASH/progress.md.)

## Step 1 — build OP-TEE (on the Thor, native)
> cp -a ~/Desktop/josh/ARMHiT/scripts/TEE/jetson-public-srcs/Linux_for_Tegra/source/jetson-optee-srcs ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs
> cd ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs/optee/optee_os && patch -p1 < ~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium_2gb/hafnium_optee_os.diff
  Expect 1 reject in core_mmu_lpae.c set_user_va_idx: NVIDIA has a literal `4` where upstream has
  ARM32_USER_VA_SLOT_LIMIT. Apply that hunk by hand (use `4` in the N == 1 branch). Check others applied:
> grep -c CFG_NUM_TA_L2_TBLS core/arch/arm/mm/core_mmu_lpae.c mk/config.mk   (expect >0 for both)
  Edit core/arch/arm/plat-tegra/conf.mk (edit lines in place; they use ?= or plain =):
    line ~23  CFG_NUM_THREADS = $(CFG_TEE_CORE_NB_CORE)   ->  CFG_NUM_THREADS = 14
              (matches tegra264-optee.dts execution-ctx-count = <14>)
    line ~26  CFG_CORE_HEAP_SIZE ?= 131072                ->  CFG_CORE_HEAP_SIZE ?= 2097152
    line ~85  CFG_TEE_RAM_VA_SIZE ?= 0x400000             ->  CFG_TEE_RAM_VA_SIZE ?= 0x00800000
    t264 sec  CFG_TZDRAM_SIZE  ?= 0x02000000              ->  CFG_TZDRAM_SIZE  ?= 0x80000000
    t264 sec  add:  CFG_NUM_TA_L2_TBLS = 40
                    CFG_CORE_PREALLOC_EL0_TBLS = y
    (CFG_LPAE_ADDR_SPACE_BITS = 38 and CFG_CORE_ASLR stay as they are = what QEMU 2gb ran.)
  Edit ../tegra264-optee.dts:  mem-size = <0x80000000>;   (load-address untouched: MB2 patches it)
> cd ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs && export CROSS_COMPILE_AARCH64=aarch64-linux-gnu- PKG_CONFIG=/usr/bin/pkg-config
> ./optee_src_build.sh -p t264 -t
  Verify the config took (same lines as Part 1 log):
> grep -E "^CFG_(NUM_THREADS|NUM_TA_L2_TBLS|LPAE_ADDR_SPACE_BITS|TEE_RAM_VA_SIZE|CORE_HEAP_SIZE|CORE_PREALLOC_EL0_TBLS|TZDRAM_SIZE|CORE_ASLR)=" optee/build/t264/conf.mk
  Core VA check: readelf -sW optee/build/t264/core/tee.elf | grep -E "__vcore_free_(start|end)"
  -> free must be > 2.3MB (QEMU: 5.2MB). NVIDIA drivers make the image bigger; if free < 2.5MB use 16MB.
> dtc -I dts -O dtb -o optee/tegra264-optee.dtb optee/tegra264-optee.dts
> python3 <atf_t264>/tools/sptool/sptool.py -i optee/build/t264/core/tee-raw.bin:optee/tegra264-optee.dtb -o tos.img
  (<atf_t264> = NVIDIA ATF t264 source tarball from the BSP sources, see ../FLASH/progress.md recon.)
  Also keep optee/install/t264/* (libteec, tee-supplicant, xtest, export-ta_arm64) for the board.
  Optional safer split: flash this tos.img FIRST with stock sizes (mem-size 32MB / CFG_TZDRAM_SIZE
  0x02000000, = ../FLASH "Stage 1"), check xtest, THEN do Step 2. Isolates code bugs from carve-out bugs.

## Step 2 — the three NVIDIA-side edits (x86 host, in Linux_for_Tegra/)
a) OP-TEE: copy the tos.img from Step 1 over bootloader/tos-optee_t264.img
b) Hafnium manifest (no Hafnium rebuild):
> dtc -I dtb -O dts -o spmc.dts bootloader/t264_spmc.dtb
   edit the op-tee vm (vm1, debug_name "op-tee"): mem_size = <0x80000000>;  nothing else.
   Check while there: does it have secure memory nodes (QEMU needed memory@3)? Note it in progress.
> dtc -I dts -O dtb -o t264_spmc.dtb spmc.dts
> <atf_t264>/tools/fiptool/fiptool update --tos-fw-config t264_spmc.dtb bootloader/hafnium_t264.fip
c) BCT: bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts, next to the existing
   GPU_COMPTAGS carveout override, same syntax:
     carveout { aux_info@CARVEOUT_TZDRAM { size = <0x0 0x83000000>; }; };
   (stock: tegra264-mb1-bct-carveout-l4t.dtsi size 80MB, alignment 2MB — leave alignment.)

## Step 3 — flash (board in recovery each time, one partition per run)
> sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_MB1_BCT jetson-agx-thor-devkit internal
> sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_secure-hv jetson-agx-thor-devkit internal
> sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
  If -k A_MB1_BCT alone doesn't take effect: full `internal` flash with the edited files (writes A+B).

## Step 4 — checks on the board (compare with Part 1)
- Boots at all? If not: serial console log (MB1/MB2/Hafnium/OP-TEE), then Recovery below.
- OP-TEE log (FF-A console): "Physical secure memory base <X> size 0x80000000"; no "Panic at".
- free -g: ~2GB less than stock. /dev/tpm0 still works (fTPM TA).
- Install optee/install/t264 libteec + tee-supplicant + xtest; run: xtest  (expect 0 failed)
- Heap sweep: copy large_mem_tas/ to the board, build natively with the Thor dev kit:
    TA_DEV_KIT_DIR=<optee-srcs>/optee/install/t264/export-ta_arm64 (or optee/build/t264/export-ta_arm64)
    CROSS_COMPILE=aarch64-linux-gnu-, TEEC_EXPORT=<optee install>/usr  (build.sh assumes ../optee: adapt paths)
  Expect as QEMU: 1..1920 MB PASS, 2048 FAIL 0xffff000c. Compare open/touch times (real HW now).
- Stock limits to compare: stock Thor = 27 of 32MB max, >= 1GB ACCESS_CONFLICT (../TEE/progress.md).

## Recovery
- A slot bad -> bootloader falls back to stock slot B after failed boots (check: nvbootctrl).
- Restore stock: copy back the three *.orig files, re-flash the same three -k partitions.
- Worst case: recovery mode + full stock QSPI flash:
> sudo ./tools/kernel_flash/l4t_initrd_flash.sh jetson-agx-thor-devkit internal

## If something fails, map it back to QEMU
- OP-TEE panics in boot_mem / mmu init -> core VA too small (raise CFG_TEE_RAM_VA_SIZE), reproduce
  here by building with CFG_NUM_THREADS=28.
- "Page tables are not available" at TA open -> CFG_CORE_HEAP_SIZE.
- Hafnium refuses the SP / MB2 "SP memory allocation failed" -> sizes in the table disagree
  (BCT too small for Hafnium + 2GB + StMM + mods + 1MB).
- No boot before OP-TEE runs -> MB1/MB2 rejected the carve-out size (closed; we can't patch). 2GB is
  under 4GB, so not a 32-bit issue; suspect placement/alignment.
- Next after 2GB works: S = 36GB (BCT <0x9 0x03000000>, mem_size/mem-size 2 cells <0x9 0x0>),
  qemu_dev_hafnium (34GB) is the rehearsal for that.

## Addendum 2026-11-12 — serial console (do this before Step 0)
- Debug USB-C under the magnetic lid (NOT the flashing USB-C) -> x86 host, USB-C to A/C cable.
  Host sees up to 4 ports /dev/ttyACM0..3; the console one is unknown (ttyACM0 may be the board
  automation interface) -> try each. 115200 8N1. Works with the board off but powered.
> sudo minicom -D /dev/ttyACM<n> -8 -b 115200        (dialout group avoids sudo)
- Shows MB1 ("MB1 last_boot_error: 0x0") / UEFI / kernel. OP-TEE (CFG_FFA_CONSOLE) + Hafnium on it:
  unconfirmed. Capture the STOCK boot (Step 0) as a reference log, then every flash after.
- Refs: RidgeRun wiki NVIDIA_Jetson_AGX_Thor/In_Board/Getting_in_Board/Serial_Console;
  docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/AT/BoardAutomation.html
