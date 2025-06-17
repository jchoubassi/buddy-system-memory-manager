//////////////////////////////////////////////////////////////////////////////////
//
//   Program Name:  Buddy System Algorithm
//                  
//   Description:  Buddy System Algorithm
//                  
//   Student name:
//
//
//
//////////////////////////////////////////////////////////////////////////////////

#include "buddysys.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iostream>

Node *free_lists[MAX_ORDER + 1] = {nullptr};
bool initialized = false;
int failure_counts[MAX_ORDER + 1] = { 0 };

void printFreeList(); //debugging function can be removed later

Node* wholememory = nullptr;
long long int MEMORYSIZE = 0;
long long int WHOLEMEMORY_BLOCKSIZE;

/////////////////////////////////////////////////////////////////////////////////

//get order for size
int getOrderSize(long long int getSize) {
	long long int totalSize = getSize + sizeof(Node); // include size of Node header
	int order = MIN_ORDER; // start from minimum order
	while ((1LL << order) < totalSize && order <= MAX_ORDER) { // 1LL << order is equivalent to 2^order
		order++; // increment order until we find a size that fits
    }
	return order; // return the order that fits the requested size
}


void splitBlock(int order) {
	if (order <= MIN_ORDER || order > MAX_ORDER || free_lists[order] == nullptr) { //check if order is valid and if there are blocks to split
		return; //no blocks to split or invalid order
    }

	Node* bigBlock = free_lists[order]; // get the first block of the current order
	free_lists[order] = bigBlock->next; // move the free list pointer to the next block
	if (bigBlock->next) bigBlock->next->previous = nullptr; // update the previous pointer of the next block if it exists

	size_t fullSize = (1ULL << order); // calculate the full size of the block based on the order
	size_t halfSize = fullSize / 2; // calculate the half size just for splitting

	Node* buddy1 = bigBlock; // buddy1 is the first half of the split block
	Node* buddy2 = (Node*)((unsigned char*)buddy1 + halfSize); // buddy2 is the second half, calculated by adding halfSize to the address of buddy1, we use this to ensure the buddy2 is aligned correctly

	buddy1->size = buddy2->size = halfSize - sizeof(Node); // set the size of both buddies
	buddy1->alloc = buddy2->alloc = 0; // mark both buddies as free 

	buddy1->next = buddy2; // link buddy1 to buddy2
	buddy1->previous = nullptr; // buddy1 is the first in the list, so it has no previous node 

	buddy2->next = free_lists[order - 1]; // buddy2's next points to the current free list of the previous order
	buddy2->previous = buddy1; // buddy2's previous points to buddy1, linking them together 

	if (buddy2->next) buddy2->next->previous = buddy2; // if there is a next node in the free list, update its previous pointer to point to buddy2
	free_lists[order - 1] = buddy1; // update the free list for the previous order to point to buddy1, which is now the first node in that list

    //printf("[DEBUG] buddies at %p and %p with size %lld at order %d\n", buddy1, buddy2, buddy1->size, order - 1);

}

void initBuddyAlloc() {

#ifndef RUN_SIMPLE_TEST
    MEMORYSIZE = (long long int)NUMBEROFPAGES * PAGESIZE;
#else
    MEMORYSIZE = (1ULL << MAX_ORDER); // For simple test, use max block size memory
#endif

	wholememory = (Node*)allocpages(MEMORYSIZE / PAGESIZE); // allocate memory for the whole memory block
	if (!wholememory) { // if memory allocation fails then we exit the program
        //printf("Memory allocation failed\n");
        exit(1);
    }

    if ((uintptr_t)wholememory % (1ULL << MIN_ORDER) != 0) {
        //printf("UNALIGNED\n");
        exit(1);
    }

	memset(wholememory, 0, MEMORYSIZE); //clear the allocated memory block

    //set max block size consistent with MEMORYSIZE and MAX_ORDER
    WHOLEMEMORY_BLOCKSIZE = MEMORYSIZE - sizeof(Node);

	wholememory->size = WHOLEMEMORY_BLOCKSIZE; // set the size of the whole memory block
	wholememory->alloc = 0; // here we mark the whole memory block as free
	wholememory->next = nullptr; //we set the next pointer to nullptr since this is the only block in the free list at the moment
	wholememory->previous = nullptr; //next pointer is nullptr since this is the only block in the free list at the moment

	// initialize free list for max order to point to the whole memory block
    for (int i = 0; i <= MAX_ORDER; i++) {
		free_lists[i] = nullptr; //initialize all free lists to nullptr to start with
    }
	free_lists[MAX_ORDER] = wholememory; // set the free list for the max order to point to the whole memory block

	initialized = true;//set to true to indicate that the buddy allocator has been started
}

