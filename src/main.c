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

#define NUM_REQUEST_PUSH 1<<20			//30MB request
#define CACHE_SIZE 10 * 1<<20				//10MB

static uint64_t length_key(const uint64_t key) {
	return sizeof(key);
}
static uint64_t length_value(char* value) {
	return sizeof(char) * strlen(value);
}
uint64_t sizeofElement(element_t *e) {
	return sizeof(element_t) + length_key(*((uint64_t*)e->key)) + length_value((char*)(e->value));
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
	lruCache *lru = lruCreate(CACHE_SIZE, sizeofElement);
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
	fprintf(stdout, "elapsed time: %ld ns\n", elapsedTime);
	/*free all*/
	lruFree(lru);
	free(pairs);
}
int main(int argc, char **argv) {
	test_set();
	return 0;
}


