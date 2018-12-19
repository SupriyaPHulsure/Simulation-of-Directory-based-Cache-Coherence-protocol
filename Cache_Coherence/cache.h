/* This file contains a functional simulator of an associative cache with LRU replacement*/
#ifndef GLOBAL_CACHE_H
#define GLOBAL_CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define MAX_LINE  4096
#define WHITE_SPACE " \t\n"
#define LINE_TERMINATOR "\n"
#define MEMORY_LABEL "Mem"
#define MEMORY_SEPARATOR " )(=\n"
#define ENV_CONFIG_FILE "cache.conf"
#define MAX_SIZE 10


struct cache_blk_t { // note that no actual data will be stored in the cache
  unsigned long tag;
  char valid;
  char dirty;
  unsigned LRU;	//to be used to build the LRU stack for the blocks in a cache set
};

struct cache_t {
	// The cache is represented by a 2-D array of blocks. 
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;					// number of sets
  int blocksize;				// block size
  int assoc;					// associativity
  int miss_penalty;				// the miss penalty
  struct cache_blk_t **blocks;	// a pointer to the array of cache blocks
};

typedef struct _request{
int cycle;
int type;
unsigned long address;
int delay;
int nextcycle; // difference between current cycle and next cycle
int possible_nextcycle_no; // cycle_difference + delay will be stored here
}request;

typedef struct _request_list{
int core_id;
int isCompleted;
request *request;
}request_list;

enum Status {
invalid,
shared,
modified
};

typedef struct _block_status {
enum Status status;
int busy; //1 if busy, 0 otherwise
int *bitMap; // list of bools which flags if the correspond node has a valid copy of this block. length should be the number of tiles
} BlockStatus;

struct cache_blk_L2 { // note that no actual data will be stored in the cache
  unsigned long tag;
  char valid;
  unsigned LRU;	//to be used to build the LRU stack for the blocks in a cache set
  BlockStatus blockStatus; //the directory of the block

};

struct cache_L2 {
	// The cache is represented by a 2-D array of blocks.
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;					// number of sets
  int blocksize;				// block size
  int assoc;					// associativity
  int miss_penalty;				// the miss penalty
  struct cache_blk_L2 **blocks;	// a pointer to the array of cache blocks
};

typedef struct _processor{
	int core_id;
	int instructionsCount;
	request_list **requestList;
}processor;

typedef struct _stat{
    int *cyclesPerCore; //the number of cycles needed for completing the execution of each core in the trace. length = number of cores
    int *missL1; // the number of miss in each L1 cache module. length = number of cores
    int *missL2; // the number of miss in each L2 cache module. length = number of cores
    int *accessL1; // the number of access (miss + hit) in each L1 cache module. length = number of cores
    int *accessL2; // the number of access (miss + hit) in each L2 cache module. length = number of cores
    int *missPenalty; // the total miss penalty for each L1 miss in each core. length = number of cores
    int controlMesCounter; // the counter for control messages
    int dataMesCounter; //the counter for data messages
}Stat;

request_list **requestList;
 
processor **processors;


struct cache_t * cache_create(int size, int blocksize, int assoc)
{
  int i, nblocks , nsets ;
  struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));
		
  nblocks = size *1024 / blocksize ;// number of blocks in the cache
  nsets = nblocks / assoc ;			// number of sets (entries) in the cache
  C->blocksize = blocksize ;
  C->nsets = nsets  ; 
  C->assoc = assoc;

  C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

  for(i = 0; i < nsets; i++) {
		C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
	}
  //printf("L1 blocksize: %d, nsets: %d\n", C->blocksize, C->nsets);
  return C;
}


struct cache_L2 * cacheL2_create(int size, int blocksize, int assoc)
{
  int i, nblocks , nsets ;
  struct cache_L2 *C = (struct cache_L2 *)calloc(1, sizeof(struct cache_L2));

  nblocks = size *1024 / blocksize ;// number of blocks in the cache
  nsets = nblocks / assoc ;			// number of sets (entries) in the cache
  C->blocksize = blocksize ;
  C->nsets = nsets ;
  C->assoc = assoc;