//function to allocate memory using the buddy system
void* buddyMalloc(int request_memory) { //returns pointer

	if (!initialized) initBuddyAlloc(); // initialize the buddy allocator if it hasn't been initialized yet
	if (request_memory <= 0 || request_memory > MEMORYSIZE) { // check if the requested memory is valid
        return nullptr;
    }

	int setOrder = getOrderSize(request_memory); // get the order size for the requested memory
	int currentOrder = setOrder; // start with the desired order

    //1. find smalest avail block >= desiredOrder
    while (currentOrder <= MAX_ORDER && !free_lists[currentOrder]) {
        currentOrder++;
    }

    if (currentOrder > MAX_ORDER) {
        failure_counts[setOrder]++;
        return nullptr;
    }

    //2. split larger block to desired order
    while (currentOrder > setOrder) {
        splitBlock(currentOrder);
        currentOrder--;
    }

    if (!free_lists[setOrder]) {
        failure_counts[setOrder]++;
        return nullptr;
    }

    //3. remove block from free list and allocate it
    Node* block = free_lists[setOrder];
    free_lists[setOrder] = block->next;
    if (block->next) block->next->previous = nullptr;

    block->alloc = 1; //allocated
    block->next = nullptr; //reset next ptr
    block->previous = nullptr; //reset prev ptr

    //4. return ptr to mem after header
    //printFreeList(); //DEBUGGING, CAN REMOVE LATER

    return (void*)((unsigned char*)block + sizeof(Node)); // changed byte to unsigned char, was getting error of it being ambiguous not sure if this will fix the problem 
    // pointer that it returns 
}


