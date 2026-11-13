# Thor: flash a bigger OP-TEE (slot A, QSPI only) from the x86 host

Every command below is the exact command that was run and worked, unless marked **(not run yet)**.
Host: Ubuntu 24.04.5 x86_64, user josh, folder `~/Desktop/thor_flash`. Board: Jetson AGX Thor devkit, L4T R39.2.1.
Full history, dead ends, and reasons: `notes/x86_flash_log_2026-11-13.md`. Serial logs: `logs/`.
Claude notes from both machines: `claude_memory/` (Thor-side) and `claude_memory/x86_*.md`.

## Status (2026-11-13)

| set | OP-TEE size S | result |
|---|---|---|
| 2gb | 0x80000000 | flashed; boots; xtest 0 failed; bigmem 1..1920 MB pass, 2048 fails cleanly; free -g 122 -> 120 |
| 36gb | 0x900000000 | **can't work.** MB1 scrub timeout (fixed by option B), then MB2 reads SP mem_size as 32-bit |
| 4gb | 0xFFE00000 (4GB - 2MB) | **on slot A now.** Boots, free -g 118. bigmem/xtest results not recorded yet |

The 4gb set uses **option B**, a non-stock MB1 setting: `enable_dram_staged_scrubbing = <0>` in our BCT dts (NVIDIA
default 1). It's needed for TZDRAM over about 2GB. To undo it, flash the 2gb set (see "Reflash a saved set").

## Rules

- Write only slot A, QSPI only. The NVMe (Ubuntu) is never touched. Slot B was never written (stock fallback).
- **Never run:** `--flash-only`, `--external-device`, `-c ...nvme.xml`, `--erase-all`, `--clean`, `odmfuse.sh`, `--fuse`,
  or `flash_bsp_images.py` without `-u`. The `--no-flash` package also contains the NVMe image (`system.img`):
  a full flash would overwrite Ubuntu.
- `l4t_initrd_flash.sh -k <partition>` (FLASH_STEPS.md §5–6) **fails** on Thor. Use the commands below.
- `flash_bsp_images.py` needs **absolute** paths. Keep `-u ...` last.

## 1. One-time host setup (done 2026-11-13)

```bash
mkdir -p ~/Desktop/thor_flash/bsp/downloads && cd ~/Desktop/thor_flash/bsp/downloads
curl -fL -C - -O https://developer.download.nvidia.com/embedded/L4T/r39_Release_v2.1/release/Jetson_Linux_R39.2.1_aarch64.tbz2 -O https://developer.download.nvidia.com/embedded/L4T/r39_Release_v2.1/release/Tegra_Linux_Sample-Root-Filesystem_R39.2.1_aarch64.tbz2
# sizes 1287223769 / 2022864741; BSP sha1 8fe9a3ac9a98550b6077df450441599141ba47fb
sudo apt install -y bzip2 && cd ~/Desktop/thor_flash/bsp && tar xpf downloads/Jetson_Linux_R39.2.1_aarch64.tbz2 && sudo tar xpf downloads/Tegra_Linux_Sample-Root-Filesystem_R39.2.1_aarch64.tbz2 -C Linux_for_Tegra/rootfs && cd Linux_for_Tegra && sudo ./tools/l4t_flash_prerequisites.sh && sudo ./apply_binaries.sh
# (rootfs extraction ~10 min, prints nothing)

cd ~/Desktop/thor_flash/bsp/Linux_for_Tegra
md5sum bootloader/hafnium_t264.fip bootloader/t264_spmc.dtb bootloader/tos-optee_t264.img bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts
# 4ccd87144fab471a361d77b6b5ed56a4  6f42759d4414bdf5ab33573a38bdf8fd  fdf33da75b49c35f8cadc92642e94d5f  e017926e35c8129272a756700a196b4c
cp -n bootloader/tos-optee_t264.img{,.orig}
cp -n bootloader/hafnium_t264.fip{,.orig}
cp -n bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts{,.orig}

# for building OP-TEE on x86 (section 4); a fresh 24.04 has no make/gcc
sudo apt install -y gcc-aarch64-linux-gnu pkg-config python3-pyelftools python3-pycryptodome
sudo apt install -y build-essential
```
USB ids (`lsusb | grep 0955`):
- `7045`: debug port, always there.
- `7020`: Thor running Ubuntu.
- `7026` APX: recovery.
- `7100`: flashing environment.

## 2. Every flash: serial log + recovery

