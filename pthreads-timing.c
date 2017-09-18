//written by: Jeremy Keys last modified 2-22-17

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/****************** structure for pthread_create argument to new thread ******************/

typedef struct BlockData {
	int* array;
	unsigned int blockSize;
	unsigned int minIdx;
	unsigned int maxIdx;
	unsigned int numBlocks;
	unsigned int step;
	unsigned int currBlock;
} BlockData;

/****************** global variables ******************/

#define BILLION 1000000000L //code taken from https://www.cs.rutgers.edu/~pxk/416/notes/c-tutorials/gettime.html

const int MIN_VAL = -1000, MAX_VAL = 1000; //used when mode == 1, to get a random value

//pointer to an array of individual block sums, which will be init after program arguments parsed;
long *blockSums; //each long elem malloc'd to array will be available to exactly one thread, preventing data races (correct??)

//this will contain the thread ids for numBlocks threads
pthread_t *threads;

BlockData *dataBlocks; 


/****************** initialization, reserve (malloc) and cleanup (free), and utility funcs ******************/

int* initArray(unsigned int size, unsigned int mode) {
	int *arr = malloc(size * sizeof(int) );
	int i; 
	
	if(mode == 1) {
		for (i = 0; i != size; i++) {
			arr[i] = (rand() % (MAX_VAL*2))+ MIN_VAL; //find a number between 0 and 2000, then subtract 1000 to get proper range
		}	
	} else { 
		for (i = 0; i != size; i++) {
			arr[i] = i;
		}
	}
	
	return arr;
}

void initThreadData(unsigned int numBlocks) {	
	blockSums = malloc(sizeof(long) * numBlocks);	
	threads = malloc(sizeof(pthread_t) * numBlocks);
	dataBlocks = malloc(sizeof(BlockData) * numBlocks);
}

void freeArray(int* arr) {
	free(arr);
	
	//also free the global arrays
	free(blockSums);
	free(threads);
	free(dataBlocks);	
}

long sumBlocks(long *blockSums, int numBlocks) {
	int i;
	long sum = 0;
	for(i = 0; i != numBlocks; i++) {
		sum += blockSums[i];
	}
	
	return sum;
}

/****************** pthread compatible block summation function ******************/

void *sumBlock(void *blockData) {	
	struct BlockData *bd = (struct BlockData*) blockData;
	// void *bd = blockData;
	
	int i,j;
	long sum = 0;
	
	for(i = 0; i != bd->step; i++) {
		for(j = bd->minIdx + i; j <= bd->maxIdx; j += bd->step) {
			sum += bd->array[j];
		}
	}
	
	blockSums[bd->currBlock] = sum;
}

/****************** sumArray, contains multi-threading code ******************/

double sumArray(int* array, unsigned int size, long* sum, unsigned int numBlocks, unsigned int step) {
	unsigned int minIdx, maxIdx, blockSize = size / numBlocks; 	
	struct timespec start, end;	
	int i;
		
	clock_gettime(CLOCK_REALTIME, &start);
	
	/*** create one thread per block, and spin up each thread to the sumBlock method ***/	
	
	for(i = 0; i != numBlocks; i++) {
		minIdx = blockSize*i;
		maxIdx = minIdx + (blockSize - 1);
		
		/*** initialize the data struct argument for the current (i'th) block ***/	
		dataBlocks[i].array = array;	//some of these could become global, but this works with little additional overhead
		dataBlocks[i].blockSize = blockSize;		
		dataBlocks[i].minIdx = minIdx;		
		dataBlocks[i].maxIdx = maxIdx;		
		dataBlocks[i].numBlocks = numBlocks; 
		dataBlocks[i].step = step;
		dataBlocks[i].currBlock = i; //need to store index so we can access the correct bytes and avoid data race
				
		/*** create the i'th (currBlock'th) thread, spin it up ***/	
		pthread_create(&threads[i], NULL, sumBlock, (void*) &(dataBlocks[i]));
	}
	
	/*** join all threads back to master thread ***/	
	for(i = 0; i != numBlocks; i++) {
		pthread_join(threads[i],NULL);
	}
	
	/*** sum each individual block sum, and store it in the sum argument ***/	
	*sum = sumBlocks(blockSums, numBlocks);
	
	clock_gettime(CLOCK_REALTIME, &end);	
	
	// return (double) ((double) (end.tv_sec - start.tv_sec) + (double) (end.tv_nsec - start.tv_nsec)) / (double) BILLION ;  
	return (double) (end.tv_sec - start.tv_sec) + (double) ((end.tv_nsec - start.tv_nsec) / (double) BILLION) ;  
}


int main(int argc, char** argv) {
	int i, arraySize= 10000, mode = 0, numBlocks = 1, stepSize = 1, seed;
	long sum;
	
	/*** process program arguments ***/	
	
	if(argc % 2 == 0 && argc != 1) {
		printf("Must provide an even amount of arguments\n"); //will actually be odd, b/c argv[0] is name of program
		return 1;
	}
		
	for(i = 1; i < argc; i += 2) { 			//start at 1, b/c argv[0] is name of prog
		char* flag = argv[i];				
		char *flagValue_s = argv[i+1];		
		int flagValue = atoi(flagValue_s);
		
		if(strcmp(flag, "-s") == 0) {
			arraySize = flagValue;			
		} else if(strcmp(flag, "-b") == 0) {
			if(flagValue < 0) {
				printf("Number of blocks must be positive\n");
				return 2;
			}			
			numBlocks = flagValue;			
		} else if(strcmp(flag, "-m") == 0) {
			if (flagValue != 0 && flagValue != 1) {
				printf("Valid mode args are 0 or 1\n");
				return 3;
			}			
			mode = flagValue;			
		} else if(strcmp(flag, "-p") == 0) {
			if(flagValue < 0) {				
				printf("Step size must be positive\n");
				return 4;
			}
			stepSize = flagValue;			
		} else if(strcmp(flag, "-r") == 0) {
			srand(flagValue); //we only seed if we are given a seed value
		} else {
			printf("Invalid flag given\n");
			return 5;
		} //end ifs
	} //end arg processing for
	
	/*** finished processing program arguments, move on to summation and print ***/	
	
	initThreadData(numBlocks); //allocate space for the blockSums, data blocks and the p_threads 
	
	int *array = initArray(arraySize, mode);	
		
	printf("Amount of time to execute sumArray: %fs\n", sumArray(array, arraySize, &sum, numBlocks, stepSize));
	printf("Sum of execution: %ld\n", sum);
	
	freeArray(array); //let's not leak memory; put in fcn wrapper in case other housekeeping is later added
	
	return 0;
}