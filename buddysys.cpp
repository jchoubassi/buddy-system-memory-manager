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
void printFreeListToFile(const std::string& filename); //debugging function can be removed later

/////////////////////////////////////////////////////////////////////////////////

//get order for size
int getOrderSize(long long int getSize){
    long long int totalSize = getSize + sizeof(Node);
    int order = MIN_ORDER;

    while ((1LL << order) < totalSize && order <= MAX_ORDER){
        order++;
    }

    return order;
}

void splitBlock(int order){
    if (order <= MIN_ORDER || order > MAX_ORDER || free_lists[order] == nullptr)
        return;
    
    Node* bigBlock = free_lists[order];
    free_lists[order] = bigBlock->next;
    
    if (bigBlock -> next) bigBlock->next->previous = nullptr;
    
    size_t halfSize = (1ULL << (order - 1));

    //buddy 1
    Node* buddy1 = bigBlock;
    //buddy 2 after
    Node* buddy2 = (Node*)((byte*)buddy1 + halfSize);

    buddy1->size = buddy2->size = halfSize - sizeof(Node);
    buddy1->alloc = buddy2->alloc = 0;

    //link buddies
    buddy1->next = buddy2;
    buddy1->previous = nullptr;

    buddy2->next = free_lists[order - 1];
    buddy2->previous = buddy1;

    if(buddy2 -> next)buddy2 -> next -> previous = buddy2;
    free_lists[order - 1] = buddy1;
}

//get buddy address
void initBuddyAlloc(){
    wholememory = (Node *) malloc(MEMORYSIZE);
    if (!wholememory){
        printf("Memory allocation failed\n");
        exit(1);
    }

    memset(wholememory, 0, MEMORYSIZE);

    wholememory -> size = MEMORYSIZE - sizeof(Node);
    wholememory -> alloc = 0;
    wholememory -> next = nullptr;
    wholememory -> previous = nullptr;

    int initOrder = MAX_ORDER;
    free_lists[initOrder] = wholememory;

    initialized = true;
}

void *buddyMalloc(int request_memory){ // returns pointer

    if (!initialized) initBuddyAlloc();
    if (request_memory <= 0 || request_memory > MEMORYSIZE){
        return nullptr;
    }

    int desiredOrder = getOrderSize(request_memory);

    //1. find smalest avail block >= desiredOrder
    int currentOrder = desiredOrder;

    while (currentOrder <= MAX_ORDER && !free_lists[currentOrder]) {
        currentOrder++;
    }

    if (currentOrder > MAX_ORDER) {
        int failedOrder = getOrderSize(request_memory); //DEBUGGING, CAN REMOVE LATER
        if (failedOrder <= MAX_ORDER) { //DEBUGGING, CAN REMOVE LATER
            failure_counts[failedOrder]++; //DEBUGGING, CAN REMOVE LATER
		} //DEBUGGING, CAN REMOVE LATER
        return nullptr; // No available block found
    }


    //2. split larger block to desired order
    while (currentOrder > desiredOrder) {
        long long int blockSize = (1LL << currentOrder);
        long long int neededSize = request_memory + sizeof(Node);
		// don't split anymore if next level is too small -used to reduce fragmentation issues 
        if (blockSize / 2 < neededSize) break;

        splitBlock(currentOrder);
        currentOrder--;
    }

    //3. remove block from free list and allocate it
    Node* block = free_lists[desiredOrder];
    free_lists[desiredOrder] = block -> next;
    if (block->next) {
        block->next->previous = nullptr;
    }

    block -> alloc = 1; //allocated
    block -> next = nullptr; //reset next ptr
    block -> previous = nullptr; //reset prev ptr

    //4. return ptr to mem after header
    //printFreeList(); //DEBUGGING, CAN REMOVE LATER
    //printFreeListToFile("log.txt"); //DEBUGGING, CAN REMOVE LATER
    return (void*)((unsigned char*) block + sizeof(Node)); // changed byte to unsigned char, was getting error of it being ambiguous not sure if this will fix the problem 
                                                          // pointer that it returns 
} 

