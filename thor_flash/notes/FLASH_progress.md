# FLASH: getting TEEm (../qemu_dev) into this board's OP-TEE

# 2026-11-11 — Workflow plan (not started, nothing verified on hardware yet)
=============================================================================================
Board: Jetson Thor, L4T R39.2.1 (`/etc/nv_tegra_release`), OP-TEE = FF-A SP under Hafnium.
Background on the memory layout and limits: ../TEE/progress.md (2026-11-11 mem limits entry).

## Why this needs a flash (and ../TEE didn't)
- TAs (../TEE/josh_ta, mem_limits) are files in /lib/optee_armtz loaded by the running OP-TEE:
  build on board + copy, no flash. That flow was fine, not a shortcut.
- TEEm changes the OP-TEE core (S2M in core_mmu_lpae.c, PbMA syscall, 64-bit fixes) -> lives in
  the TOS image (tos.img) in QSPI. Plus a bigger secure carve-out -> MB1 BCT + Hafnium manifest.
  Both = reflash the QSPI bootloader partition from an x86 host.

## Step 1: build (on the Thor or on the x86 host, either works)
- NVIDIA OP-TEE source for **R39.2.1** (matching installed release; maybe already in
  ../TEE/jetson-public-srcs).
- Re-apply ../qemu_dev/teem_optee_os.diff by hand (NVIDIA fork is 4.6-based, diff is vs upstream).
- Config in plat-tegra/conf.mk: CFG_TZDRAM_SIZE, L2 tables per TA, core heap.
- Build optee_os for the t264 platform, then package with NVIDIA's TOS script (Hafnium + OP-TEE +
  manifest DTB). Set the new `mem-size` in optee/tegra264-optee.dts.
- "Right targets" matter here: correct platform flavor + NVIDIA's build/packaging scripts, not the
  dev-kit TA flow. Native build on the Thor is fine; output is just a binary.
- TODO: find exact script names and the BCT carve-out file in jetson-public-srcs.

## Step 2: flash (x86 Ubuntu host)
- Host Ubuntu version matters: NVIDIA supports specific versions per release. JetPack 7 / R38-R39
  = 22.04 or 24.04 (believed). Check "host machine requirements" in R39.2.1 release notes first.
  Install natively, not in a VM (USB recovery flashing from VMs is unreliable).
- One-time setup: R39.2.1 Driver Package (BSP) + sample rootfs, extract,
  tools/l4t_flash_prerequisites.sh, apply_binaries.sh.
- Replace stock TOS image in Linux_for_Tegra/bootloader/ with ours.
- Change carve-out size in MB1 BCT config under bootloader/ (must match CFG_TZDRAM_SIZE and
  manifest mem-size).
- Thor into Force Recovery, USB-C to host, flash **QSPI only** with the Thor devkit board config
  (the only "target" picked on the host).
- So: compile here, copy image to the Ubuntu box, run NVIDIA's flash script there.

## Suggested order (isolate failures)
1. Flash **stock** images (proves host setup + flash flow).
2. Flash stock OP-TEE + **bigger carve-out only** (BCT + manifest + CFG_TZDRAM_SIZE).
3. Flash TEEm.
After each: normal boot check + xtest + ../TEE/mem_limits tests.

## Warnings
- Same BSP release as installed (R39.2.1), so the rest of the firmware matches.
- QSPI only; a full flash wipes the NVMe rootfs.
- **Fuses**: Jetson supports fusing (secure boot keys, ODM fuses), one-time programmable,
  irreversible. Only happens if explicitly run: odmfuse.sh, or a `--fuse` / fuse-config flag.
  Normal flashing does not burn fuses. Never run those.
- fTPM, StMM (UEFI vars), LUKS keys run in this same OP-TEE: a bad build can break boot / disk
  unlock. Recovery mode is in boot ROM, so stock images can always be reflashed.
- Back up first: data + stock bootloader/ images.
- Hafnium maps OP-TEE memory at stage 2; unknown if it/the manifest caps SP memory size.

# 2026-11-11 — Host Ubuntu version, Hafnium, NVIDIA's t264 TOS steps
=============================================================================================
## Why a specific Ubuntu
- NVIDIA flash tools = prebuilt x86 binaries + Python scripts, tested only on specific LTS
  releases (host libs, Python version).
- l4t_flash_prerequisites.sh uses apt/Ubuntu package names; apply_binaries.sh uses
  qemu-user-static to chroot into the arm64 rootfs.
- l4t_initrd_flash.sh boots the board into a small Linux over USB and talks to it over USB
  networking + NFS + udev/systemd/NetworkManager -> version differences cause hangs.
