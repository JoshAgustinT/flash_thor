CFG_TEE_TA_LOG_LEVEL ?= 2
# BINARY (the UUID), O (output dir) and HEAP_MB are passed by ../build.sh.
# CPPFLAGS_ta_arm64 also reaches the dev kit's user_ta_header.c (which uses TA_DATA_SIZE)
CPPFLAGS_ta_arm64 += -DHEAP_MB=$(HEAP_MB)
-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk
