//////////////////////////////////////////////////////////////////////////////////
//
//   Program Name:  Buddy System Algorithm
//                  
//   Description:  Buddy System Algorithm
//                  
//   Student name:
//
//
//////////////////////////////////////////////////////////////////////////////////


#ifndef __BUDDYSYS_H__
#define __BUDDYSYS_H__

#include "auxiliary.h"

typedef unsigned char byte;  // shorthand for byte type

struct llist {
    long long int size;      // size of block (data only, excludes Node header)
    int alloc;               // 0 = free, 1 = allocated
    struct llist* next;      // pointer to next block
    struct llist* previous;  // pointer to previous block
};

typedef struct llist Node; // Node structure for linked list

extern Node* wholememory; // Pointer to the start of the whole memory block
extern long long int MEMORYSIZE; // Total size of the memory block
extern long long int WHOLEMEMORY_BLOCKSIZE; // Size of the whole memory block excluding the header

#define MIN_ORDER 5 // Minimum order size 
#define MAX_ORDER 25 // Maximum order size 
#define NUMBEROFPAGES 8192 // Number of pages in the memory block
#define HEADER_SIZE sizeof(Node) // Size of the header for each block

extern int failure_counts[MAX_ORDER + 1]; // Array to count allocation failures for each order size

void* buddyMalloc(int request_memory); // Allocate memory using Buddy System
int buddyFree(void* p); // Free allocated memory
void printFreeList(); // Print the free list of memory blocks
int getOrderSize(long long int getSize); // Get the order size for a given size

#endif 
