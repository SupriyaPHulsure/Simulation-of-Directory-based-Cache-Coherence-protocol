
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "cache.h"


void Loadtracefile (char *fileName, int p);
void printallrequests();
int calculateHome(unsigned long address, int p);
void InitializeCache(int Notiles, int L1size, int L2size, int blocksize, int L1assoc, int L2assoc);
void InitializeStat(int num_tiles);
void IssueRequest(int p, int c);
int IssueRead(request_list *current_request, int p);
int IssueWrite(request_list *current_request, int p);
void loadProcessor(int p);
void printallrequestsperprocessors(int p);
void printStatistics(int p);
void printAllBlockStatus();

int size_list = 0;
struct cache_t **arr_L1_cache;
struct cache_L2 **arr_L2_cache;
Stat *stat_counter;
int flag = 1;
int parameters[9];
int debug = 0;

int main(int argc, char** argv) {
	//Validate command line argument

	FILE *file = fopen ( ENV_CONFIG_FILE, "r" );
    char config[64];
    int i,temp;
    for (i = 0; i < 9; i++) {
        fgets(config, 64, file);
		//printf("%s", config);
        strtok(config, "=");
        temp = atoi(strtok(NULL, ";"));		
        parameters[i] = temp;
    }
    fclose(file);
	
	int p = parameters[0];
    int n1 = parameters[1];
    int n2 = parameters[2];
    int b = parameters[3];
    int a1 = parameters[4];
	int a2 = parameters[5];
    int C = parameters[6];
    int d = parameters[7];
    int d1 = parameters[8];

	printf ("parameters %d  %d %d %d  %d %d %d  %d %d\n", p, n1, n2, b, a1, a2, C, d, d1);
	
	//printf("%s\n", argv[1]);
	//if(b == 2 && argc > 2 && strcmp(argv[2],"-d")==0) {
	if(argc > 2 && strcmp(argv[2],"-d")==0) { // TODO: change back
        debug = 1;
	}
	Loadtracefile(argv[1], (int)pow(2,p));
	InitializeCache((int)pow(2,p), (int)pow(2,n1), (int)pow(2,n2), (int)pow(2,b), (int)pow(2,a1), (int)pow(2,a2));
	InitializeStat((int)pow(2,p));
	IssueRequest(p, C);
	printStatistics(p);
	if (debug) {
        printAllBlockStatus();
	}
	//printf("delay: %d\n", mesh_message_delay(0,3));
	return 0;
}

/**
 * Loads all requests from input trace file to RequestList data structure
 */
