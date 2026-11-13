#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

#include <bigmem_ta.h>

#define TA_UUID			TA_BIGMEM_UUID
#define TA_FLAGS		0
#define TA_STACK_SIZE		(4 * 1024)

#define TA_DATA_SIZE		(HEAP_MB * 1024UL * 1024UL)

#define TA_VERSION		"1.0"
#define TA_DESCRIPTION		"TEEm large memory test TA"

#endif
