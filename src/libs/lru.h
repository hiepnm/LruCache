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
#define MIN_PAYLOAD_KEY 10			//minimum size of a key (optional)
#define MIN_PAYLOAD_VALUE 1024		//minimum size of a value (optional)
#define MIN_ELEMENTS 512			//minimum number of elements of a cache

/************************************** Define for ERROR HANDLING ***********************************/
#define MAX_OF_ERRNO_ON_SYSTEM 1000	//real max is 134 (see in /usr/include/asm-generic/errno.h)
typedef enum {
	ERR_LRU_NULL = MAX_OF_ERRNO_ON_SYSTEM,
	ERR_LRU_CANNOT_CREATE,
	ERR_LRU_KEY_NULL,
	ERR_LRU_VALUE_NULL,
	ERR_LRU_KEY_EXISTED,
	ERR_LRU_NO_ERROR

} lruErrorCode;
#define ERM_LRU_NULL "lru cache is NULL"
#define ERM_LRU_CANNOT_CREATE "lru can not create with this memory size"
#define ERM_LRU_KEY_NULL "key is NULL"
#define ERM_LRU_VALUE_NULL "value is NULL"
#define ERM_LRU_KEY_EXISTED "key is existed"
#define ERM_LRU_NO_ERROR "no error"
/*****************************************************************************************************/

struct lruCache;
typedef struct element_t {
	void *key;
	void *value;
	struct element_t *next;
	struct element_t *prev;
} element_t;
typedef uint64_t sizeOfElement(element_t *e);
typedef uint64_t sizeOfKey(const void *key);
typedef struct lruCache {
	uint64_t maxMem;		//capacity of cache
	uint64_t mem;			//current memory of cache
	uint64_t maxElement;
	uint64_t used;			//number of elements are used
	element_t **table;
	element_t *head;
	element_t *tail;
	sizeOfElement *memElement;	//size of an element callback
	sizeOfKey *lenKey;			//size of a key callback
	pthread_mutex_t mutex;
} lruCache;

/********************************* API prototypes ****************************************/
lruCache* lruCreate(uint64_t maxMem, sizeOfElement *memElement, sizeOfKey *lenKey);
int lruFree(lruCache *lru);
int lruSet(lruCache *lru, void *key, void *value);
int lruRemove(lruCache *lru, const void* key);
element_t *lruGet(lruCache *lru, const void* key);
char *lruError(int errorNum);
/*******************************************************************************************/
#endif /* LIBS_LRU_H_ */
