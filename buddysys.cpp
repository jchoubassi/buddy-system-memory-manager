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

#define MIN_ORDER 5 // 32 bytes, adjust ?
#define MAX_ORDER 20 //adjust
#define HEADER_SIZE sizeof(Node)

Node *free_lists[MAX_ORDER + 1] = {nullptr};
bool initialized = false;

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
    if (order <= MIN_ORDER || order > MAX_ORDER || !free_lists[order])
        return;
    
    Node* bigBlock = free_lists[order];
    free_lists[order] = bigBlock->next;
    
    if (bigBlock -> next)
        bigBlock->next->previous = nullptr;
    
    long long int halfSize = (1LL << (order - 1));

    //buddy 1
    Node* buddy1 = bigBlock;
    buddy1 -> size = halfSize - sizeof(Node);
    buddy1 -> alloc = 0;

    //buddy 2 after
    Node* buddy2 = (Node*)((byte*) buddy1 + halfSize);
    buddy2 -> size = halfSize - sizeof(Node);
    buddy2 -> alloc = 0;

    //link buddies
    buddy1 -> next = buddy2;
    buddy1 -> previous = nullptr;
    buddy2 -> next = free_lists[order - 1];

    if(buddy2 -> next)
        buddy2 -> next -> previous = buddy2;
    
    buddy2 -> previous = buddy1;
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

void *buddyMalloc(int request_memory){

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
        return nullptr; // No available block found
    }

    //2. split larger block to desired order
    while (currentOrder > desiredOrder) {
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
    printFreeList(); //DEBUGGING, CAN REMOVE LATER
    return (void*)((byte*) block + sizeof(Node));

    void *p = NULL; //dummy statement only

    return p; //dummy statement only
} 

int buddyFree(void *p){

   ///////////////////////////////////
   //put your implementation here

   return 1; //dummy statement only
}

//------------- Debugging Functions -----------------
//Called in buddyMalloc if need to remove
void printFreeList() {
    printf("----- Free Lists -----\n");
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
    printf("-------------------------\n");
}
//------------- Can comment out/remove -----------------