/*
 * lru.c
 *
 *  Created on: Mar 5, 2015
 *      Author: hiepnm
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "lru.h"
//static uint32_t dict_hash_function_seed = 5381;
//#include <pthread.h>

element_t *elementCreate(void *key, void *value);
int elementFree(element_t *e);
int lruAdd(lruCache *lru, uint64_t idx, element_t *e);
int lruPushBack(lruCache *lru, element_t* e); //push the newest element to the double linked list
int lruPushExistingBack(lruCache *lru, element_t* e); //push the existing element to the tail of the double linked list
element_t *lruFront(lruCache *lru);	//get the oldest element of the double linked list
int lruPopFront(lruCache *lru);
uint64_t lruOverhead();

element_t *elementCreate(void *key, void *value) {
	if (!key || !value) {
		return NULL;
	}
	element_t *e = (element_t*) malloc(sizeof(element_t));
	e->key = key;
	e->value = value;
	e->next = NULL;
	e->prev = NULL;
	return e;
}
int elementFree(element_t *e) {
	free(e->key);
	free(e->value);
	free(e);
	return 0;
}

uint64_t hashFunction(const void* key, uint64_t len) {
	return (*(uint64_t*)key)%len;
}

uint64_t sizeOfLruCache(lruCache *lru) {
	uint64_t memAllElement = 0;
	element_t *e = lru->head;
	while (e) {
		memAllElement += lru->memElement(e);
		e = e->next;
	}
	return memAllElement + lruOverhead();
}
uint64_t lruOverhead() {
	return sizeof(lruCache);
}
int lruAdd(lruCache *lru, uint64_t idx, element_t *e) {
	if (lru->memElement(e) + lru->mem > lru->maxMem) {	//full size of cache
		lruPopFront(lru);								//remove the oldest element
		return lruAdd(lru, idx, e);						// add the newest again
	}
	/*add the newest*/
	lru->mem += lru->memElement(e);
	lru->table[idx] = e;
	lruPushBack(lru, e);
	return 0;
}

int lruPushBack(lruCache *lru, element_t *e) {
	if (!lru || !e || lru->used >= lru->maxElement) {
		return -1;
	}
	if (!lru->head && !lru->tail) {
		lru->head = e;
		lru->tail = e;
	} else {
		lru->tail->next = e;
		lru->tail = e;
	}
	lru->used++;
	return 0;
}
void _lruPushExistingBack(lruCache *lru, element_t *e) {
	lru->tail->next = e;
	e->prev = lru->tail;
	e->next = NULL;
	lru->tail = e;
}
void detach(lruCache *lru, element_t *e) {
	if (e->next) {
		e->next->prev = e->prev;
	} else {
		lru->tail = e->prev;
	}
	if (e->prev) {
		e->prev->next = e->next;
	} else {
		lru->head = e->next;
	}
}

int lruPushExistingBack(lruCache *lru, element_t *e) {
	if (!lru || !e) {
		return -1;
	}
	//detach(lru, e)
	//_lruPushExistingBack(lru, e)
	if (e->next) {
		if (e->prev) {
			e->prev->next = e->next;
		} else {
			lru->head = e->next;
		}
		e->next->prev = e->prev;
		_lruPushExistingBack(lru, e);
	}
	return 0;
}

element_t *lruFront(lruCache *lru) {
	if (!lru) {
		return NULL;
	}
	return lru->head;
}

int lruPopFront(lruCache *lru) {
	if (!lru || lru->used == 0) {
		return -1;
	}
	element_t* e = lru->head;
	lru->head = lru->head->next;
	lru->head->prev = NULL;
	lru->mem -= lru->memElement(e);
	free(e);
	return 0;
}
//pthread_mutex_t *mutex;
/************************* API Implementation ********************************************/
lruCache* lruCreate(uint64_t maxMem, sizeOfElement *memElement) {
	uint64_t maxElement = maxMem / (sizeof(element_t) + MAX_PAYLOAD_KEY + MAX_PAYLOAD_VALUE);
	if (maxElement < MAX_ELEMENT || maxMem < lruOverhead()) {
		return NULL;
	}
	lruCache *lru = (lruCache*) malloc(sizeof(lruCache));
	lru->maxMem = maxMem;
	lru->memElement = memElement;
	lru->mem = 0;
	lru->used = 0;
	lru->maxElement = maxElement;
	lru->table = (element_t**) malloc(sizeof(element_t*) * lru->maxElement);
	lru->head = NULL;
	lru->tail = NULL;
	//pthread_mutex_init(mutex, NULL);
	return lru;
}

int lruFree(lruCache *lru) {
	if (!lru) {
		return -1;
	}
	element_t *e = lru->head;
	while(e) {
		lru->head = e->next;
		elementFree(e);
		e = lru->head;
	}
	free(lru);
	return 0;
}

int lruSet(lruCache *lru, void *key, void *value) {
	uint64_t idx = hashFunction(key, lru->maxElement);
	if (!lru || lru->table[idx]) {
		return -1;
	}
	element_t *e = elementCreate(key, value);
	if (!e) {
		return -1;
	}
	//pthread_mutex_lock(mutex);
	int ret = lruAdd(lru, idx, e);
	//pthread_mutex_unlock(mutex);
	return ret;
}

int lruRemove(lruCache *lru, const void* key) {
	if (!lru || !key) {
		return -1;
	}
	uint64_t idx = hashFunction(key, lru->maxElement);
	//pthread_mutex_lock(mutex);
	element_t *e = lru->table[idx];
	if (e) {
		detach(lru, e);
		free(e);
	}
	//pthread_mutex_unlock(mutex);
	return 0;
}

element_t *lruGet(lruCache *lru, const void* key) {
	if (!lru || !key) {
		return NULL;
	}
	uint64_t idx = hashFunction(key, lru->maxElement);
	//pthread_mutex_lock(mutex);
	element_t *e = lru->table[idx];
	lruPushExistingBack(lru, e);
	//pthread_mutex_unlock(mutex);
	return e;
}