int buddyFree(void* p) // we point to the variable we created that was passed through buddyfree which pointed to the start of the access memory (this line return (void*)((unsigned char*) block + sizeof(Node)); ) - using linked list
{
    if (p == NULL) return 0; // error handling, if ptr sent through buddymalloc is null exit

    Node* block = (Node*)((unsigned char*)p - sizeof(Node)); // since the pointer is pointing at the start of the access memory, we need to move it back to the sizeof(node) because we need to know what the size allocation status is 
    block->alloc = 0; // update alloc to 0, 1 = block is currenty allocated, 0 = block isn't allocated, freeing so block isn't allocated inside nodes

    long long int totalSize = block->size + sizeof(Node); // total size includes header
    int order = MIN_ORDER; // currently 5 = 32bytes
    while ((1LL << order) < totalSize && order <= MAX_ORDER) { // find the correct order
        order++;
    }

    while (order < MAX_ORDER) { // try merge while not reaching max block size
        uintptr_t block_address = (uintptr_t)block; // uintptr_t, so we can do xor caluclations to the memory address, block is the pointer to the node ->  Node* block = (Node*)((unsigned char*)p - sizeof(Node));
        uintptr_t buddy_address = block_address ^ (1ULL << order); // same here, but we do an XOR caluclation where we caluclate the buddy_address (1ULL - 0001 in binary, << bitwise shift left which equals 0001 -> 100000 which the binary of that is 32)
        Node* buddy = (Node*)buddy_address; // buddy address converted back into a pointer to the node

        if (buddy < (Node*)wholememory || buddy >= (Node*)((unsigned char*)wholememory + MEMORYSIZE)) {
            break; // checks if buddy address is inside the memory range otherwise break
        }

		// Check if buddy is in the free list
        bool buddyInFreeList = false;
        Node* iter = free_lists[order];
        while (iter) {
            if (iter == buddy) {
                buddyInFreeList = true;
                break;
            }
            iter = iter->next;
        }

        if (!buddyInFreeList || buddy->alloc || buddy->size != block->size) {
            break; // checks if buddy size matches if not break
        }

        // qill unlink buddy from free list
        if (buddy->previous) {
            buddy->previous->next = buddy->next; // updates next pointer
        }
        else {
            free_lists[order] = buddy->next; // otherwise its the first node
        }
        if (buddy->next) {
            buddy->next->previous = buddy->previous; // if we move onto next node, remove previous pointer (buddy)
        }

        if (buddy < block) {
            block = buddy; // ensure it points to the lowest address
        }

        block->size = (1ULL << (order + 1)) - sizeof(Node);  // merge blocks
        block->alloc = 0;
        block->next = nullptr;
        block->previous = nullptr;
        order++; // move to next order after merging
    }

    // Insert block into free list (sorted by address to help future merges)
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

void printFreeListToFile(const std::string& filename) {
    std::ofstream out(filename, std::ios::app);
    if (!out) return;

    out << "\n--- Free List (debug print) ---\n";
    for (int order = MIN_ORDER; order <= MAX_ORDER; ++order) {
        out << "Order " << order << " (Block size: " << (1LL << order) << "): ";
        Node* current = free_lists[order];
        if (!current) {
            out << "[empty]\n";
            continue;
        }
        while (current) {
            out << "[addr: " << current << " | size: " << current->size << "] -> ";
            current = current->next;
        }
        out << "NULL\n";
    }
    out << "------------------------------\n";
}

//------------- Can comment out/remove -----------------

// comments
// ptr = buddyMalloc(size) = user inputs the size, then buddymalloc allocates a memory block large enough to hold it
// it will then return (void*)((unsigned char*)block + sizeof(Node)); void is the pointer that points to the start of the accesses the memory
// The address of a block's "buddy" is equal to the bitwise exclusive OR (XOR) of the block's address and the block's size. 
// block sizes are 2^order, buddy block size differs exactly between 2^order e.g/ block size is 0x1000 2^5, buddy address will be 0x1000 XOR 32