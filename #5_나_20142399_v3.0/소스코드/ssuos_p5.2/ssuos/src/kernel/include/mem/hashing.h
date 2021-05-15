#ifndef __HASHING_H__
#define __HASHING_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <type.h>
#include <proc/proc.h>
#include <ssulib.h>

#define SLOT_NUM 4							// The number of slots in a bucket
#define CAPACITY 256						// level hash table capacity

typedef struct entry{
    uint32_t key;
    uint32_t value;
} entry;


//level bucket에 num 변수를 넣어 
//현재 bucket에 있는 원소의 개수를 표현
typedef struct level_bucket
{
    uint8_t token[SLOT_NUM];
    entry slot[SLOT_NUM];
    int num;
} level_bucket;

typedef struct level_hash {
    level_bucket top_buckets[CAPACITY];
    level_bucket bottom_buckets[CAPACITY / 2];
} level_hash;

level_hash hash_table;

void init_hash_table(void);
uint32_t F_IDX(uint32_t addr, uint32_t capacity);	// Get first index to use at table
uint32_t S_IDX(uint32_t addr, uint32_t capacity);	// Get second index to use at table

#endif