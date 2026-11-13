> **2026-11-13: outdated for flashing. Use progress.md.** §5–6 (`l4t_initrd_flash.sh -k`) fails on Thor. The working
> method is `--no-flash` + `flash_bsp_images.py -u ...`. This file is the original Thor-side plan.

# FLASH_STEPS: first Thor flash (OP-TEE region 32MB -> 2GB)

Thor = this board. x86 = Ubuntu 24.04 host (installed natively, not a VM) with the R39.2.1 BSP.
Background: ./progress.md and ../qemu_dev_hafnium_2gb/progress.md.

**Never** run odmfuse.sh or anything with `--fuse`. Flash slot **A** only (`-k A_...`). Slot B stays stock.

## Already built on the Thor (2026-11-13), copied to x86 `~/Desktop/thor_flash/`
| path in thor_flash/ | what |
|---|---|
| FLASH_STEPS.md | this file |
| to_x86/ | the flash files (table below) + MD5SUMS |
| notes/ | FLASH progress.md, qemu_dev_hafnium_2gb progress.md, TEE progress.md, hafnium_optee_os.diff, t264_spmc_2gb.dts |
| optee-srcs-2gb.tar.gz | patched NVIDIA OP-TEE source (2GB config), no build outputs: rebuild on x86 (see end) |
| tools/sptool/, tools/fiptool | sptool.py (packs tos.img) + x86 fiptool (edits hafnium fip) |
| large_mem_tas/ | bigmem test (TAs + app prebuilt for the Thor in out/, also still on the Thor's disk) |
| claude_memory/ | Claude's memory notes from the Thor (preferences / project context) |

Check: `cd ~/Desktop/thor_flash/to_x86 && md5sum -c MD5SUMS` (all OK).

| to_x86/ file | goes to (x86, Linux_for_Tegra/) | what |
|---|---|---|
| tos_2gb.img | bootloader/tos-optee_t264.img | MAIN: new OP-TEE, mem-size 2GB |
| hafnium_t264_2gb.fip | bootloader/hafnium_t264.fip | MAIN: stock Hafnium binary, manifest optee mem_size 2GB |
| tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts | bootloader/generic/BCT/ (same name) | MAIN: stock + TZDRAM size override 0x83000000 |
| tos_32mb.img | bootloader/tos-optee_t264.img | backup only (§6-backup): new code, stock 32MB |
| t264_spmc_2gb.dtb | (reference only) | the manifest inside hafnium_t264_2gb.fip |

Built from stock R39.2.1 files with these md5s. Check that your x86 BSP matches (step 2):
`hafnium_t264.fip 4ccd8714…`, `t264_spmc.dtb 6f42759d…`, `tos-optee_t264.img fdf33da7…`,
`BCT misc-p3834-0008 dts e017926e…`

## 1. x86: serial console (debug log). WORKS, tested 2026-11-13: ttyACM0 = console
Debug USB-C port (under the magnetic lid) -> x86. Board only needs power. `ls /dev/ttyACM*` -> ACM0..3.
Start this BEFORE powering on or rebooting the Thor. Use a new file name for each boot (tee overwrites):
```
sudo stty -F /dev/ttyACM0 115200 && sudo cat /dev/ttyACM0 | tee ~/Desktop/boot_<name>.log
```
Ctrl-C to stop. No trailing `/` after ttyACM0. ACM1-3: unknown (helper processors / automation?).

## 2. x86: BSP + backups
```
tar xpf Jetson_Linux_R39.2.1_aarch64.tbz2
sudo tar xpf Tegra_Linux_Sample-Root-Filesystem_R39.2.1_aarch64.tbz2 -C Linux_for_Tegra/rootfs
cd Linux_for_Tegra
sudo ./tools/l4t_flash_prerequisites.sh && sudo ./apply_binaries.sh
md5sum bootloader/hafnium_t264.fip bootloader/t264_spmc.dtb bootloader/tos-optee_t264.img \
       bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts   # match the list above
cp bootloader/tos-optee_t264.img{,.orig}
cp bootloader/hafnium_t264.fip{,.orig}
cp bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts{,.orig}
```
BSP download: https://developer.download.nvidia.com/embedded/L4T/r39_Release_v2.1/release/Jetson_Linux_R39.2.1_aarch64.tbz2
(the rootfs tarball is on the same R39.2.1 page).

## 3. Thor: record the baseline
```
sudo nvbootctrl dump-slots-info ; free -g
```
Then shut the board down.

## 4. Recovery mode (before every flash)
1. Board off.
2. Hold **Force Recovery** and press **Power**, then release both.
3. Connect the **flashing** USB-C port to the x86 machine.
4. Check on the x86 machine that the board shows up: `lsusb | grep -i nvidia`

## 5–6. Run B: flash 2GB (3 partitions, recovery mode before each)
```
cp ~/Desktop/thor_flash/to_x86/tos_2gb.img bootloader/tos-optee_t264.img
cp ~/Desktop/thor_flash/to_x86/hafnium_t264_2gb.fip bootloader/hafnium_t264.fip
cp ~/Desktop/thor_flash/to_x86/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts bootloader/generic/BCT/
sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_MB1_BCT       jetson-agx-thor-devkit internal
sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_secure-hv     jetson-agx-thor-devkit internal
sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
```
Boot, then run the §7 checks.

If the flash command itself errors out (the direct `-k` form is untested), use NVIDIA's
documented form instead, per partition. It only prepares the flash; its log prints the actual
flash commands, which you then run:
`sudo UNIFIED_FLASH=1 ./tools/kernel_flash/l4t_initrd_flash.sh --no-flash -k <partition> jetson-agx-thor-devkit internal`

If the board boots but the size is still 32MB, `-k A_MB1_BCT` alone wasn't enough. The fallback
is a full flash with the same files:
`sudo ./tools/kernel_flash/l4t_initrd_flash.sh jetson-agx-thor-devkit internal`
That flash also overwrites slot B, so check that the restore in §8 works before you try it.

### 6-backup. If Run B fails: Run A (new code at stock 32MB)
This splits "our OP-TEE code is broken" from "the 2GB carve-out is broken".
1. Restore stock first (§8), so the BCT and Hafnium image are stock again.
2. Flash the 32MB build (recovery mode first):
```
cp ~/Desktop/thor_flash/to_x86/tos_32mb.img bootloader/tos-optee_t264.img
sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
```
3. Boot and run the §7 checks.
- Run A passes: the code is fine, and the 2GB sizes (BCT, MB1/MB2, Hafnium) are the problem.
- Run A fails: the bug is in our OP-TEE code or build.

## 7. Checks on the Thor after each boot
```
sudo nvbootctrl get-current-slot          # MUST be 0. 1 = A failed, you're on stock B
sudo dmesg | grep -iE "optee|ffa"         # "optee: revision 4.6", no errors
free -g                                   # Run B: ~2GB less than the baseline
ls /dev/tpm0                              # fTPM still up
sudo xtest 2>&1 | tail -3                 # expect 0 failed
sudo sh ~/Desktop/josh/ARMHiT/scripts/FLASH/large_mem_tas/run_tests.sh 2>&1 | tee ~/bigmem.log
```
The TAs for the sweep are already built (`large_mem_tas/out`).
- Run B expected: 1..1920 MB PASS, 2048 FAIL 0xffff000c.
- Run A expected: only the small heaps (≤ ~16MB) pass. The rest must fail cleanly, with no panic.

Serial log (x86):
```
grep -nE "last_boot_error|MB2|SP memory|ERROR|Panic at|E/TC|Physical secure memory" ~/Desktop/boot_<name>.log
```
- Run B expected: `Physical secure memory base ... size 0x80000000`. This shows only if OP-TEE
  prints to serial at all; compare with the stock boot log.

| symptom | cause |
|---|---|
| nothing after MB1, or last_boot_error != 0 | MB1 rejected the carve-out → restore |
| `SP memory allocation failed` (MB2) | TZDRAM too small vs manifest sizes |
| `Panic at` in mmu/boot_mem | core VA too small (built with 4.5MB free, needs 2.3MB) |
| slot = 1 | slot A didn't boot. Read its serial log |

## 8. Restore stock (x86)
```
cp bootloader/tos-optee_t264.img.orig bootloader/tos-optee_t264.img
cp bootloader/hafnium_t264.fip.orig bootloader/hafnium_t264.fip
cp bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts{.orig,}
```
Then re-flash the same `-k` partitions as in §6. Worst case, do a full stock flash. It writes
QSPI only; the NVMe is untouched:
`sudo ./tools/kernel_flash/l4t_initrd_flash.sh jetson-agx-thor-devkit internal`

## Where the files came from (Thor, to rebuild)
- `optee-srcs/`: NVIDIA jetson-optee-srcs + `../qemu_dev_hafnium_2gb/hafnium_optee_os.diff`.
  - One hunk (set_user_va_idx loop) was applied by hand, with `4` in place of ARM32_USER_VA_SLOT_LIMIT.
  - conf.mk changes: NUM_THREADS 14, HEAP 2MB, TEE_RAM_VA 8MB, t264 TZDRAM_SIZE 0x80000000,
    + NUM_TA_L2_TBLS 40, PREALLOC_EL0_TBLS y.
  - dts mem-size 0x80000000.
  - `optee-srcs-32mb/` is the same, with the sizes left at 32MB.
- Build: `export CROSS_COMPILE_AARCH64_PATH=/usr CROSS_COMPILE_AARCH64=/usr/bin/aarch64-linux-gnu- PKG_CONFIG=/usr/bin/pkg-config; ./optee_src_build.sh -p t264 -t`
- Package: `dtc` (optee dts → dtb), then `python3 atf_t264/.../tools/sptool/sptool.py -i tee-raw.bin:dtb -o tos.img`.
- Hafnium fip: stock dtb → dts, edit the optee vm `mem_size`, dts → dtb, then
  `fiptool update --tos-fw-config` (fiptool rebuilt for arm64 in `atf_t264/`).
- `t264_spmc_2gb.dts`: the edited Hafnium manifest source.


## Picking up on x86 (Thor not reachable)
- Claude: the Thor session (`claude --resume 7008d770-e3d8-4855-8d7c-5f3c1bae3bbf`) only resumes ON THE THOR.
  Nothing from the Thor repo is committed/pushed: ~/Desktop/thor_flash/ is the ONLY copy on x86.
  Start a new session in ~/Desktop/thor_flash and tell it to read FLASH_STEPS.md + notes/
  (claude_memory/ = the Thor's memory notes; notes/qemu_dev_hafnium_2gb/ = QEMU rehearsal diffs + manifest).
- State at handoff: nothing flashed yet. Serial console works (ttyACM0). x86 at josh@128.189.251.144.
  Next: §2 (BSP + backups) -> §3 -> §4 -> §5–6 (2GB) -> §7. Write results in FLASH/progress.md.
- Rebuild OP-TEE on x86 if a config change is needed (e.g. `Panic at` -> CFG_TEE_RAM_VA_SIZE 0x01000000),
  untested on x86:
```
sudo apt install gcc-aarch64-linux-gnu device-tree-compiler pkg-config python3-pyelftools python3-cryptography python3-pycryptodome
mkdir -p ~/Desktop/thor_flash/build && tar -xzf ~/Desktop/thor_flash/optee-srcs-2gb.tar.gz -C ~/Desktop/thor_flash/build
cd ~/Desktop/thor_flash/build/optee-srcs        # edit optee/optee_os/core/arch/arm/plat-tegra/conf.mk as needed
export CROSS_COMPILE_AARCH64_PATH=/usr CROSS_COMPILE_AARCH64=/usr/bin/aarch64-linux-gnu- PKG_CONFIG=/usr/bin/pkg-config
./optee_src_build.sh -p t264 -t
dtc -I dts -O dtb -o optee/tegra264-optee.dtb optee/tegra264-optee.dts
python3 ~/Desktop/thor_flash/tools/sptool/sptool.py -i optee/build/t264/core/tee-raw.bin:optee/tegra264-optee.dtb -o tos_new.img
readelf -sW optee/build/t264/core/tee.elf | grep -E "__vcore_free_(start|end)"   # free must be > 2.3MB
```
  32MB variant = same tree with conf.mk t264 `CFG_TZDRAM_SIZE ?= 0x02000000` and dts `mem-size = <0x02000000>;`.
- Change the Hafnium manifest size (e.g. different S):
```
dtc -I dtb -O dts -o spmc.dts Linux_for_Tegra/bootloader/t264_spmc.dtb   # edit vm1 "optee" mem_size
dtc -I dts -O dtb -o t264_spmc_new.dtb spmc.dts
cp Linux_for_Tegra/bootloader/hafnium_t264.fip.orig hafnium_new.fip
~/Desktop/thor_flash/tools/fiptool update --tos-fw-config t264_spmc_new.dtb hafnium_new.fip
```
  BCT TZDRAM = 0x3000000 + S (S = OP-TEE size). All four sizes must agree (notes/qemu_dev_hafnium_2gb_progress.md table).
- Big-memory TAs must be built with the NEW build's `optee/build/t264/export-ta_arm64` (bget.c fix):
  large_mem_tas/build.sh expects ../optee-srcs/optee: adjust paths + CROSS_COMPILE if rebuilding on x86.
