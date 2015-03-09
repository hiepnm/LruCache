/*
 * lru.c
 *
 *  Created on: Mar 5, 2015
 *      Author: hiepnm
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "lru.h"
//static uint32_t dict_hash_function_seed = 5381;

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
		if (!key) errno = ERR_LRU_KEY_NULL;
		else if (!value) errno = ERR_LRU_VALUE_NULL;
		return NULL;
	}
	element_t *e = (element_t*) malloc(sizeof(element_t));
	if (!e) {
		return NULL;
	}
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

uint64_t hashFunction(lruCache *lru, const void* key) {
	uint32_t hash = 5381; //use jdb2 hash function.
	int c;
	uint64_t lenKey = lru->lenKey(key);
	while (lenKey--){
		c = (*(uint64_t*)key)++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash%lru->maxElement;
//	return (*(uint64_t*)key)%lru->maxElement;
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
lruCache* lruCreate(uint64_t maxMem, sizeOfElement *memElement, sizeOfKey *lenKey) {
	uint64_t maxElement = maxMem / (sizeof(element_t) + MIN_PAYLOAD_KEY + MIN_PAYLOAD_VALUE);
	if (maxElement < MIN_ELEMENTS || maxMem < lruOverhead()) {
		errno = ERR_LRU_CANNOT_CREATE;	//set for errno
		return NULL;
	}
	lruCache *lru = (lruCache*) malloc(sizeof(lruCache));
	if (!lru) {//errno was set
		return NULL;
	}
	lru->maxMem = maxMem;
	lru->memElement = memElement;
	lru->lenKey = lenKey;
	lru->mem = 0;
	lru->used = 0;
	lru->maxElement = maxElement;
	lru->table = (element_t**) malloc(sizeof(element_t*) * lru->maxElement);
	if (!lru->table) {
		free(lru);
		return NULL;
	}
	lru->head = NULL;
	lru->tail = NULL;

	int rc = pthread_mutex_init(&(lru->mutex), NULL);
	if (rc == -1) {	//errno was set
		free(lru);
		return NULL;
	}
	//pthread_mutex_init(mutex, NULL);
	return lru;
}

int lruFree(lruCache *lru) {
	if (!lru) {
		errno = ERR_LRU_NULL;
		return -1;
	}
	element_t *e = lru->head;
	while(e) {
		lru->head = e->next;
		elementFree(e);
		e = lru->head;
	}
	int rc = pthread_mutex_destroy(&(lru->mutex));
	if (rc == -1) {
		return -1;
	}
	free(lru);
	return 0;
}

int lruSet(lruCache *lru, void *key, void *value) {
	if (!lru) {
		if (!lru) errno = ERR_LRU_NULL;
		return -1;
	}
	int ret;
	uint64_t idx = hashFunction(lru, key);
	element_t *e = lru->table[idx];
	if (e) {	//update
		ret = lruPushExistingBack(lru, e);
	} else {	//new
		e = elementCreate(key, value);
		if (!e)
			return -1;
		int rc = pthread_mutex_lock(&lru->mutex);
		if (rc == -1) return -1;
		ret = lruAdd(lru, idx, e);
		rc = pthread_mutex_unlock(&lru->mutex);
		if (rc == -1) return -1;
	}
	return ret;
}

int lruRemove(lruCache *lru, const void* key) {
	if (!lru || !key) {
		if (!lru) errno=ERR_LRU_NULL;
		else if (!key) errno=ERR_LRU_KEY_NULL;
		return -1;
	}
	uint64_t idx = hashFunction(lru, key);
	int rc;
	rc = pthread_mutex_lock(&lru->mutex);
	if (rc == -1) return -1;
	element_t *e = lru->table[idx];
	if (e) {
		detach(lru, e);
		free(e);
	}
	rc = pthread_mutex_unlock(&lru->mutex);
	if (rc == -1) return -1;
	return 0;
}

element_t *lruGet(lruCache *lru, const void* key) {
	if (!lru || !key) {
		if (!lru) errno=ERR_LRU_NULL;
		else if (!key) errno=ERR_LRU_KEY_NULL;
		return NULL;
	}
	uint64_t idx = hashFunction(lru, key);
	int rc;
	rc = pthread_mutex_lock(&lru->mutex);
	if (rc == -1) return NULL;
	element_t *e = lru->table[idx];
	lruPushExistingBack(lru, e);
	rc = pthread_mutex_unlock(&lru->mutex);
	if (rc == -1) return NULL;
	return e;
}

char *_lruError(int errorNum) {
	char *ret = (char*)malloc(100*sizeof(char));
	if (!ret) {
		perror("Memory for Error");
		exit(EXIT_FAILURE);
	}
	switch (errorNum) {
	case ERR_LRU_NULL: strcpy(ret, ERM_LRU_NULL); break;
	case ERR_LRU_CANNOT_CREATE: strcpy(ret, ERM_LRU_CANNOT_CREATE); break;
	case ERR_LRU_KEY_NULL: strcpy(ret, ERM_LRU_KEY_NULL); break;
	case ERR_LRU_VALUE_NULL: strcpy(ret, ERM_LRU_VALUE_NULL); break;
	case ERR_LRU_KEY_EXISTED: strcpy(ret, ERM_LRU_KEY_EXISTED); break;
	default: strcpy(ret, ERM_LRU_NO_ERROR);
	}
	return ret;
}

char *lruError(int errorNum) {
	return errorNum < MAX_OF_ERRNO_ON_SYSTEM ? strerror(errorNum) : _lruError(errorNum);
}