```bash
# terminal 1 (keep running; use a new file name per flash)
sudo stty -F /dev/ttyACM0 115200 && sudo cat /dev/ttyACM0 | tee ~/Desktop/boot_4gb.log
```
Recovery mode:
1. On the Thor: `sudo poweroff`.
2. Hold **Force Recovery**, press **Power**, release both.
3. If the last flash attempt failed, **pull power** (or hold Power 10 s) first. Otherwise the next run fails with
   `timeout in USB read`.

```bash
lsusb | grep 0955          # must show 0955:7026 NVIDIA Corp. APX
```

## 3. Flash the 4gb set (what is on the board now)

```bash
cd ~/Desktop/thor_flash/bsp/Linux_for_Tegra
cp ~/Desktop/thor_flash/to_x86_4gb/tos_4gb.img bootloader/tos-optee_t264.img
cp ~/Desktop/thor_flash/to_x86_4gb/hafnium_t264_4gb.fip bootloader/hafnium_t264.fip
cp ~/Desktop/thor_flash/to_x86_4gb/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts bootloader/generic/BCT/
md5sum bootloader/tos-optee_t264.img bootloader/hafnium_t264.fip bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts
# a8781cffcbd498dc366880b412620daf  17b1dd056453c750c672906aeae4128f  616e54420410b40c414bd78aaf516986  (= to_x86_4gb/MD5SUMS)

# board in recovery (7026). Build the signed package: host only, writes nothing to the board, ~1-2 min.
# Ends with "Finish generating flash package. Put device in recovery mode, run with option --flash-only" -> IGNORE that hint
sudo ./tools/kernel_flash/l4t_initrd_flash.sh --no-flash jetson-agx-thor-devkit internal

# check the package holds the 4gb files (prints ALL OK)
python3 -I ~/Desktop/thor_flash/tools/check_package.py unified_flash/out ~/Desktop/thor_flash/to_x86_4gb/tos_4gb.img ~/Desktop/thor_flash/to_x86_4gb/hafnium_t264_4gb.fip bootloader/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts

# flash (board still 7026). ~4x "adb ... wait-for-device failed ... Return code: 124" first = normal
sudo python3 /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out/bsp_images/tools/flashtools/bootburn/flash_bsp_images.py -b jetson-t264 --l4t -P /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out/bsp_images/flash_workspace --l4t_boot_chain_select A -u A_MB1_BCT A_secure-hv A_sec-partition
# -> "Flashing finished Successfully!!" (50 s). Board stays at 0955:7100: press Reset (no Force Recovery) to boot.

# keep the signed package + named copies of the inputs
cp -a unified_flash/out unified_flash/out_4gb
cp -n bootloader/tos-optee_t264.img bootloader/tos-optee_t264.img.4gb
cp -n bootloader/hafnium_t264.fip bootloader/hafnium_t264.fip.4gb
cp bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts.4gb_nostaged
```

### 3b. The 2gb flash (first flash, 2026-11-13 14:25; same steps)

```bash
cd ~/Desktop/thor_flash/bsp/Linux_for_Tegra
cp ~/Desktop/thor_flash/to_x86/tos_2gb.img bootloader/tos-optee_t264.img
cp ~/Desktop/thor_flash/to_x86/hafnium_t264_2gb.fip bootloader/hafnium_t264.fip
cp ~/Desktop/thor_flash/to_x86/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts bootloader/generic/BCT/
sudo ./tools/kernel_flash/l4t_initrd_flash.sh --no-flash jetson-agx-thor-devkit internal
sudo python3 /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out/bsp_images/tools/flashtools/bootburn/flash_bsp_images.py -b jetson-t264 --l4t -P /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out/bsp_images/flash_workspace --l4t_boot_chain_select A -u A_MB1_BCT A_secure-hv A_sec-partition
cp -n bootloader/tos-optee_t264.img bootloader/tos-optee_t264.img.2gb
cp -n bootloader/hafnium_t264.fip bootloader/hafnium_t264.fip.2gb
cp -n bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts.2gb
cp -a unified_flash/out unified_flash/out_2gb
```

## 4. How the 4gb set was built (all on x86, 2026-11-13)

Four places must agree. Here S = 0xFFE00000. S must be **< 4GB** (MB2 limit).

