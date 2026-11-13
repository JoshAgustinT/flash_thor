/*
 * bigmem <heap|gib> <N> [alloc_MiB]
 *   heap N : open the TA built with TA_DATA_SIZE = N MiB
 *   gib  N : open the TA built with TA_DATA_GIBIBYTE = N (TEEm PbMA)
 * then ask it to malloc+touch alloc_MiB (default 15/16 of its heap, in KiB units).
 * Prints one line: PASS/FAIL, where it failed (open = TA load, alloc = in TA), timings.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tee_client_api.h>

#define CMD_ALLOC_TOUCH 0

static double now_ms(void)
{
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

int main(int argc, char *argv[])
{
	TEEC_UUID uuid = { 0xb16e0000, 0x0000, 0x0000, { 0 } };
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_Result res;
	uint32_t origin;
	unsigned long n, kind;
	unsigned long long heap_kib, alloc; /* KiB */
	double t0, t_open;

	if (argc < 3 || (strcmp(argv[1], "heap") && strcmp(argv[1], "gib"))) {
		fprintf(stderr, "usage: %s <heap|gib> <N> [alloc_MiB]\n", argv[0]);
		return 2;
	}
	kind = !strcmp(argv[1], "gib");
	n = strtoul(argv[2], NULL, 0);
	heap_kib = (kind ? n * 1024ULL : n) * 1024;
	alloc = argc > 3 ? strtoull(argv[3], NULL, 0) * 1024 : heap_kib * 15 / 16;
	uuid.clockSeqAndNode[1] = kind;
	uuid.clockSeqAndNode[6] = (n >> 8) & 0xff;
	uuid.clockSeqAndNode[7] = n & 0xff;

	if (TEEC_InitializeContext(NULL, &ctx) != TEEC_SUCCESS)
		return 1;

	t0 = now_ms();
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC,
			       NULL, NULL, &origin);
	t_open = now_ms() - t0;
	if (res != TEEC_SUCCESS) {
		printf("FAIL %s %lu: open (TA load) res=0x%x origin=%u (%.0f ms)\n",
		       argv[1], n, res, origin, t_open);
		TEEC_FinalizeContext(&ctx);
		return 1;
	}

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = (uint32_t)(alloc << 10);
	op.params[0].value.b = (uint32_t)((alloc << 10) >> 32);
	res = TEEC_InvokeCommand(&sess, CMD_ALLOC_TOUCH, &op, &origin);
	if (res != TEEC_SUCCESS)
		printf("FAIL %s %lu: alloc %llu KiB res=0x%x origin=%u (open %.0f ms)\n",
		       argv[1], n, alloc, res, origin, t_open);
	else
		printf("PASS %s %lu: alloc+touch %llu KiB (open %.0f ms, touch %u ms)\n",
		       argv[1], n, alloc, t_open, op.params[1].value.a);

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	return res != TEEC_SUCCESS;
}