- SDK Manager refuses unsupported hosts.
- Other versions often work, but failures are vague ("waiting for device", USB timeouts).

## Which version: **Ubuntu 22.04 x86_64**
- NVIDIA docs disagree: r39.2.1 Quick Start says 20.04/22.04; r39.2(.0) release notes say
  24.04/22.04. 22.04 is the only one in both. (r39.2.1 release notes PDF not checked.)

## Hafnium: mostly transparent, we don't rebuild it
- From jetson-optee-srcs/optee/atf_and_optee_README.txt: on t264, tos.img is just
  OP-TEE (tee-raw.bin) + its Hafnium manifest (tegra264-optee.dtb) packed with ATF's sptool.py:
  > dtc -I dts -O dtb -o ./optee/tegra264-optee.dtb ./optee/tegra264-optee.dts
  > python3 sptool.py -i ./optee/build/t264/core/tee-raw.bin:./optee/tegra264-optee.dtb -o ./tos.img
  Hafnium itself = prebuilt binary in the BSP, separate partition, left stock.
- Build: optee_src_build.sh -p t264 (in jetson-optee-srcs).
- Hafnium only matters via the manifest: mem-size in tegra264-optee.dts must match
  CFG_TZDRAM_SIZE and fit in the BCT carve-out. Unknown: whether Hafnium's own config limits
  SP memory size / which ranges it lets an SP have -> may only show up when flashing step 2.
- Flash only the TOS partition (no full QSPI flash needed when only OP-TEE changes):
  copy tos.img to Linux_for_Tegra/bootloader/tos-optee_t264.img, then
  > sudo UNIFIED_FLASH=1 ./tools/kernel_flash/l4t_initrd_flash.sh --no-flash -k A_sec-partition jetson-agx-thor-devkit internal
  and use the commands from its log to flash that partition (per README).
  Carve-out change (MB1 BCT) is a different partition -> TODO: find which and how to flash it.