| place | 2gb value | 4gb value |
|---|---|---|
| `optee/optee_os/core/arch/arm/plat-tegra/conf.mk` line 176 | `CFG_TZDRAM_SIZE ?= 0x80000000` | `CFG_TZDRAM_SIZE ?= 0xFFE00000` |
| `optee/tegra264-optee.dts` | `mem-size = <0x80000000>;` | `mem-size = <0xFFE00000>;` |
| Hafnium manifest, vm1 "optee" | `mem_size = <0x80000000>;` | `mem_size = <0xFFE00000>;` |
| BCT dts TZDRAM (0x3000000 + S) | `size = <0x0 0x83000000>;` | `size = <0x1 0x02E00000>;` + option B |

```bash
# OP-TEE: base = patched NVIDIA sources with the 2GB config (Thor-built tarball)
mkdir -p ~/Desktop/thor_flash/build_4gb ~/Desktop/thor_flash/to_x86_4gb
tar -xzf ~/Desktop/thor_flash/optee-srcs-2gb.tar.gz -C ~/Desktop/thor_flash/build_4gb
cd ~/Desktop/thor_flash/build_4gb/optee-srcs
sed -i '176s/0x80000000/0xFFE00000/' optee/optee_os/core/arch/arm/plat-tegra/conf.mk
sed -i 's/mem-size = <0x80000000>;/mem-size = <0xFFE00000>;/' optee/tegra264-optee.dts
# (results saved as to_x86_4gb/conf.mk.4gb and to_x86_4gb/tegra264-optee_4gb.dts)
export CROSS_COMPILE_AARCH64_PATH=/usr CROSS_COMPILE_AARCH64=/usr/bin/aarch64-linux-gnu- PKG_CONFIG=/usr/bin/pkg-config && ./optee_src_build.sh -p t264 -t > ../optee_build.log 2>&1
tail -1 ../optee_build.log      # "optee sources compiled successfully."
dtc -I dts -O dtb -o optee/tegra264-optee.dtb optee/tegra264-optee.dts
python3 ~/Desktop/thor_flash/tools/sptool/sptool.py -i optee/build/t264/core/tee-raw.bin:optee/tegra264-optee.dtb -o ~/Desktop/thor_flash/to_x86_4gb/tos_4gb.img
readelf -sW optee/build/t264/core/tee.elf | grep -E "__vcore_free_(start|end)"
# 0x8037e000 .. 0x80800000 = 4.5MB free (must be > 2.3MB)
grep -E '^CFG_(TZDRAM_SIZE|NUM_THREADS|CORE_HEAP_SIZE|TEE_RAM_VA_SIZE|NUM_TA_L2_TBLS|CORE_PREALLOC_EL0_TBLS)=' optee/build/t264/conf.mk
# TZDRAM_SIZE=0xFFE00000 NUM_THREADS=14 CORE_HEAP_SIZE=2097152 TEE_RAM_VA_SIZE=0x00800000 NUM_TA_L2_TBLS=40 PREALLOC_EL0_TBLS=y

# Hafnium manifest: stock dtb, change only vm1 "optee" mem_size (one cell)
cd ~/Desktop/thor_flash/build_4gb
dtc -I dtb -O dts -o spmc_stock.dts ~/Desktop/thor_flash/bsp/Linux_for_Tegra/bootloader/t264_spmc.dtb
sed '/debug_name = "optee";/{n;s/mem_size = <0x2000000>;/mem_size = <0xFFE00000>;/}' spmc_stock.dts > spmc_4gb.dts
dtc -I dts -O dtb -o t264_spmc_4gb.dtb spmc_4gb.dts
cp ~/Desktop/thor_flash/bsp/Linux_for_Tegra/bootloader/hafnium_t264.fip.orig hafnium_t264_4gb.fip
~/Desktop/thor_flash/tools/fiptool update --tos-fw-config t264_spmc_4gb.dtb hafnium_t264_4gb.fip
cp hafnium_t264_4gb.fip t264_spmc_4gb.dtb spmc_4gb.dts ~/Desktop/thor_flash/to_x86_4gb/

# BCT dts = stock file (first 38 lines) + this override block appended at the end
#   (file: to_x86_4gb/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts)
```
```dts
/* ARMHiT large TA memory: TZDRAM = 80MB - 32MB (stock OP-TEE) + 4GB-2MB = 0xFFE00000 (new OP-TEE; MB2 reads SP mem_size as 32-bit) */
/ {
    mb1_bct {
        carveout {
            aux_info@CARVEOUT_TZDRAM {
                size = <0x1 0x02E00000>;
            };
        };
        /* ARMHiT OPTION B (2026-11-13): NOT STOCK. NVIDIA default = 1 (staged, tegra264-mb1-bct-features-defaults.dtsi:61).
         * Set to 0 to try to avoid MB1 "Timeout while polling for CO 49 scrub" with a TZDRAM > ~2GB (seen at 36GB).
         * UNDO: delete this non_psc_shared_features block (or use the .dts.36gb copy). */
        non_psc_shared_features {
            enable_dram_staged_scrubbing = <0>;
        };
    };
};
```
```bash
# bigmem TAs, built against THIS OP-TEE build (build.sh uses ../optee-srcs)
cp -a ~/Desktop/thor_flash/large_mem_tas ~/Desktop/thor_flash/build_4gb/large_mem_tas && cd ~/Desktop/thor_flash/build_4gb/large_mem_tas && rm -rf out
sed -i 's/^HEAP_MBS=.*/HEAP_MBS="1 1024 2048 3072 3584 3840 3968 4000 4032 4064 4096"/' build.sh
sed -i 's/^for mb in .*; do/for mb in 1 1024 2048 3072 3584 3840 3968 4000 4032 4064 4096; do/' run_tests.sh
./build.sh > ../tas_build.log 2>&1        # 1 warning: gpd.ta.dataSize uint32 overflow for 4096 (cosmetic)
mkdir -p ~/Desktop/thor_flash/to_x86_4gb/large_mem_tas && cp -a out run_tests.sh build.sh ta host ~/Desktop/thor_flash/to_x86_4gb/large_mem_tas/

cd ~/Desktop/thor_flash/to_x86_4gb && md5sum tos_4gb.img hafnium_t264_4gb.fip t264_spmc_4gb.dtb tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts > MD5SUMS
```
For another size, change the 4 values above (the BCT = 0x3000000 + S) and the TA sizes, using a new `build_<set>/` and
`to_x86_<set>/`. Then run section 3 with those files.

## 5. Checks after boot

```bash
# x86: serial log
grep -anE "last_boot_error|Scrub Mode|CO 49|SP: |SP memory|Loading VM|OP-TEE version|E/TC|Panic at" ~/Desktop/boot_4gb.log
```
```bash
# Thor
sudo nvbootctrl get-current-slot     # 0 = slot A
free -g                              # stock ~122; 2gb 120; 4gb 118
ls -l /dev/tpm0 ; sudo tpm2_getrandom 8 --hex
sudo xtest 2>&1 | tail -3            # 2gb: 0 failed
sudo sh <copy of to_x86_4gb/large_mem_tas>/run_tests.sh 2>&1 | tee ~/bigmem_4gb.log
```

| serial message | meaning |
|---|---|
| `Timeout while polling for CO 49 scrub` (MB1) | TZDRAM too big for staged scrubbing: needs option B |
| `SP: Accessing SP1 ... after boundary ...` (MB2) | SP mem_size >= 4GB, which MB2 reads as 32-bit |
| `SP memory allocation failed` (MB2) | TZDRAM smaller than the manifest sizes |
| `Panic at` in mmu/boot_mem (OP-TEE) | core VA too small |
| `jetson_user_key_pta_init: Failed (ffff0005)` | EKB error. Seen on the 2gb and 4gb boots; xtest still passes. Stock not compared |

## 6. Reflash a saved set (no rebuild) — (not run yet: same command, pointed at the saved copy)

```bash
# 2gb (known good; also removes option B)
sudo python3 /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out_2gb/bsp_images/tools/flashtools/bootburn/flash_bsp_images.py -b jetson-t264 --l4t -P /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out_2gb/bsp_images/flash_workspace --l4t_boot_chain_select A -u A_MB1_BCT A_secure-hv A_sec-partition
# 4gb
sudo python3 /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out_4gb/bsp_images/tools/flashtools/bootburn/flash_bsp_images.py -b jetson-t264 --l4t -P /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out_4gb/bsp_images/flash_workspace --l4t_boot_chain_select A -u A_MB1_BCT A_secure-hv A_sec-partition
```
If the saved copy misbehaves, rebuild instead: section 3 with the `.2gb`/`.4gb` files.

## 7. Back to stock slot A — (not run yet)

```bash
cd ~/Desktop/thor_flash/bsp/Linux_for_Tegra
cp bootloader/tos-optee_t264.img.orig bootloader/tos-optee_t264.img
cp bootloader/hafnium_t264.fip.orig bootloader/hafnium_t264.fip
cp bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts.orig bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts
# then recovery, --no-flash, flash_bsp_images.py -u ... exactly as in section 3
```

