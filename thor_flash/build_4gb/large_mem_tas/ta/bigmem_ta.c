#include <stdlib.h>
#include <tee_internal_api.h>
#include <bigmem_ta.h>

TEE_Result TA_CreateEntryPoint(void) { return TEE_SUCCESS; }
void TA_DestroyEntryPoint(void) { }
TEE_Result TA_OpenSessionEntryPoint(uint32_t pt __unused,
				    TEE_Param p[4] __unused,
				    void **s __unused) { return TEE_SUCCESS; }
void TA_CloseSessionEntryPoint(void *s __unused) { }

static TEE_Result alloc_touch(uint32_t pt, TEE_Param p[4])
{
	const uint32_t exp = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
					     TEE_PARAM_TYPE_VALUE_OUTPUT,
					     TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE);
	size_t len = ((uint64_t)p[0].value.b << 32) | p[0].value.a;
	TEE_Time t0 = { }, t1 = { };
	uint64_t *buf = NULL;
	size_t i = 0;
	const size_t step = 4096 / sizeof(uint64_t);	/* one word per page */

	if (pt != exp)
		return TEE_ERROR_BAD_PARAMETERS;

	buf = malloc(len);
	if (!buf) {
		EMSG("malloc(%zu MiB) failed", len >> 20);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_GetSystemTime(&t0);
	for (i = 0; i < len / sizeof(uint64_t); i += step)
		buf[i] = i ^ 0x5a5a5a5a5a5a5a5aULL;
	for (i = 0; i < len / sizeof(uint64_t); i += step) {
		if (buf[i] != (i ^ 0x5a5a5a5a5a5a5a5aULL)) {
			EMSG("mismatch at word %zu", i);
			free(buf);
			return TEE_ERROR_GENERIC;
		}
	}
	TEE_GetSystemTime(&t1);
	IMSG("alloc+touch %zu MiB at %p OK", len >> 20, (void *)buf);
	free(buf);
	p[1].value.a = (t1.seconds - t0.seconds) * 1000 + t1.millis - t0.millis;
	return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void *s __unused, uint32_t cmd,
				      uint32_t pt, TEE_Param p[4])
{
	if (cmd == TA_BIGMEM_CMD_ALLOC_TOUCH)
		return alloc_touch(pt, p);
	return TEE_ERROR_BAD_PARAMETERS;
}