void Loadtracefile (char *fileName, int p) {
	int j = 0;
	char *line = (char *) malloc (sizeof(char) * MAX_LINE);
	request *request1 = (request *) malloc (sizeof(request)); 
	requestList = (request_list **) calloc (MAX_SIZE, sizeof(request_list *)); 
	
	for(j = 0; j < MAX_SIZE; j++) {
		requestList[j] = (request_list *) malloc (sizeof(request_list));
	}
	char *memoryAddress;
	int cycle_no;
	int core_id;
	int typeOfrequest, i = 0;
    long addr;
	char *ptr;

	size_t len = 0;
	ssize_t read;

	int currentAddress;

	FILE *fp;

	if ((fp = fopen(fileName, "r")) == NULL) {
		perror ("Error to open the trace file...");
		exit (EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) { //loop to read file line by line and tokenize
		//strcpy (tempLine, line);
		if(i < MAX_SIZE){
			if ((cycle_no = atoi(strtok(line, WHITE_SPACE))) != 0){
			
				core_id = atoi(strtok(NULL, WHITE_SPACE));
				typeOfrequest = atoi(strtok(NULL, WHITE_SPACE));
				memoryAddress = strtok(NULL, WHITE_SPACE);
				
				request1 -> address = strtoul(memoryAddress, NULL, 16);
				request1 -> type = typeOfrequest;
				request1 -> cycle = cycle_no;
				request1 -> delay = 0;
				requestList[i] -> request = request1;
				requestList[i] -> core_id = core_id;
				requestList[i] -> isCompleted = 0;
				//printf("%d\t %d\t %d\t %s\n", cycle_no, core_id, typeOfrequest, memoryAddress);
			}
		}
		else{
			requestList = (request_list **) realloc (requestList, sizeof(request_list*)* (i+1)); 
			requestList[i] = (request_list *) malloc (sizeof(request_list));
			
			if ((cycle_no = atoi(strtok(line, WHITE_SPACE))) != 0){
			
				core_id = atoi(strtok(NULL, WHITE_SPACE));
				typeOfrequest = atoi(strtok(NULL, WHITE_SPACE));
				memoryAddress = strtok(NULL, WHITE_SPACE);
				
				request1 -> address = strtoul(memoryAddress, NULL, 16);
				request1 -> type = typeOfrequest;
				request1 -> cycle = cycle_no;
				request1 -> delay = 0;
				requestList[i] -> request = request1;
				requestList[i] -> core_id = core_id;
				requestList[i] -> isCompleted = 0;

				//printf("%d\t %d\t %d\t %s\n", cycle_no, core_id, typeOfrequest, memoryAddress);
			}
			
		}
		line = (char *) malloc (sizeof(char) * MAX_LINE);
		request1 = (request *) malloc (sizeof(request ));
		i++;
	}
	printf("total number of lines in input file is %d\n", i);
	size_list = i;
	if (fp)
		fclose(fp);

	//printallrequests();
	loadProcessor(p);
}

/**
 * Initializes Cache function creates array of L1 and L2 cache with size equal to no of tiles.
 */
void InitializeCache(int Notiles, int L1size, int L2size, int blocksize, int L1assoc, int L2assoc){
	arr_L1_cache = realloc(arr_L1_cache, Notiles * sizeof(struct cache_t *));
	arr_L2_cache = realloc(arr_L2_cache, Notiles * sizeof(struct cache_t *));
	int i;
	for (i = 0; i< Notiles; i++){
		arr_L1_cache[i] = (struct cache_t *) malloc(sizeof(struct cache_t));
		struct cache_t *L1;
		L1 = cache_create(L1size, blocksize, L1assoc);
		arr_L1_cache[i] = L1;
	}
	for (i = 0; i< Notiles; i++){
		arr_L2_cache[i] = (struct cache_L2 *) malloc(sizeof(struct cache_L2));
		struct cache_L2 *L2;
		L2 = cacheL2_create(L2size, blocksize, L2assoc);
		arr_L2_cache[i] = L2;
	}
}

/**
 * Initializes Statistic counters
 */
void InitializeStat(int num_tiles){
    stat_counter = (Stat *) malloc (sizeof(Stat));
    stat_counter->cyclesPerCore = (int *) calloc (num_tiles, sizeof(int));
    stat_counter->missL1 = (int *) calloc (num_tiles, sizeof(int));
    stat_counter->missL2 = (int *) calloc (num_tiles, sizeof(int));
    stat_counter->accessL1 = (int *) calloc (num_tiles, sizeof(int));
    stat_counter->accessL2 = (int *) calloc (num_tiles, sizeof(int));
    stat_counter->missPenalty = (int *) calloc (num_tiles, sizeof(int));
    stat_counter->controlMesCounter = 0;
    stat_counter->dataMesCounter = 0;
	int i;
	for (i = 0; i< num_tiles; i++){
	    stat_counter->cyclesPerCore[i] = 0;
        stat_counter->missL1[i] = 0;
        stat_counter->missL2[i] = 0;
        stat_counter->accessL1[i] = 0;
        stat_counter->accessL2[i] = 0;
        stat_counter->missPenalty[i] = 0;
	}
}


void IssueRequest(int p, int c){
    long int cycle_counter = 0;
    long int next_cycle;
	int i, j, k, cores, counter = 0, delay = 0;
	cores = (int)pow(2,p);
	request_list *current_request = (request_list *) malloc (sizeof(request_list));
	int *index_request = (int*) calloc (cores, sizeof(int));
	for (k = 0; k < cores; k++){
	    index_request[k] = 0;
	}
	//current_request = processors[j] -> requestList[0];
	int iter = 0;
    while(iter < size_list){
        //if (iter%1000 == 0) printf("Request the %d000 line: \n", (int) iter/1000);
        if (debug) printf("Cycle: %lu ---\n", cycle_counter);
        long int min_next_cycle = 0;
        for (j = 0; j < cores; j++) {
            i = index_request[j];
            if (i < processors[j] -> instructionsCount){
                current_request = processors[j] -> requestList[i];
                if (current_request-> request -> possible_nextcycle_no == cycle_counter){
                    if(current_request -> request -> type == 0){
                        delay = IssueRead(current_request, p);
                    }else{
                        delay = IssueWrite(current_request, p);
                    }
                    //printf("i - %d\t s - %d\n",i, processors[j] -> instructionsCount );
                    if( i+1 < processors[j] -> instructionsCount){
                        processors[j] -> requestList[i+1] -> request -> possible_nextcycle_no = current_request-> request -> possible_nextcycle_no + processors[j] -> requestList[i+1] -> request -> nextcycle + delay;

                    }else{
                        stat_counter->cyclesPerCore[j] = current_request-> request -> possible_nextcycle_no + delay;
                    }
                    index_request[j] += 1;
                    iter++;
                    if (debug) {
                        printallrequestsperprocessors(cores);
                    }
                }
                if (index_request[j] < processors[j] -> instructionsCount){
                    next_cycle = processors[j] -> requestList[index_request[j]]-> request -> possible_nextcycle_no;
                    if (min_next_cycle == 0) min_next_cycle = next_cycle;
                    else{
                        if (next_cycle < min_next_cycle)
                            min_next_cycle = next_cycle;
                    }
                }
            }
        }
        cycle_counter = min_next_cycle;
	}
}

/**
 * read request will be processed by this function.
 */
int IssueRead(request_list *current_request, int p){
	int hometile, responseL1, responseL2, h, delay=0;
	int num_tiles = (int)pow(2, p);
	hometile = calculateHome(current_request -> request -> address, p);
	if (debug) {
        printf("Request: cycle %d, address %x, core %d ...\n", current_request->request->cycle, current_request->request->address, current_request->core_id);
        printf("Home tile is %d\n", hometile);
    }
	responseL1 = cache_access(arr_L1_cache[current_request->core_id], current_request->request->address, 0, current_request->core_id);
	stat_counter->accessL1[current_request -> core_id] += 1;
    if(responseL1 == 0){// L1 read hit
        if (debug) {
            printf("Memory access request(cycle %d) for read address %x on core %d is satisfied in %d cycles.\n", current_request->request->cycle, current_request->request->address, current_request->core_id, current_request->request->delay);
        }
    }else{// L1 read miss
        if (debug) {
            printf("L1 read miss\n");
        }
        //stat_counter->missL1 += 1;
        stat_counter->missL1[current_request -> core_id] += 1;
        // step 1: Request to home node
        stat_counter->controlMesCounter += 1;
        delay = mesh_message_delay(current_request -> core_id, hometile);
        BlockStatus *bs = (BlockStatus *) malloc (sizeof(BlockStatus));
        delay += parameters[7]; // L2 hit time
        responseL2 = cacheL2_access(arr_L2_cache[hometile], current_request->request->address, bs, num_tiles);
        stat_counter->accessL2[hometile] += 1;
        if(responseL2 == 1){ // L2 miss
            if (debug) {
                printf("L2 read miss\n");
            }
            delay += mesh_message_delay(0, hometile) * 2 + parameters[8]; // miss penalty for memory access
            stat_counter->controlMesCounter += 1;
            stat_counter->dataMesCounter += 1;
            stat_counter->missL2[hometile] += 1;
        }
        if((bs->status == invalid) || (bs->status == shared)){ // block is S or I
            if (debug) {
                printf("Block was invalid or shared\n");
            }
            //Step 2: H sends the data to L
            delay += mesh_message_delay(current_request -> core_id, hometile);
            stat_counter->dataMesCounter += 1;
            // change directory status
            enum Status st = shared;
            change_cacheL2_dir(arr_L2_cache[hometile], current_request->request->address, st, current_request->core_id, num_tiles);
        }else{ // Block is M
            if (debug) {
                printf("Block was modified\n");
            }
            assert(bs->status == modified);
            // Step 2: H returns owner
            delay += mesh_message_delay(current_request->core_id, hometile);
            stat_counter->controlMesCounter += 1;
            int R_tile;
            int i;
            for (i = 0; i < num_tiles; i++){
                if (bs->bitMap[i] == 1){
                    R_tile = i;
                    break;
                }
            }
            // Step 3: L requests from R
            delay += mesh_message_delay(current_request->core_id, R_tile);
            stat_counter->controlMesCounter += 1;
            // Step 4: R returns data to L, R revised entry to H
            if (mesh_message_delay(current_request->core_id, R_tile) > mesh_message_delay(hometile, R_tile))
                delay += mesh_message_delay(current_request->core_id, R_tile);
            else
                delay += mesh_message_delay(hometile, R_tile);
            stat_counter->dataMesCounter += 2;
            // change R's block status to shared
            enum Status st = shared;
            change_cacheL1_status(arr_L1_cache[R_tile], current_request->request->address, st);
            // change H's directory to shared
            change_cacheL2_dir(arr_L2_cache[hometile], current_request->request->address, st, current_request->core_id, num_tiles);
        }

        stat_counter->missPenalty[current_request->core_id] += delay;
        current_request -> request -> delay = delay;
        if (debug) {
            printf("Memory access request(cycle %d) for read address %x on core %d is satisfied in %d cycles.\n", current_request->request->cycle, current_request->request->address, current_request->core_id, current_request->request->delay);
        }
    }
	return delay; 
}


/**
 * write request will be processed by this function.
 */
int IssueWrite(request_list *current_request, int p){
    int hometile, responseL1, responseL2, h, delay = 0;
	int num_tiles = (int)pow(2, p);
	hometile = calculateHome(current_request -> request -> address, p);
    if (debug) {
        printf("Request: cycle %d, address %x, core %d ...\n", current_request->request->cycle, current_request->request->address, current_request->core_id);
        printf("Home tile is %d\n", hometile);
    }
	responseL1 = cache_access(arr_L1_cache[current_request->core_id], current_request->request->address, 1, current_request->core_id);
	stat_counter->accessL1[current_request -> core_id] += 1;
    if(responseL1 == 3){// L1 write hit and the block was modified
        if (debug) {
            printf("Block was modified, write directly\n");
            printf("Memory access request(cycle %d) for write address %x on core %d is satisfied in %d cycles.\n", current_request->request->cycle, current_request->request->address, current_request->core_id, current_request->request->delay);
        }
    }else{
        if (responseL1 == 0){ // L1 write hit and the block was shared
            if (debug) {
                printf("Block was shared.\n");
            }
            // step 1: Request to home node
            stat_counter->controlMesCounter += 1;
            delay = mesh_message_delay(current_request->core_id, hometile);
            BlockStatus *bs = (BlockStatus *) malloc (sizeof(BlockStatus));
            delay += parameters[7]; // L2 hit time
            responseL2 = cacheL2_access(arr_L2_cache[hometile], current_request->request->address, bs, num_tiles);
            // step 2: H returns L block sharers
            delay += mesh_message_delay(current_request->core_id, hometile);
            stat_counter->controlMesCounter += 1;
            // Step 3: L sends invalid to all R
            int R_tile;
            int i;
            int max_delay = 0;
            int R_delay;
            enum Status st = invalid;
            for (i = 0; i < num_tiles; i++){
                if (bs->bitMap[i] == 1){
                    R_tile = i;
                    R_delay = mesh_message_delay(current_request->core_id, R_tile);
                    if (R_delay > max_delay) max_delay = R_delay;
                    if (R_tile != current_request->core_id) // if it is not L
                        change_cacheL1_status(arr_L1_cache[R_tile], current_request->request->address, st);
                }
            }
            delay += max_delay;
            stat_counter->controlMesCounter += 1;
            // Step 4: R ack to L
            delay += max_delay;
            stat_counter->controlMesCounter += 1;
            // Step 5: L revises entry to H
            delay += mesh_message_delay(current_request->core_id, hometile);
            stat_counter->controlMesCounter += 1;
            st = modified;
            change_cacheL2_dir(arr_L2_cache[hometile], current_request->request->address, st, current_request->core_id, num_tiles);
        }else{ // write miss to L1
            if (debug){
                printf("L1 write miss\n");
            }
            stat_counter->missL1[current_request -> core_id] += 1;
            // step 1: Request to home node
            stat_counter->controlMesCounter += 1;
            delay = mesh_message_delay(current_request -> core_id, hometile);
            BlockStatus *bs = (BlockStatus *) malloc (sizeof(BlockStatus));
            delay += parameters[7]; // L2 hit time
            responseL2 = cacheL2_access(arr_L2_cache[hometile], current_request->request->address, bs, num_tiles);
            stat_counter->accessL2[hometile] += 1;
            if(responseL2 == 1){ // L2 miss
                if (debug) printf("L2 access miss\n");
                delay += mesh_message_delay(0, hometile) * 2 + parameters[8]; // miss penalty for memory access
                stat_counter->controlMesCounter += 1;
                stat_counter->dataMesCounter += 1;
                stat_counter->missL2[hometile] += 1;
            }
            if(bs->status == invalid){ // block is I
                if (debug) printf("Block was invalid\n");
                //Step 2: H sends the data to L
                delay += mesh_message_delay(current_request -> core_id, hometile);
                stat_counter->dataMesCounter += 1;
                // change directory status
                enum Status st = modified;
                change_cacheL2_dir(arr_L2_cache[hometile], current_request->request->address, st, current_request->core_id, num_tiles);
            }
            if (bs->status == modified){ // Block is M
                if (debug) printf("Block was modified\n");
                // Step 2: H returns owner
                delay += mesh_message_delay(current_request->core_id, hometile);
                stat_counter->controlMesCounter += 1;
                int R_tile;
                int i;
                for (i = 0; i < num_tiles; i++){
                    if (bs->bitMap[i] == 1){
                        R_tile = i;
                        break;
                    }
                }
                // Step 3: L requests from R
                delay += mesh_message_delay(current_request->core_id, R_tile);
                stat_counter->controlMesCounter += 1;
                // Step 4: R returns data to L, R revised entry to H
                if (mesh_message_delay(current_request->core_id, R_tile) > mesh_message_delay(hometile, R_tile))
                    delay += mesh_message_delay(current_request->core_id, R_tile);
                else
                    delay += mesh_message_delay(hometile, R_tile);
                stat_counter->dataMesCounter += 2;
                // change R's block status to invalid
                enum Status st = invalid;
                change_cacheL1_status(arr_L1_cache[R_tile], current_request->request->address, st);
                // change H's directory to modified
                st = modified;
                change_cacheL2_dir(arr_L2_cache[hometile], current_request->request->address, st, current_request->core_id, num_tiles);
            }
            if (bs->status == shared){ // Block is S
                if (debug) printf("Block was shared.\n");
                // step 2: H returns L block sharers and data
                delay += mesh_message_delay(current_request->core_id, hometile);
                stat_counter->dataMesCounter += 1;
                // Step 3: L sends invalid to all R
                int R_tile;
                int i;
                int max_delay = 0;
                int R_delay;
                enum Status st = invalid;
                for (i = 0; i < num_tiles; i++){
                    if (bs->bitMap[i] == 1){
                        R_tile = i;
                        R_delay = mesh_message_delay(current_request->core_id, R_tile);
                        if (R_delay > max_delay) max_delay = R_delay;
                        change_cacheL1_status(arr_L1_cache[R_tile], current_request->request->address, st);
                    }
                }
                delay += max_delay;
                stat_counter->controlMesCounter += 1;
                // Step 4: R ack to L
                delay += max_delay;
                stat_counter->controlMesCounter += 1;
                // Step 5: L revises entry to H
                delay += mesh_message_delay(current_request->core_id, hometile);
                stat_counter->controlMesCounter += 1;
                st = modified;
                change_cacheL2_dir(arr_L2_cache[hometile], current_request->request->address, st, current_request->core_id, num_tiles);
            }
        }
    stat_counter->missPenalty[current_request->core_id] += delay;
    current_request -> request -> delay = delay;
    if (debug) printf("Memory access request(cycle %d) for write address %x on core %d is satisfied in %d cycles.\n", current_request->request->cycle, current_request->request->address, current_request->core_id, current_request->request->delay);
    }
	return delay;
}

/**
 * Calculates the index of the home tile by extracting bits 12 to 12+p-1 of the 32-bit memory address.
 * @return int index the home tile index
 */
int calculateHome(unsigned long address, int p) {
    unsigned long mask = 0;
    unsigned long index = address >> 12;
    int i;
    for (i = 0; i < p; i++) {
        mask = mask << 1;
        mask = mask | 0x1;
    }
    return (int)index & mask;
}

/**
 * prints all parsed requests from input file
 */
void printallrequests(){
	int i = 0;
	request_list *current_request= (request_list *) malloc (sizeof(request_list));
	printf("core_id  cycle  type  mem_addr\n");
	for (i = 0;i < size_list; i++) {
        current_request = requestList[i];
		printf("%d\t %d\t %d\t %x\n", current_request -> core_id, current_request-> request -> cycle, current_request -> request -> type, current_request -> request -> address );
    }
}


void loadProcessor(int p){
	int i, j, k = 0, l;
	int prevCycleNo = 1;
	
	processors = (processor **) malloc (p * sizeof(processor *));
	for(i = 0; i<p; i++)
	{
		processors[i] = (processor *) malloc (sizeof(processor));
	}
	
	request_list *current_request= (request_list *) malloc (sizeof(request_list));
	//printf(" p = %d, size_list - %d\n", p, size_list);
	for(i = 0; i< p; i++)
	{
		processors[i] -> core_id = i;
		processors[i] -> instructionsCount = 0;		
		k = 0;
		processors[i] -> requestList = ( request_list **)calloc(MAX_SIZE, sizeof(request_list *));

		for(l = 0; l < MAX_SIZE; l++) {
			processors[i] -> requestList[l] = (request_list *)calloc(1,sizeof(request_list));
		}
	
		for (j = 0;j < size_list; j++) {
			current_request = requestList[j];
			if(i == current_request -> core_id){
				if(k < MAX_SIZE){
					current_request -> request -> nextcycle = current_request -> request -> cycle - prevCycleNo; 
					current_request -> request -> possible_nextcycle_no = 0;
					processors[i] -> requestList[k] = current_request;
					prevCycleNo = current_request -> request -> cycle;
					k++;
				}
				else{
					processors[i] -> requestList = (request_list **) realloc (processors[i] -> requestList, sizeof(request_list*)* (k+1)); 
					processors[i] -> requestList[k] = (request_list *) malloc (sizeof(request_list));
					current_request -> request -> nextcycle = current_request -> request -> cycle - prevCycleNo; 
					current_request -> request -> possible_nextcycle_no = 0;
					processors[i] -> requestList[k] = current_request;
					prevCycleNo = current_request -> request -> cycle;
					k++;
				}
			}
			
		}
		prevCycleNo = 1;
		processors[i] -> instructionsCount = k;		
		printf("core - %d instructions - %d\n", i, k);
	}
	if (debug) {
        printallrequestsperprocessors(p);
    }
	free(requestList);
}


//
void printallrequestsperprocessors(int p){
	int i = 0, j = 0;
	request_list *current_request= (request_list *) malloc (sizeof(request_list));
	
	for (j = 0; j < p; j++){
		printf("core_id  cycle nextcycle possible_next  type  mem_addr\n");
		for (i = 0;i < processors[j]->instructionsCount; i++) {
			current_request = processors[j]->requestList[i];
			printf("%d\t %d\t %d\t %d\t %d\t %x\n", current_request -> core_id, current_request-> request -> cycle, current_request-> request -> nextcycle, current_request-> request -> possible_nextcycle_no, current_request -> request -> type, current_request -> request -> address );
		}
	}
}

// Compute the delay to send message between two tiles
int mesh_message_delay(int tile1, int tile2){
    int p = parameters[0];
    int i;
    if(p%2 == 0){
        i = p/2;
    }else{
        i = (p-1)/2;
    }
    int num_per_line = (int)pow(2,i);
    int x1 = (int)(tile1/num_per_line);
    int y1 = tile1%num_per_line;
    int x2 = (int)(tile2/num_per_line);
    int y2 = tile2%num_per_line;
    int d = abs(x1 - x2) + abs(y1 - y2);
    return d * parameters[6];
}


void printStatistics(int p) {
    int i;
    int L2misses = 0;
    int L2accesses = 0;
    printf("Core \t | \t Cycles Taken \t\t | \t L1 Miss Rate \t\t | \t L2 Miss Rate (Tile) \t\t | \t L1 Miss Penalty\n");
    for (i = 0; i < pow(2,p); i++) {
        double L1miss, L2miss, L1penalty;
        //printf("L1 access: %d; L1 miss: %d, L2 access: %d, L2 miss: %d\n", stat_counter -> accessL1[i], stat_counter -> missL1[i], stat_counter -> accessL2[i], stat_counter -> missL2[i]);
        if (stat_counter -> accessL1[i] != 0 ) {
            L1miss = stat_counter -> missL1[i] / (float)stat_counter -> accessL1[i] * 100;
        } else {
            L1miss = 0;
        }
        if (stat_counter -> accessL2[i] != 0 ) {
          L2miss = stat_counter -> missL2[i] / (float) stat_counter -> accessL2[i] * 100;
        } else {
            L2miss = 0;
        }
        if (stat_counter -> missL1[i] != 0) {
            L1penalty = stat_counter -> missPenalty[i] / (float)stat_counter -> missL1[i];
        } else {
            L1penalty = 0;
        }
        L2misses += stat_counter -> missL2[i];
        L2accesses += stat_counter -> accessL2[i];
        printf("%d \t | \t %7d \t\t | \t %5.1f% \t\t | \t %5.1f% \t\t\t | \t %4.1f\n",
         i, stat_counter -> cyclesPerCore[i], L1miss, L2miss, L1penalty);
    }
    double L2missTotal;
    if (L2accesses != 0) {
        L2missTotal = L2misses / (float)L2accesses * 100;
    } else {
        L2missTotal = 0;
    }
    printf("===========================\nL2 Miss Rate (Total) \t | \t Control Messages \t\t | \t Data Messages\n");
    printf("%5.1f% \t\t\t | \t %7d \t\t\t | \t %d\n", L2missTotal,stat_counter -> controlMesCounter, stat_counter -> dataMesCounter);
}

void printAllBlockStatus() {
    int i, j, k, core;
    int block_address;
	int index;
	int tag;
	int num_tiles = (int)pow(2, parameters[0]);
    for (i = 0; i < num_tiles; i++) { //Go through each L1 cache
        unsigned long address,addressTemp;
        struct cache_t * cp = arr_L1_cache[i];
//        for (addressTemp = 0; addressTemp < pow(2,parameters[1]); addressTemp += pow(2,parameters[3])) {
//            address = i * pow(2,parameters[1]) + addressTemp;
//            printf("0x%x\n",address);
//            printf("Addr: %d\n",address);
//            block_address = (address / cp->blocksize);
//	        tag = block_address / cp->nsets;
//	        index = block_address - (tag * cp->nsets);
//            for (j = 0; j < cp->assoc; j++) {
//                printf("BA:%d tag:%d Index:%d way:%d\n",block_address,tag,index,j);
//                printf("Tag %d IsValid:%d\n",cp->blocks[index][j].tag,cp->blocks[index][j].valid);
//            }
//    }
        printf("L1 on Tile %d\n",i);
        for (j = 0; j < cp->nsets; j++) {
            for (k = 0; k < cp->assoc; k++) {	/* look for the requested block */
                if (cp->blocks[j][k].valid) {
                    block_address = j + (cp->blocks[j][k].tag * cp->nsets);
                    printf("Block Address: 0x%x\n",block_address);
                    printf("Tag %d IsDirty: %d\n",cp->blocks[j][k].tag,cp->blocks[j][k].dirty);
                }
            }
        }
        printf("\n");
    }
    for (i = 0; i < num_tiles; i++) {
        printf("L2 on Tile %d\n",i);
        struct cache_L2 * cp = arr_L2_cache[i];
        for (j = 0; j < cp->nsets; j++) {
            for (k = 0; k < cp->assoc; k++) {
                if (cp->blocks[j][k].tag,cp->blocks[j][k].valid) {
                    block_address = j + (cp->blocks[j][k].tag * cp->nsets);
                    printf("Block Address: 0x%x\n",block_address);
                    printf("Tag %d Valid: %d\n",cp->blocks[j][k].tag,cp->blocks[j][k].valid);
                    switch(cp->blocks[j][k].blockStatus.status) {
                        case 0:
                            printf("Status: Invalid\n");
                            break;
                        case 1:
                            printf("Status: Shared with cores ");
                            for (core = 0; core < num_tiles; core++){
                                if (cp->blocks[j][k].blockStatus.bitMap[core] == 1) printf("%d ", core);
                            }
                            printf("\n");
                            break;
                        case 2:
                            printf("Status: Modified in core ");
                            for (core = 0; core < num_tiles; core++){
                                if (cp->blocks[j][k].blockStatus.bitMap[core] == 1) printf("%d ", core);
                            }
                            printf("\n");
                            break;
                    }
                }
            }
        }
        printf("\n");
    }
}


int cacheL2_access(struct cache_L2 *cp, unsigned long address, BlockStatus *bs, int num_tiles)
//returns 0 (if a hit), and assign BlockStatus *bs to the real block status
//return 1 (if miss), and fetch data from memory, and assign BlockStatus *bs to the initialized block status
{
	int i ;
	int block_address ;
	int index ;
	int tag ;
	int way ;
	int max ;

	block_address = (address / cp->blocksize);
	tag = block_address / cp->nsets;
	index = block_address - (tag * cp->nsets);

	for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
	  if (cp->blocks[index][i].tag == tag && cp->blocks[index][i].valid == 1)
	  {
	  	if (debug) printf("Cache hit! address: %x, index: %d, tag: %d, way: %d\n", address, index, tag, i);
		updateLRU_L2(cp, index, i);
		*bs = cp->blocks[index][i].blockStatus;
		return(0);						/* a cache hit */
	  }
	}
	/* a cache miss */
	 BlockStatus init_blockStatus;
	 init_blockStatus.status = invalid;
	 init_blockStatus.busy = 0;
	 init_blockStatus.bitMap = (int *)calloc(num_tiles, sizeof(int)) ;
     for(i = 0; i < num_tiles; i++) {
		init_blockStatus.bitMap[i] = 0;
	 }

	for (way=0 ; way< cp->assoc ; way++)		/* look for an invalid entry */
		if (cp->blocks[index][way].valid == 0)	/* found an invalid entry */
		{
	      if (debug) printf("Cache miss and fetch on an invalid entry! address: %x, index: %d, tag: %d, way: %d\n", address, index, tag, way);
		  cp->blocks[index][way].valid = 1 ;
		  cp->blocks[index][way].tag = tag ;
          cp->blocks[index][way].blockStatus = init_blockStatus;
		  updateLRU_L2(cp, index, way);
		  *bs = cp->blocks[index][way].blockStatus;
		  return(1);
		}

	 max = cp->blocks[index][0].LRU ;			/* find the LRU block */
	 way = 0 ;
	 for (i=1 ; i< cp->assoc ; i++)
	  if (cp->blocks[index][i].LRU > max) {
		max = cp->blocks[index][i].LRU ;
		way = i ;
	  }
	 if (debug) printf("Cache miss and replace an old entry! address: %x, index: %d, tag: %d, way: %d\n", address, index, tag, way);
	// cp->blocks[index][way] is the LRU block
	// First evict the correspondent L1 cache if it is not uncached
	if ((cp->blocks[index][way].blockStatus.status == shared)||(cp->blocks[index][way].blockStatus.status == modified)){
	    int R_tile;
        enum Status st = invalid;
        for (i = 0; i < num_tiles; i++){
            if (cp->blocks[index][way].blockStatus.bitMap[i] == 1){
                R_tile = i;
                unsigned long addr = (index + (cp->blocks[index][way].tag * cp->nsets)) * cp->blocksize;
                change_cacheL1_status(arr_L1_cache[R_tile], addr, st);
            }
        }
	}
	// Then replace the block
	cp->blocks[index][way].tag = tag;
	cp->blocks[index][way].blockStatus = init_blockStatus;
	updateLRU_L2(cp, index, way);
	*bs = cp->blocks[index][way].blockStatus;
	return(1);
}

