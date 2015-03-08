/*
 * lru.h
 *
 *  Created on: Mar 5, 2015
 *      Author: hiepnm
 */

#ifndef LIBS_LRU_H_
#define LIBS_LRU_H_
#include <stdint.h>
#include <pthread.h>
#define MAX_PAYLOAD_KEY 10
#define MAX_PAYLOAD_VALUE 1024
#define MAX_ELEMENT 512
struct lruCache;

typedef struct element_t {
	void *key;
	void *value;
	struct element_t *next;
	struct element_t *prev;
} element_t;
typedef uint64_t sizeOfElement(element_t *e);
typedef struct lruCache {
	uint64_t maxMem;		//capacity of cache
	uint64_t mem;			//current memory of cache
	uint64_t maxElement;
	uint64_t used;			//number of elements are used
	element_t **table;
	element_t *head;
	element_t *tail;
	sizeOfElement *memElement;
	pthread_mutex_t mutex;
} lruCache;
/*API*/
lruCache* lruCreate(uint64_t maxMem, sizeOfElement *memElement);
int lruFree(lruCache *lru);
int lruSet(lruCache *lru, void *key, void *value);
int lruRemove(lruCache *lru, const void* key);
element_t *lruGet(lruCache *lru, const void* key);

#endif /* LIBS_LRU_H_ */
