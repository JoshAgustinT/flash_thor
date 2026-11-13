CC ?= $(CROSS_COMPILE)gcc

bigmem: main.c
	$(CC) -Wall -O2 -I$(TEEC_EXPORT)/include -o $@ $< -L$(TEEC_EXPORT)/lib -lteec

clean:
	rm -f bigmem
