/*
 * bigmem test TA: one source, built many times with a different memory size.
 *
 *   KIND=0 "heap": TA_DATA_SIZE = HEAP_MB MiB  (classic .bss heap, vanilla OP-TEE way)
 *   KIND=1 "gib" : TA_DATA_GIBIBYTE = GIB      (TEEm PbMA: heap mapped by syscall at init)
 *
 * UUID = b16e0000-0000-0000-KKKK-00000000NNNN, K = kind, N = HEAP_MB or GIB (hex).
 */
#ifndef BIGMEM_TA_H
#define BIGMEM_TA_H

#ifdef GIB
#define BIGMEM_KIND	1
#define BIGMEM_N	GIB
#else
#define BIGMEM_KIND	0
#define BIGMEM_N	HEAP_MB
#endif

#define TA_BIGMEM_UUID \
	{ 0xb16e0000, 0x0000, 0x0000, \
	  { 0x00, BIGMEM_KIND, 0x00, 0x00, 0x00, 0x00, \
	    ((BIGMEM_N) >> 8) & 0xff, (BIGMEM_N) & 0xff } }

/*
 * CMD_ALLOC_TOUCH: params[0].value = bytes to malloc (a = low 32 bits, b = high).
 * The TA mallocs it, writes every 4KB page, reads it back, frees it.
 * params[1].value.a = ms spent writing+verifying.
 */
#define TA_BIGMEM_CMD_ALLOC_TOUCH	0

#endif