  C->blocks= (struct cache_blk_L2 **)calloc(nsets, sizeof(struct cache_blk_L2 *));

  for(i = 0; i < nsets; i++) {
		C->blocks[i] = (struct cache_blk_L2 *)calloc(assoc, sizeof(struct cache_blk_L2));
	}
  //printf("L2 blocksize: %d, nsets: %d\n", C->blocksize, C->nsets);
  return C;
}

int updateLRU(struct cache_t *cp ,int index, int way)
{
	int k ;
	for (k=0 ; k< cp->assoc ; k++)
	{
	  if(cp->blocks[index][k].LRU <= cp->blocks[index][way].LRU)
		 cp->blocks[index][k].LRU = cp->blocks[index][k].LRU + 1 ;
	}
	cp->blocks[index][way].LRU = 0 ;
}

int updateLRU_L2(struct cache_L2 *cp ,int index, int way)
{
	int k ;
	for (k=0 ; k< cp->assoc ; k++)
	{
	  if(cp->blocks[index][k].LRU <= cp->blocks[index][way].LRU)
		 cp->blocks[index][k].LRU = cp->blocks[index][k].LRU + 1 ;
	}
	cp->blocks[index][way].LRU = 0 ;
}

// change the directory of cache L2
void change_cacheL2_dir(struct cache_L2 *cp, unsigned long address, enum Status st, int L_tile, int num_tiles)
{
	int i, j;
	int block_address ;
	int index ;
	int tag ;

	block_address = (address / cp->blocksize);
	tag = block_address / cp->nsets;
	index = block_address - (tag * cp->nsets);

	for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
	  if (cp->blocks[index][i].tag == tag && cp->blocks[index][i].valid == 1)
	  {
	    if (st == modified){
	         //cp->blocks[index][i].blockStatus.bitMap = (int *)calloc(num_tiles, sizeof(int)) ;
             for(j = 0; j < num_tiles; j++) {
                cp->blocks[index][i].blockStatus.bitMap[j] = 0;
             }
	    }
	    cp->blocks[index][i].blockStatus.status = st;
	    cp->blocks[index][i].blockStatus.bitMap[L_tile] = 1;
	    return;
	  }
	}
	assert (0); // It is impossible to have a miss
}

// change the block status of cache L1
void change_cacheL1_status(struct cache_t *cp, unsigned long address, enum Status st)
{
	int i ;
	int block_address ;
	int index ;
	int tag ;

	block_address = (address / cp->blocksize);
	tag = block_address / cp->nsets;
	index = block_address - (tag * cp->nsets);

	for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
	  if (cp->blocks[index][i].tag == tag && cp->blocks[index][i].valid == 1)
	  {
		if (st == shared) cp->blocks[index][i].dirty = 0 ;
		if (st == invalid) cp->blocks[index][i].valid = 0 ;
		return;
	  }
	}
	assert (0); // It is impossible to have a miss
}

// Evict the directory of cache L2
void evict_cacheL2_dir(struct cache_L2 *cp, unsigned long address, int L_tile, int num_tiles)
{
	int i, j;
	int block_address ;
	int index ;
	int tag ;
	int sum_bit = 0;

	block_address = (address / cp->blocksize);
	tag = block_address / cp->nsets;
	index = block_address - (tag * cp->nsets);

	for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
	  if (cp->blocks[index][i].tag == tag && cp->blocks[index][i].valid == 1)
	  {
	    cp->blocks[index][i].blockStatus.bitMap[L_tile] = 0;
	    for(j = 0; j < num_tiles; j++) {
            sum_bit += cp->blocks[index][i].blockStatus.bitMap[j];
        }
	    if (sum_bit == 0){// change the status to uncached
	        enum Status st = invalid;
            cp->blocks[index][i].blockStatus.status = st;
	    }
	    return;
	  }
	}
	assert (0); // It is impossible to have a miss
}


#endif /* GLOBAL_CACHE_H */