- Then copy optee/install/t264/* (client libs, xtest) to the board.

# 2026-11-11 — Docs to read (NVIDIA first), and a correction
=============================================================================================
- Correction to "we don't rebuild Hafnium": NVIDIA's Hafnium page says SPs are declared as vm
  nodes in Hafnium's SPMC manifest (t264_spmc.dts) and changing that = rebuild Hafnium. If that
  manifest also fixes OP-TEE's memory range/size, a bigger carve-out may need a Hafnium rebuild
  too (source: Linux_for_Tegra/source/hafnium_src.tbz2). Check when reading the page.
- NVIDIA r39.2.1 Developer Guide:
  - Hafnium: https://docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/SD/Security/Hafnium.html
  - OP-TEE overview: https://docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/SD/Security/OpTee.html
  - OP-TEE architecture: https://docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/SD/Security/OpTee/Architecture.html
  - Boot architecture (MB1/BCT, carve-outs): https://docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/AR/BootArchitecture.html
  - Flashing support: https://docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/SD/FlashingSupport.html
  - Quick Start (host setup): https://docs.nvidia.com/jetson/archives/r39.2.1/DeveloperGuide/IN/QuickStart.html
  - Local: TEE/jetson-public-srcs/.../jetson-optee-srcs/optee/atf_and_optee_README.txt
- Upstream background:
  - Hafnium SPMC: https://hafnium.readthedocs.io/en/latest/secure-partition-manager/secure-partition-manager.html
  - FF-A manifest fields (mem-size, load-address, ...): https://trustedfirmware-a.readthedocs.io/en/latest/components/ffa-manifest-binding.html
  - TF-A SPM overview: https://tf-a.docs.trustedfirmware.org/en/latest/components/secure-partition-manager.html

# 2026-11-11 — Correction: host Ubuntu = 24.04
=============================================================================================
- Read the r39.2.1 release notes PDF itself (sec 1.1 Platform and Release Information):
  "Host machine Linux distribution for flashing software ... : Ubuntu 24.04 and 22.04".
  Release notes = authoritative; the Quick Start's "20.04 or 22.04" is stale text.
- => Use **Ubuntu 24.04 LTS x86_64** (also matches the board's 24.04 rootfs). Not 26.04: not listed.
- Release notes: https://docs.nvidia.com/jetson/archives/r39.2.1/ReleaseNotes/Jetson_Linux_Release_Notes_r39.2.1.pdf

# 2026-11-12 — Porting qemu_dev changes onto NVIDIA's OP-TEE (recon, read-only)
=============================================================================================
- Tree: TEE/jetson-public-srcs/Linux_for_Tegra/source/jetson-optee-srcs/optee/optee_os (OP-TEE 4.6, R39.2.1).
- teem_optee_os.diff dry run: 13/14 files apply (some hunks offset). 1 hunk fails:
  core_mmu_lpae.c set_user_va_idx(). Reason: NVIDIA's 4.6 predates upstream 058859447
  (2026-07-02, "retry ASLR when no user mapping entry remains"), which added
  ARM32_USER_VA_SLOT_LIMIT = (UINT32_MAX >> 30) + 1 = 4. NVIDIA has the literal `4`.
  Same logic -> apply by hand, use 4 (or add the #define) where our hunk says ARM32_USER_VA_SLOT_LIMIT.
- Already in NVIDIA's tree: CFG_CORE_PREALLOC_EL0_TBLS, ldelf_syscall_map_zi().
- Drop for hardware: teem_qemu.diff (virt.c; carve-out does that job), generic_ram_layout.h hunk
  (core/TA split was for QEMU TF-A's 16MB core region; Thor is CFG_CORE_PHYS_RELOCATABLE=y -> one region).
- teem_build.diff (qemu_v8.mk) values -> plat-tegra/conf.mk t264 section. optee_src_build.sh has
  no extra-flags hook, and conf.mk sets some with plain `=` (env can't override).
- Memory size at runtime comes from OP-TEE manifest mem-size (tegra264-optee.dts), read by boot.c;
  CFG_TZDRAM_SIZE mostly ignored.
- OP-TEE size is stated 4 times: BCT carve-out, Hafnium t264_spmc.dts vm1 mem_size (0x02000000),
  tegra264-optee.dts mem-size, CFG_TZDRAM_SIZE. Bigger carve-out = change all -> Hafnium rebuild likely.
  Manifest addrs (optee 0x89c00000 + 32MB overlaps StMM 0x8B000000; mem_limits showed 0x89C00000 is
  Linux RAM) are placeholders; firmware relocates SPs into the real carve-out, which must hold
  Hafnium + OP-TEE + StMM + mods.
  Risk: Hafnium stage-2 maps SP memory from a fixed page pool; tens of GB may exhaust it. Untested.
- Threads: board has 14 cores, no SMT (lscpu). But conf.mk t264 hardcodes CFG_TEE_CORE_NB_CORE = 28
  (SoC max) and CFG_NUM_THREADS = $(CFG_TEE_CORE_NB_CORE) -> stock build has 28 threads.
  Hafnium vm1 vcpu_count = 28 too. S2M page tables = threads x CFG_NUM_TA_L2_TBLS x 4KB:
  28 x 40 x 4KB ~ 4.4MB vs 14 x 40 x 4KB ~ 2.2MB. CFG_TEE_RAM_VA_SIZE is 4MB.
  Plan: leave NB_CORE = 28 (matches Hafnium), set CFG_NUM_THREADS = 14 in conf.mk (plain `=`, so edit
  the file), and probably still raise CFG_TEE_RAM_VA_SIZE (2.2MB + core image + 2MB heap > 4MB). Unverified that
  NB_CORE could safely drop to 14.
- Core heap: NVIDIA 128KB (upstream 192KB) -> still needs the ~2MB bump.
- CFG_LPAE_ADDR_SPACE_BITS = 38 (256 x 1GB slots) -> enough for 40GB user VA. TAs arm64 only,
  so the "N > 1 breaks 32-bit TAs" limit doesn't apply.
- Build: board has fTPM (/dev/tpm0) -> ENABLE_FTPM_BUILD=yes. No LUKS on this board.
  Script builds optee_os twice; 2nd pass embeds early TAs (luks-srv, cpubl-payload-dec, ftpm),
  rebuilt against our libutee. Native build: CROSS_COMPILE_AARCH64=aarch64-linux-gnu-.
- Existing TAs fine (TA_DATA_GIBIBYTE defaults 0); big-memory TAs must use this build's export-ta_arm64.
- Still unknown (need BSP bootloader/ on x86 host): which BCT file sets the carve-out + MB1 limits
  (size/alignment/<4GB?); whether Hafnium manifest ships prebuilt; which partitions the BCT change needs.

# 2026-11-12 — Recon: everything we'd edit for a big carve-out (read-only)
=============================================================================================
- Sources: OP-TEE read in place (TEE/jetson-public-srcs/.../jetson-optee-srcs). Hafnium + ATF t264 only
  ship as tarballs -> extracted to a /tmp scratch dir to read, nothing in the repo touched.
- Boot chain for secure memory: BCT (MB1/MB2) -> MB2 fills BL31's manifest -> ATF programs the carve-out
  -> Hafnium maps SPs per its manifest -> OP-TEE reads its own manifest.

## 1. BCT (BSP bootloader/, not on this board) — still the biggest unknown
- ATF's fdts/nvidia-t264.dts /boot-params is a template: tzdram-base 0xBAADBAAD, tzdram-size 0,
  gsc-config/gsc@N valid=0. MB2 fills these at boot from the BCT. So the size lives in the BCT only.
- TODO on x86 host: find the carve-out entry in bootloader/ BCT files, its limits (size/align/<4GB?),
  and which partitions that change reflashes.

## 2. ATF (bl31.fip) — probably no edits
- plat_bl31_manifest_parse() reads tzdram-base/size + gsc-config from the MB2-filled manifest;
  plat_memctrl programs the GSC carve-outs from that. Nothing size-specific hardcoded seen
  (TZDRAM_SIZE in tegra_bl31_setup.c is only a default overwritten by the manifest).
- Note: last 1MB of tzdram is used by BL31 as secure shmem (plat_setup.c), carve-out must keep room.

## 3. Hafnium (hafnium.fip = hafnium.bin + t264_spmc.dtb via fiptool)
- Correction: the SPMC manifest is a separate DTB (--tos-fw-config) in the fip, so a size change =
  edit soc/t264/manifests/t264_spmc.dts(0) vm1 mem_size, dtc, fiptool. hafnium.bin rebuild only if
  we change build knobs in soc/t264/BUILD.gn (heap_pages = 1536 = 6MB page pool, max_cpus 28).
- load.c: FF-A SPs are mapped at manifest load_address..+mem_size as-is. But 0x89C00000 is Linux RAM
  on this board -> something (likely MB2) patches the manifests at boot. Unknown, check BSP/docs.
- Page-pool risk mostly gone: arch_mm_is_block_allowed(level <= 2) -> stage-2 uses 1GB/2MB blocks,
  so tens of GB costs few tables if the carve-out is 2MB/1GB aligned.
- vm1 vcpu_count = 28 vs OP-TEE manifest execution-ctx-count = 14 (matches our 14 cores). Leave both.
- Build: needs Hafnium v2.9 prebuilts (clang/gn/ninja/dtc, linux-x64) -> tarball's prebuilts/ is
  empty; build on the x86 host per hafnium_README.txt:
  > ./hafnium_src_build.sh -p t264 -t /tmp/hafnium/prebuilts/linux-x64

## 4. OP-TEE (tos.img)
- teem_optee_os.diff (+ hand fix in set_user_va_idx), conf.mk t264 values (above).
- tegra264-optee.dts: mem-size = new size (boot.c reads load-address + mem-size). load-address is the
  same placeholder 0x89C00000 -> see Hafnium note.

## Edit list (so far)
  BCT carve-out size ............ bootloader/ (TBD)
  Hafnium vm1 mem_size .......... t264_spmc.dts -> dtb -> hafnium.fip
  OP-TEE mem-size ............... tegra264-optee.dts -> tos.img
  CFG_TZDRAM_SIZE + TEEm config . plat-tegra/conf.mk -> tos.img
  TEEm code ..................... teem_optee_os.diff -> tos.img
  ATF ........................... none expected

# 2026-11-12 — Flash plan (from R39.2.1 BSP; board is R39.2.1, slot A)
=============================================================================================
- BSP read from a scratch download (Jetson_Linux_R39.2.1_aarch64.tbz2, 1.29GB, sha1 8fe9a3ac9a98...).
  Board: /etc/nv_tegra_release = R39 REVISION 2.1 -> matches.
- How secure memory is laid out (confirmed):
  - BCT: Linux_for_Tegra/bootloader/tegra264-mb1-bct-carveout-l4t.dtsi
    aux_info@CARVEOUT_TZDRAM { size = 80MB; alignment = 2MB; } (no fixed base). Pulled in by the board's
    MISC_CONFIG (bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts ->
    tegra264-mb1-bct-misc-p3834-xxxx-p4071-xxxx.dtsi), which already overrides GPU_COMPTAGS size the
    same way. Not in MEM_BCT.
  - MB2 (strings in mb2_t264.bin) reads each vm's mem_size from the SPMC DTB, allocates it inside
    TZDRAM ("SP memory allocation failed"), and patches load_address / load-address itself.
    => the 0x89C00000-style addresses are placeholders; we only change SIZES.
  - TZDRAM holds: Hafnium + OP-TEE 32MB + StMM 8MB + mods 1MB + BL31 shmem 1MB (last 1MB).
    Rule: new TZDRAM = 80MB + (new OP-TEE size - 32MB).
- Partitions (flash_l4t_t264_qspi.xml, A/B each):
  A_MB1_BCT  <- MISC_CONFIG etc. (BCT)
  A_secure-hv <- bootloader/hafnium_t264.fip = hafnium.bin + t264_spmc.dtb (tos-fw-config)
  A_sec-partition <- sp_t264.fip built at flash time (tegraflash_impl_t264.py) from
    bootloader/tos-optee_t264.img + standalonemm_jetson.pkg + mods_t264.pkg. Size 2.25MB (lz4).
  ATF (A_atf, bl31_t264.fip) untouched.
- Shipped t264_spmc.dtb == the one inside hafnium_t264.fip == source dts (optee vm1 mem_size 32MB,
  vcpu 28). Shipped tos-optee_t264.img = SPKG with manifest mem-size 32MB, 14 exec ctxs.
- Hafnium needs NO rebuild: only its DTB changes -> fiptool update (prebuilt x86 fiptool in
  atf_t264_src: tools/fiptool/fiptool).

## Stage 0 — host + baseline (x86, Ubuntu 24.04)
  > tar xpf Jetson_Linux_R39.2.1_aarch64.tbz2
  > sudo tar xpf Tegra_Linux_Sample-Root-Filesystem_R39.2.1_aarch64.tbz2 -C Linux_for_Tegra/rootfs
  > cd Linux_for_Tegra && sudo ./tools/l4t_flash_prerequisites.sh && sudo ./apply_binaries.sh
  Board in recovery (USB-C), then prove the flash path with STOCK files:
  > sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
  Boots as before -> flash path works. B slot stays stock throughout (fallback).

## Stage 1 — TEEm OP-TEE, stock 32MB (only A_sec-partition)
  Work on a copy, keep TEE/ untouched:
  > cp -a ~/Desktop/josh/ARMHiT/scripts/TEE/jetson-public-srcs/Linux_for_Tegra/source/jetson-optee-srcs ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs
  > cd ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs/optee/optee_os && patch -p1 < ~/Desktop/josh/ARMHiT/scripts/qemu_dev/teem_optee_os.diff
  Fix the 1 reject (core_mmu_lpae.c set_user_va_idx: ARM32_USER_VA_SLOT_LIMIT -> 4). Drop the
  generic_ram_layout.h hunk (QEMU-only split).
  Edit core/arch/arm/plat-tegra/conf.mk, t264 section, add:
    CFG_NUM_THREADS = 14
    CFG_NUM_TA_L2_TBLS = 40
    CFG_CORE_PREALLOC_EL0_TBLS = y
    CFG_CORE_HEAP_SIZE = 2097152
    CFG_TEE_RAM_VA_SIZE = 0x00800000
  (LPAE_ADDR_SPACE_BITS 38 already enough.) Build natively on the Thor:
  > cd ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs && export CROSS_COMPILE_AARCH64=aarch64-linux-gnu- PKG_CONFIG=/usr/bin/pkg-config
  > ./optee_src_build.sh -p t264 -t
  > dtc -I dts -O dtb -o optee/tegra264-optee.dtb optee/tegra264-optee.dts
  > python3 <atf_t264>/tools/sptool/sptool.py -i optee/build/t264/core/tee-raw.bin:optee/tegra264-optee.dtb -o tos.img
  Host: copy tos.img over Linux_for_Tegra/bootloader/tos-optee_t264.img (keep the original), then
  > sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
  Board: copy optee/install/t264/* (libteec, xtest), run xtest + an existing TA + a small TEEm TA.

## Stage 2 — bigger carve-out (A_MB1_BCT + A_secure-hv + A_sec-partition)
  Pick OP-TEE size S (first 2GB, then e.g. 36GB). TZDRAM = 0x3000000 + S.
    S = 2GB  (0x80000000)  -> TZDRAM <0x0 0x83000000>
    S = 36GB (0x900000000) -> TZDRAM <0x9 0x03000000>
  a) BCT, host Linux_for_Tegra/bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts,
     add next to the existing override:  carveout { aux_info@CARVEOUT_TZDRAM { size = <hi lo>; }; };
  b) Hafnium manifest (on host):
     > dtc -I dtb -O dts -o spmc.dts bootloader/t264_spmc.dtb   # edit vm1 mem_size = S
     > dtc -I dts -O dtb -o t264_spmc.dtb spmc.dts
     > fiptool update --tos-fw-config t264_spmc.dtb bootloader/hafnium_t264.fip
  c) OP-TEE: tegra264-optee.dts mem-size = S, conf.mk t264 CFG_TZDRAM_SIZE = S, rebuild tos.img as Stage 1.
  Flash, one partition per run (recovery mode each time):
  > sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_MB1_BCT jetson-agx-thor-devkit internal
  > sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_secure-hv jetson-agx-thor-devkit internal
  > sudo ./tools/kernel_flash/l4t_initrd_flash.sh -k A_sec-partition jetson-agx-thor-devkit internal
  Check: free -g drops by ~S; xtest; TEEm TA allocating > 1GB.

## Recovery
- Bad A slot: bootloader falls back to stock B after failed boots (A/B on by default; verify with
  nvbootctrl). Worst case: recovery mode + full stock QSPI flash from host
  > sudo ./tools/kernel_flash/l4t_initrd_flash.sh jetson-agx-thor-devkit internal
  ("internal" = QSPI only; NVMe rootfs untouched).

## Unknowns that only flashing answers
- Does MB1 accept a multi-GB TZDRAM (any max / placement limit)? Try 2GB before 36GB.
- MB2's alignment of SP allocations (affects Hafnium block mappings; heap_pages 6MB should cover it).
- tee-raw.bin with TEEm still fits 2.25MB A_sec-partition (bss not in image; expect yes).
- Whether -k A_MB1_BCT alone suffices or the BR "BCT" partition also needs reflash. Fallback: full
  internal flash with the edited files (writes both A and B).
- Correction to Stage 2 sizing: no reason 2GB passes and 36GB fails, except 32-bit size fields
  (>4GB) or MB1 placement limits, and 2GB tests neither. Go straight to the target S; if boot fails,
  bisect (e.g. 6GB = just past 4GB, tells if it's a 32-bit limit).

# 2026-11-12 — What we actually change (simple version)
=============================================================================================
Only OP-TEE is compiled. Everything else is a small edit to files NVIDIA ships in the BSP
(Linux_for_Tegra/ on the x86 host). The flash script then packs, signs and flashes them.
1. OP-TEE (build on the Thor): TEEm patch + conf.mk -> optee_src_build.sh -> sptool -> tos.img
   -> copy over bootloader/tos-optee_t264.img            (flashes to A_sec-partition)
2. Hafnium (no build): change vm1 mem_size in its manifest
  > dtc -I dtb -O dts -o spmc.dts bootloader/t264_spmc.dtb
  > dtc -I dts -O dtb -o t264_spmc.dtb spmc.dts
  > fiptool update --tos-fw-config t264_spmc.dtb bootloader/hafnium_t264.fip   (A_secure-hv)
3. BCT (text edit): TZDRAM size override in
   bootloader/generic/BCT/tegra264-mb1-bct-misc-p3834-0008-p4071-0000.dts; script compiles it (A_MB1_BCT)
- Back up tos-optee_t264.img and hafnium_t264.fip (*.orig) first; stock = restore them + undo the BCT edit.
- BSP = Jetson_Linux_R39.2.1_aarch64.tbz2 (1.3GB), download on the x86 host:
  https://developer.download.nvidia.com/embedded/L4T/r39_Release_v2.1/release/Jetson_Linux_R39.2.1_aarch64.tbz2

# 2026-11-12 — Can a 36GB TZDRAM work? (checked every open layer)
=============================================================================================
Not a limit (read the code):
- ATF: tzdram_base/size are uint64 from the MB2-filled manifest. The one (uint32_t) cast
  (tegra_pm.c -> tegra_memctrl_tzdram_setup) only reaches plat_memctrl_tzdram_setup, a no-op on t264.
  Carve-out HW regs: SIZE_128KB is 32-bit in 128KB units (max 512TB) + BOM_HI for >4GB bases.
  MAX_XLAT_TABLES 75; 36GB maps with 1GB/2MB blocks.
- Hafnium: mem_size read as uint64; stage-2 block mappings (see above).
- OP-TEE: manifest load-address/mem-size read as uint64 into paddr_t/size_t. Relocatable = one
  region: core carved from the start, rest is the pool; phys_mem_ta_alloc falls back to the core
  pool when there's no separate TA pool -> TEEm allocations come from it. (QEMU used 2 regions.)
Side effects of a big TZDRAM (ATF plat_psci_handlers.c):
- Every reboot/poweroff: tegra_clear_tzdram() maps + zeroes ALL of TZDRAM -> slower reboot (seconds).
- Suspend (SC7): ENABLE_SC7_MEM_DIGEST hashes all TZDRAM -> slow suspend; avoid suspending.
- Linux loses the TZDRAM size from RAM.
Can't check (closed binaries): MB1 (allocates carve-outs from the BCT; no readable strings) and MB2.
- No NVIDIA t264 config has any carve-out >= 4GB (largest ~1GB: GPU comptags, display FB;
  TZDRAM 64MB auto / 80MB l4t). So >4GB is untested by NVIDIA, not known-broken.
- Placement: DRAM 0x80000000..~0x2080000000; VPR pinned near the top only with ENABLE_128GB_MEMORY.
  36GB contiguous should fit, but MB1's allocator decides.
=> Flash target S directly. If it doesn't boot: 3GB (under 4GB, still bigger than any NVIDIA
   carve-out) vs 6GB tells a 32-bit limit from a placement/other limit.
- Decision: Stage 2 first with S = 2GB (checks the 3 edits + MB2 SP allocation + TEEm in one region;
  already 2x NVIDIA's largest carve-out). Then 36GB (adds the >4GB question).
  S = 2GB values: BCT TZDRAM size <0x0 0x83000000>; t264_spmc vm1 mem_size <0x80000000>;
  tegra264-optee.dts mem-size <0x0 0x80000000>; conf.mk CFG_TZDRAM_SIZE = 0x80000000.

# 2026-11-12 — Keep a stock image ready to re-flash
=============================================================================================
Before flashing anything modified, have a known-good stock set ready on the x86 host.
- Build stock OP-TEE as well (unpatched TEE/ sources, same optee_src_build.sh -p t264 -t + sptool),
  boot it once and check it works -> confirms our build + packaging steps aren't the problem.
- Keep stock tos-optee_t264.img, hafnium_t264.fip, and the unedited BCT .dts as *.orig in the BSP.
- Write the restore commands down beforehand (same -k per-partition flashes, stock files) so recovery
  is copy-paste. Fallback if that fails: full internal (QSPI) flash; slot B stays stock regardless.

# 2026-11-12 — Switch to the no-syscall diff + Hafnium rehearsal on QEMU + checklist
=============================================================================================
## Use this diff now (replaces teem_optee_os.diff in Stage 1)
- ../qemu_dev_hafnium/hafnium_optee_os.diff: 7 files +38/-23 = S2M (core_mmu_lpae.c, config.mk
  CFG_NUM_TA_L2_TBLS) + 64-bit fixes (tee_mm, mobj, fobj, core_mmu.h idx2va) + bget.c.
  NO PbMA syscall / libutee / user_ta_header changes (../qemu_dev_no_sys: a plain .bss heap works
  to 32GB, see its progress.md), NO generic_ram_layout.h hunk (not used when relocatable).
- Vs OP-TEE 4.6.0 tag: 6/7 files apply clean; same 1-line set_user_va_idx conflict as above
  (`4` vs ARM32_USER_VA_SLOT_LIMIT). NVIDIA's fork not re-checked with this diff yet.
> cd ~/Desktop/josh/ARMHiT/scripts/FLASH/optee-srcs/optee/optee_os && patch -p1 < ~/Desktop/josh/ARMHiT/scripts/qemu_dev_hafnium/hafnium_optee_os.diff
- Big TAs: TA_DATA_SIZE = N GB in user_ta_header_defines.h, built with THIS build's export-ta_arm64
  (bget.c is linked into TAs). Stock TAs keep working (TA ABI unchanged).

## Rehearsal: ../qemu_dev_hafnium (QEMU, OP-TEE as FF-A SP under Hafnium v2.14 = Thor's model)
- OP-TEE took ONE 34GB region from its SP manifest ("Physical secure memory base 0x3000000000 size
  0x880000000"), heap 1MB..32GB PASS, 34GB fails cleanly, Linux devmem -> Bus error, xtest
  152/43572 0 failed. Hafnium needed no changes. Use it to reproduce Thor failures first.
- Its TF-A/QEMU patches stand in for MB2/BCT (QEMU TF-A BL2 read the SP load-address as uint32 and
  mapped 4GB) -> nothing to port, but it's the kind of bug to suspect in MB1/MB2.

## Checklist for the Thor build (check each before flashing)
1. Core VA window: CFG_TEE_RAM_VA_SIZE must hold core image + CFG_CORE_HEAP_SIZE +
   CFG_NUM_THREADS x CFG_NUM_TA_L2_TBLS x 4KB (static xlat_tables_ul1, core_mmu_lpae.c).
   QEMU: 2 thr x 40 = 320KB. Thor: 28 thr x 40 = 4.4MB (stock), 14 x 40 = 2.2MB (plan above).
   Stock tegra default is 4MB -> MUST raise (plan: 8MB with 14 threads; 16MB if keeping 28).
   Too small = LINK error "TEE_RAM_VA_SIZE is too small" (kern.ld.S) -> loud, no flash wasted.
   Check after build: optee/build/t264/core/tee.map, end of .nozi vs TEE_RAM_START + VA size.
   CFG_NUM_TA_L2_TBLS = GB of TA VA: 34 is enough for 32GB TAs; 8 for a <= 6GB first test.
2. Core heap: CFG_CORE_HEAP_SIZE ~2MB (stock 128KB). ~200B per 4 L3 tables -> 32GB TA ~0.9MB.
   Too small = RUNTIME "Page tables are not available" when opening a big TA (not a boot failure).
3. CFG_CORE_PREALLOC_EL0_TBLS=y (already in NVIDIA's tree, make sure it's on).
4. 32-bit size fields: everything open we read is 64-bit (ATF/Hafnium/OP-TEE, above). Remaining
   unknowns are closed MB1/MB2 -> boot fail at S > 4GB = suspect them first (bisect 3GB vs 6GB).
   Cosmetic: TA property gpd.ta.dataSize is uint32 (-Woverflow warning for >= 4GB heaps), ignore.
5. TA RAM = same pool as core (relocatable, one region): a 32GB TA needs S >= 32GB + ~64MB L3
   tables + core + other TAs. QEMU used 34GB for a 32GB TA.
6. Every place stating OP-TEE's size must agree (BCT TZDRAM, t264_spmc vm1 mem_size,
   tegra264-optee.dts mem-size, CFG_TZDRAM_SIZE). On QEMU a mismatch = Hafnium refuses to load the SP.
7. After flash: xtest, an existing TA (fTPM still works: /dev/tpm0), bigmem sweep (copy
   ../qemu_dev_hafnium/large_mem_tas, rebuild against the Thor export-ta_arm64), free -g drop ~S.

# 2026-11-12 — START HERE (next session)
=============================================================================================
- Overview deck: ../qemu_dev_hafnium/large_mem_hafnium.pptx (slides 13-15 = Thor mapping, checklist, stages).
- Stage 1 above still says teem_optee_os.diff -> use ../qemu_dev_hafnium/hafnium_optee_os.diff instead
  (section "Use this diff now"). conf.mk t264: CFG_TEE_RAM_VA_SIZE = 0x00800000 with 14 threads (checklist 1).
- First actions, in order:
  1. Unpack NVIDIA optee sources to FLASH/optee-srcs, patch -p1 hafnium_optee_os.diff, hand-fix the
     set_user_va_idx hunk (untested on NVIDIA's fork; 4.6.0 tag = 1 conflict). Build on the Thor.
  2. Check tee.map: .nozi end within TEE_RAM_START + 8MB.
  3. x86 host: Stage 0 (stock flash path), keep *.orig of tos-optee_t264.img / hafnium_t264.fip / BCT dts.
  4. Stage 1 (32MB, A_sec-partition only), then Stage 2.
- Still unknown: t264_spmc memory nodes (QEMU needed memory@3 or at least had it); MB1/MB2 >4GB.
- Never fuse. QSPI only.

# 2026-11-12 — First flash = 2GB, modeled in ../qemu_dev_hafnium_2gb (supersedes START HERE above)
=============================================================================================
- ../qemu_dev_hafnium_2gb/progress.md "Part 2" is now THE step-by-step for the first flash (S = 2GB):
  exact values for all 4 size places, conf.mk line edits, build/verify/flash/check, recovery.
- QEMU rehearsal with the Thor's knobs (14 threads, 38-bit VA, ASLR, 40 L2 tbls, 2MB heap, 8MB VA,
  2GB region): heap 1..1920 MB PASS, 2048 fails cleanly, xtest 0 failed, devmem Bus error.
- CORRECTION to checklist item 1: OP-TEE is relocatable (Thor too) -> TA L2 tables come from
  boot_mem_alloc() out of free core VA, not the static array. Too small CFG_TEE_RAM_VA_SIZE =
  BOOT PANIC, not a link error. QEMU: 2.3MB used of 5.2MB free with 14 threads (8MB VA).
  Check __vcore_free_start/end in the Thor tee.elf before flashing.