int cache_access(struct cache_t *cp, unsigned long address, int access_type /*0 for read, 1 for write*/, int L_tile)
//returns 0 (if a read hit or a write hit but it is not dirty (shared)), 1 (if a miss but no dirty block is writen back) or
//2 (if a miss and a dirty block is writen back)
// 3 (if a write hit and it is dirty (modified))
{
	int i ;
	int block_address ;
	int index ;
	int tag ;
	int way ;
	int max ;

	block_address = (address / cp->blocksize);
	tag = block_address / cp->nsets;
	index = block_address - (tag * cp->nsets);

	for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
	  if (cp->blocks[index][i].tag == tag && cp->blocks[index][i].valid == 1)
	  {
	    if (debug) printf("Cache hit! address: %x, index: %d, tag: %d, way: %d\n", address, index, tag, i);
		updateLRU(cp, index, i) ;
		if (access_type == 1) {
		    if (cp->blocks[index][i].dirty == 0) {
		        cp->blocks[index][i].dirty = 1 ;
		        return(0); /* a cache write hit and it was not dirty */
		    }else{
		        return (3); /* a cache write hit and it was already dirty */
		    }
		}else{
		    return(0); /* a cache read hit */
		}
	  }
	}
	/* a cache miss */
	for (way=0 ; way< cp->assoc ; way++)		/* look for an invalid entry */
		if (cp->blocks[index][way].valid == 0)	/* found an invalid entry */
		{
		  if (debug) printf("Cache miss and fetch on an invalid entry! address: %x, index: %d, tag: %d, way: %d\n", address, index, tag, way);
		  cp->blocks[index][way].valid = 1 ;
		  cp->blocks[index][way].tag = tag ;
		  updateLRU(cp, index, way);
		  cp->blocks[index][way].dirty = 0;
		  if(access_type == 1) cp->blocks[index][way].dirty = 1 ;
		  return(1);
		}

	 max = cp->blocks[index][0].LRU ;			/* find the LRU block */
	 way = 0 ;
	 for (i=1 ; i< cp->assoc ; i++){
	  if (cp->blocks[index][i].LRU > max) {
		max = cp->blocks[index][i].LRU ;
		way = i ;
	  }
	  }
	if (debug) printf("Cache miss and replace an old entry! address: %x, index: %d, tag: %d, way: %d\n", address, index, tag, way);
	// cp->blocks[index][way] is the LRU block
	// Firstly, send the home of the evicted L1 cache that it turns to invalid
	unsigned long addr = (index + (cp->blocks[index][way].tag * cp->nsets)) * cp->blocksize;
	int hometile = calculateHome(addr, parameters[0]);
	int num_tiles = (int)pow(2, parameters[0]);
	evict_cacheL2_dir(arr_L2_cache[hometile], addr, L_tile, num_tiles);
	// Then replace the block
	cp->blocks[index][way].tag = tag;
	updateLRU(cp, index, way);
	if (cp->blocks[index][way].dirty == 0)
	  {											/* the evicted block is clean*/
		cp->blocks[index][way].dirty = access_type;
		return(1);
	  }
	else
	  {											/* the evicted block is dirty*/
		cp->blocks[index][way].dirty = access_type;
		return(2);
	  }

}