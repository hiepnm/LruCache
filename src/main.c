/*
 * main.c
 *
 *  Created on: Mar 5, 2015
 *      Author: hiepnm
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "libs/lru.h"

#define ONE_MEGABYTE 1<<20
#define NUM_REQUEST_PUSH ONE_MEGABYTE			//30MB request
#define CACHE_SIZE 10 * ONE_MEGABYTE				//10MB

static uint64_t length_key(const void *key) {
	return sizeof(uint64_t);
}
static uint64_t length_value(char* value) {
	return sizeof(char) * strlen(value);
}
uint64_t sizeofElement(element_t *e) {
	return sizeof(element_t) + length_key(e->key) + length_value((char*)(e->value));
}
typedef struct pairtest_t {
	uint64_t *key;
	char *value;
} pairtest_t;
void test_set() {
	uint64_t i;
	pairtest_t *pairs = (pairtest_t*)malloc(sizeof(pairtest_t) * NUM_REQUEST_PUSH);
	for (i = 0; i < NUM_REQUEST_PUSH; i++) {
		pairs[i].key = (uint64_t*)malloc(sizeof(uint64_t));
		*(pairs[i].key) = i;
		pairs[i].value = (char*)malloc(11*sizeof(char));
		strcpy(pairs[i].value, "0123456789");
	}/*all = 50MB*/
	lruCache *lru = lruCreate(CACHE_SIZE, sizeofElement, length_key);
	fprintf(stderr, "lruCache: %p\n",lru);
	fprintf(stderr, "memory size = %ld MB\n",(lru->maxMem)/ONE_MEGABYTE);
	fprintf(stderr, "maxElement = %ld\n",lru->maxElement);
	fprintf(stderr, "-----------------------------------------------\n");
	clock_t start = clock();
	int rc;
	for (i = 0; i < NUM_REQUEST_PUSH; i++) {
		rc = lruSet(lru, pairs[i].key, pairs[i].value);
		if (rc==-1){
			fprintf(stderr, "lruSet: %s\n", lruError(errno));
			break;
		}
	}
	clock_t elapsedTime = clock() - start;
	fprintf(stderr, "elapsed time: %ld ns\n", elapsedTime);
	/*free all*/
	lruFree(lru);
	free(pairs);
}
int main(int argc, char **argv) {
	test_set();
	return 0;
}