## 8. The 36GB attempts (failed, nothing written; kept for reference)

```bash
cd ~/Desktop/thor_flash/bsp/Linux_for_Tegra
cp ~/Desktop/thor_flash/to_x86_36gb/tos_36gb.img bootloader/tos-optee_t264.img
cp ~/Desktop/thor_flash/to_x86_36gb/hafnium_t264_36gb.fip bootloader/hafnium_t264.fip
cp ~/Desktop/thor_flash/to_x86_36gb/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts bootloader/generic/BCT/
sudo ./tools/kernel_flash/l4t_initrd_flash.sh --no-flash jetson-agx-thor-devkit internal
sudo python3 /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out/bsp_images/tools/flashtools/bootburn/flash_bsp_images.py -b jetson-t264 --l4t -P /home/josh/Desktop/thor_flash/bsp/Linux_for_Tegra/unified_flash/out/bsp_images/flash_workspace --l4t_boot_chain_select A -u A_MB1_BCT A_secure-hv A_sec-partition
```
- **Try 1 (BCT as received):** `tegrarcm_v2 ... --download blob` -> "timeout in USB write". Serial:
  `Timeout while polling for CO 49 scrub` (CO 49 = CARVEOUT_RCM_BLOB). Log: `logs/boot_36gb_try1.log`.
- **Try 2 (+ option B, BCT `.dts.36gb_nostaged`):** MB1 passed (`DRAM ECC Scrub Mode: full`). MB2 then failed with
  `SP: Accessing SP1 0x170350e000 after boundary 0x1703421000`: the 36GB mem_size was read as 32-bit, so 0.
  adb then loops on wait-for-device. Log: `logs/boot_36gb.log`.
- Packages kept: `unified_flash/out_36gb`, `out_36gb_nostaged`.

## What is where

| path | contents |
|---|---|
| `to_x86/` | 2gb set (built on the Thor) + MD5SUMS |
| `to_x86_36gb/` | 36gb set (built on the Thor; does not boot) |
| `to_x86_4gb/` | 4gb set (built on x86), conf.mk/dts used, Hafnium dts, bigmem TAs + run_tests.sh, MD5SUMS |
| `build_4gb/` | x86 build logs + dts (`optee-srcs/` gitignored: recreate with section 4) |
| `optee-srcs-2gb.tar.gz` | patched NVIDIA OP-TEE sources (2GB config) = base for every build |
| `notes/hafnium_optee_os.diff` | the OP-TEE code changes inside that tarball |
| `tools/` | sptool, fiptool (x86), check_package.py |
| `logs/` | serial logs: boot_2gb, boot_36gb_try1, boot_36gb, boot_4gb |
| `bsp/` (gitignored, ~64GB) | downloads + Linux_for_Tegra (recreate with section 1) |
| `bsp/Linux_for_Tegra/bootloader/*.{orig,2gb,36gb,4gb}`, BCT `.dts.{orig,2gb,36gb,36gb_nostaged,4gb_nostaged}` | each set's 3 input files |
| `bsp/Linux_for_Tegra/unified_flash/out_{2gb,4gb,36gb,36gb_nostaged}` | signed package per set (`out` = last built) |

## Open

- 4gb: record xtest + bigmem results (`to_x86_4gb/large_mem_tas`) here.
- More than 4GB needs something MB2 doesn't size: e.g. an extra FF-A memory region at a fixed TZDRAM `pref_base`,
  plus OP-TEE support for 2 regions. Or a fix from NVIDIA.
- EKB/fTPM: `tpm2_getrandom`, and compare with a stock boot log.

## Prompt for the Thor's Claude (when the Thor builds a set)

> The <prev> flash worked. Package the **<set> set (S = <hex>)** the same way as the 2GB one and send it to x86
> `~/Desktop/thor_flash/to_x86_<set>/` with MD5SUMS. Don't flash. Include: `tos_<set>.img` (conf.mk CFG_TZDRAM_SIZE = S,
> dts mem-size = S; check `__vcore_free` > 2.3MB), `hafnium_t264_<set>.fip` (vm1 mem_size = S) + its dts, the BCT misc dts
> (TZDRAM = 0x3000000 + S, keep option B), bigmem TAs + run_tests.sh up to S with one size above S that must fail
> cleanly, and the config changes as a diff vs NVIDIA stock. S must be < 4GB (MB2 limit).