int buddyFree(void* p) // we point to the variable we created that was passed through buddyfree which pointed to the start of the access memory (this line return (void*)((unsigned char*) block + sizeof(Node)); ) - using linked list
{
    if (p == NULL) return 0; // error handling, if ptr sent through buddymalloc is null exit

    // since the pointer points to the start of accessed memory,
    // move back by sizeof(Node) to get the actual block metadata
    Node* block = (Node*)((unsigned char*)p - sizeof(Node));

    // update alloc to 0: 1 = allocated, 0 = free
    block->alloc = 0;

    long long int totalSize = block->size + sizeof(Node); // total size includes header
    // currently MIN_ORDER = 5 (32 bytes)
    int order = MIN_ORDER;
    // find the correct order based on block total size
    while ((1LL << order) < totalSize && order <= MAX_ORDER) {
        order++;
    }

    // Insert freed block first
    Node* curr = free_lists[order];
    Node* prev = nullptr;
    while (curr && curr < block) {
        prev = curr;
        curr = curr->next;
    }
    block->next = curr;
    block->previous = prev;
    if (curr) curr->previous = block;
    if (prev) prev->next = block;
    else free_lists[order] = block;
    // try merge while not reaching max block size
    while (order < MAX_ORDER) { 
        uintptr_t block_address = (uintptr_t)block; // uintptr_t, so we can do xor caluclations to the memory address, block is the pointer to the node ->  Node* block = (Node*)((unsigned char*)p - sizeof(Node));
        
        // alignment check for current order
        if (block_address % (1ULL << order) != 0) {
            break; // block must be aligned to its size
        }

        uintptr_t buddy_address = block_address ^ (1ULL << order); // same here, but we do an XOR caluclation where we caluclate the buddy_address (1ULL - 0001 in binary, << bitwise shift left which equals 0001 -> 100000 which the binary of that is 32)
        
        Node* buddy = (Node*)buddy_address; // buddy address converted back into a pointer to the node

		if (buddy == block) break; //if buddy is the same as block, we cannot merge

		if (buddy < (Node*)wholememory || buddy >= (Node*)((unsigned char*)wholememory + MEMORYSIZE)) break; //if buddy is outside the bounds of the whole memory block, we cannot merge

		if (buddy->alloc) break; //if buddy is allocated, we cannot merge

		// check if buddy is in the free list for the current order
        bool buddyFound = false;
        Node* iter = free_lists[order];
        while (iter) {
            if (iter == buddy) {
                buddyFound = true; //if buddy is found in the free list, we can merge
                break;
            }
			iter = iter->next; //now we iterate through the free list to find the buddy
        }

        //buddy checks
        if (!buddyFound) break; // if buddy is not in the free list for the current order, we cannot merge

		if (buddy->size != (1ULL << order) - sizeof(Node)) break; //if buddy size is not equal to the expected size, we cannot merge
		if (block->size != (1ULL << order) - sizeof(Node)) break; //if block size is not equal to the expected size, we cannot merge

		//remove block from free list
        if (block->previous) {
            block->previous->next = block->next;
        }
        else {
            free_lists[order] = block->next;
        }
        if (block->next) block->next->previous = block->previous;

        // unlink buddy from free list
        if (buddy->previous) {
            buddy->previous->next = buddy->next; // updates next pointer
        }
        else {
            free_lists[order] = buddy->next; // otherwise its the first node
        }
        if (buddy->next) {
            buddy->next->previous = buddy->previous; // if we move onto next node, remove previous pointer
        }

		//calculate merged block address
		uintptr_t merged_addr = ((uintptr_t)block) & ~((1ULL << (order + 1)) - 1); //use unsigned long long int to ensure we get the correct address
		block = (Node*)merged_addr; //set block to the merged address bc this is the new block that we will use
        
		//check if merged block is aligned to its size
        if (block->size != (1ULL << order) - sizeof(Node)) break;

        block->size = (1ULL << (order + 1)) - sizeof(Node);  // merge blocks
		block->alloc = 0; // mark merged block as free so we can use it again
		block->next = nullptr; // set next pointer to nullptr 
		block->previous = nullptr; // set previous pointer to nullptr so we can insert it into the free list

        order++; //increase order

        // Insert block into free list (sorted by address to help future merges)
        curr = free_lists[order]; //set curr to the first node in the free list for the current order
        prev = nullptr; //set prev to nullptr to start with
        while (curr && curr < block) {
            prev = curr; //find the correct position to insert the block in the free list
            curr = curr->next; // set curr to the next node in the free list
        }

        block->next = curr; // link the block to the next node in the free list
        block->previous = prev; // link the block to the previous and next nodes in the free list
        if (curr) curr->previous = block; // if curr is not null, link it to the block
        if (prev) prev->next = block; // if prev is not null, link it to the block
        else free_lists[order] = block; // if prev is null, block is the first in the list
    }

    return 1;
}

//------------- Debugging Functions -----------------
//Called in buddyMalloc if need to remove
void printFreeList() {
    printf("\n--- Free List (debug print) ---\n");
    for (int order = MIN_ORDER; order <= MAX_ORDER; ++order) {
        printf("Order %2d (Block size: %6lld): ", order, 1LL << order);
        Node* current = free_lists[order];
        if (!current) {
            printf("[empty]\n");
            continue;
        }
        while (current) {
            printf("[addr: %p | size: %lld] -> ", (void*)current, current->size);
            current = current->next;
        }
        printf("NULL\n");
    }
    printf("------------------------------\n");
}

//------------- Can comment out/remove -----------------

// comments
// ptr = buddyMalloc(size) = user inputs the size, then buddymalloc allocates a memory block large enough to hold it
// it will then return (void*)((unsigned char*)block + sizeof(Node)); void is the pointer that points to the start of the accesses the memory
// The address of a block's "buddy" is equal to the bitwise exclusive OR (XOR) of the block's address and the block's size. 
// block sizes are 2^order, buddy block size differs exactly between 2^order e.g/ block size is 0x1000 2^5, buddy address will be 0x1000 XOR 32