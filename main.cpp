
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Program Name:  Buddy System
//                  
// Author of start-up code: Napoleon Reyes
// Description:   Memory Management using the Buddy System Algorithm.
//                This work was based largely on the previous assignments made by Martin Johnson 
//                and Andre Barczak.
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Submitted by: 
//
//      Student name(s), ID number:
//
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//
// References:
//
//  Martin Johnson's codes and assignment design
//  Andre Barczak's codes and assignment design
//  https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes
//  https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
//
//  VIRTUAL ADDRESS SPACE AND PHYSICAL STORAGE: https://docs.microsoft.com/en-us/windows/win32/memory/virtual-address-space-and-physical-storage
//  MEMORY MANAGEMENT:  https://docs.microsoft.com/en-us/windows/win32/memory/about-memory-management
//  MEMORY PROTECTION CONSTANTS:
//     https://docs.microsoft.com/en-nz/windows/win32/memory/memory-protection-constants
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "auxiliary.h"
#include "buddysys.h"
#include <algorithm>
#include <cctype>
#include <chrono>

using namespace std;

//---------------------------------------
// DEFINE CONSTANTS
//---------------------------------------
unsigned seed;
Node *wholememory;
long long int MEMORYSIZE;
long long total_success_count = 0;
long long total_allocation_size = 0;
size_t peak_memory_usage = 0;
int builtin_failure_count = 0;

//---------------------------------------
// DEFINE NUMBER OF PAGES
//---------------------------------------
#define NUMBEROFPAGES  7169//8192//7200  
// 8192 pages = 4MB (4096 bytes per page) this is the maximum size of the memory block

//---------------------------------------
// DEFINE MODE 
//---------------------------------------
//#define DEBUG_MODE //enable to see more details

//---------------------------------------
// SELECT STRATEGY
//---------------------------------------
//#define USE_BUILTIN_MALLOC //uncomment to use built-in malloc/free functions
//#define USE_CUSTOM_MYALLOC //uncomment to use custom mymalloc/myfree functions
#define USE_BUDDY_SYSTEM  //uncomment to use Buddy System
//---------------------------------------
//(1) use built-in C functions
#if defined(USE_BUILTIN_MALLOC)
const string strategy = "Built-in malloc/free";
#define MALLOC malloc
#define FREE free
//---------------------------------------
//(2) use user-defined functions
#elif defined(USE_CUSTOM_MYALLOC)
const string strategy = "Custom mymalloc/myfree";
#define MALLOC mymalloc
#define FREE myfree
//---------------------------------------
//(3) use Buddy System
#elif defined(USE_BUDDY_SYSTEM)
const string strategy = "Buddy System";
#define MALLOC buddyMalloc
#define FREE buddyFree
//---------------------------------------
#else
#error "No strategy selected!"
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION
////////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
   int i,k;
   unsigned char *n[NO_OF_POINTERS]; // used to store pointers to allocated memory
   int size;
   unsigned int s[NO_OF_POINTERS]; // size of memory allocated - for testing

///////////////////////////////////////////////////////////
   //Initialise simulation variables

   seed=7652; //DO NOT CHANGE THIS SEED FOR RANDOM NUMBER GENERATION
   for(i=0;i<NO_OF_POINTERS;i++) {
      n[i]=0;     // initially nothing is allocated
   }
   
// ================== TEST CONFIG ==================

#if defined(RUN_SIMPLE_TEST) && defined(RUN_COMPLETE_TEST)
#error "You can't enable both RUN_SIMPLE_TEST and RUN_COMPLETE_TEST. Pick only one."
#endif

#if !defined(RUN_SIMPLE_TEST) && !defined(RUN_COMPLETE_TEST)
#define RUN_COMPLETE_TEST  // default
#endif

// ================== END CONFIG ===================

//---------------------------------------
// WHICH TEST ROUTINE?
//---------------------------------------
#ifdef RUN_SIMPLE_TEST
   cout << "=========================================" << endl;
   cout << "          << RUN SIMPLE TEST >>" << endl;
   cout << "=========================================" << endl;
#else
   #ifdef RUN_COMPLETE_TEST 
   cout << "=========================================" << endl;
   cout << "          << RUN COMPLETE TEST >>" << endl;
   #ifdef USE_SIMULATION_2
   cout << "          << SIMULATION 2 >>" << endl;
   #else
   cout << "          << SIMULATION 1 >>" << endl;
   #endif 
   cout << "=========================================" << endl;
   #endif

#endif
//---------------------------------------
//Record start time
//---------------------------------------

   auto start = std::chrono::steady_clock::now();

//---------------------------------------
//Record initial memory
//---------------------------------------

#ifndef USE_BUDDY_SYSTEM
   size_t initialMemory = getMemoryUsage();
   printf("\nInitial ");
   printMemoryUsage(initialMemory);
#endif
  
//---------------------------------------------------------------------------------------
#ifdef USE_BUDDY_SYSTEM
//acquire one wholememory block
   if (wholememory==NULL) {
        #ifndef RUN_SIMPLE_TEST
//MEMORYSIZE = (long long int) ((long long int)NUMBEROFPAGES * (long long int)PAGESIZE);
              MEMORYSIZE = (long long int) ((long long int)NUMBEROFPAGES * (long long int)PAGESIZE);
        #else
              MEMORYSIZE = 512; //bytes  -  RUN_SIMPLE_TEST  
        #endif
#if defined __unix__ || defined __APPLE__      
         wholememory=(Node*) Virtual_Alloc(MEMORYSIZE); 
#elif defined __WIN32__
//VirtualAlloc - Reserves, commits, or changes the state of a region of pages in the virtual address space of the calling process. Memory allocated by this function is automatically initialized to zero.
//  the return value is the base address of the allocated region of pages.
         wholememory=(Node*) VirtualAlloc(NULL, MEMORYSIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); //works!
#endif
         wholememory->size=(long long int)(MEMORYSIZE-(long long int)sizeof(Node));//Data size only          
         wholememory->next=NULL;
         wholememory->previous=NULL;
   
   printf("\n---<< MEMORY SETTINGS >>-------------------------------------");
   
   //Find pagesize 
   show_page_size();
   cout << "\twhole memory address: " << wholememory << "\n"; //print the address of the whole memory block
   printf("\tNode structure size: %d\n", sizeof(Node)); //print the size of the Node structure
   printf("\tInitial block: %lld bytes or %lld Megabytes.\n", wholememory->size, wholememory->size / (1024 * 1024));//print the size of the whole memory block
   printf("----------------------------------------------------------------");     
   }
#endif   

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//---------------------------------------
// Simulation start
//---------------------------------------
   
printf("\n<< Simulation start >>\n");
   
//---------------------------------------
//Test routines - Begin
//---------------------------------------
long long int totalAllocatedBytes=0;
long long int totalFreeSpace=0;
long long int totalSizeOfNodes=0;

#ifdef RUN_SIMPLE_TEST // Simple test with predef requests
  int NUM_OF_REQUESTS= 5; //sequence of requests
  char actions[] =      {'m' ,  'm', 'm', 'f', 'f'};  //m = MALLOC, f = FREE
  int requests[] =      {13, 3, 110, 3, 13};  //if NUMBEROFPAGES_BUDDY is set to 64
  int pointer_index[] = { 0,  1, 2, 1, 0};

//======================================================================================= 
    
  char selectedAction;
  cout << "\tExecuting " << NUM_OF_REQUESTS << " rounds of combinations of memory allocation and deallocation..." << endl;
  for(int r=0; r < NUM_OF_REQUESTS; r++){
     #ifdef DEBUGGINGPRT
        cout << "---[Iteration : " << r << "]" << endl;
      #endif
      selectedAction = actions[r];
      size = (int)requests[r];
      k = pointer_index[r];     
      switch(selectedAction){
        case 'm':
                   // #ifdef DEBUGGINGPRT
                       // cout << "\n======>REQUEST: MALLOC(" << size << ") =======\n\n";
                       cout << "\n======>REQUEST: n[" << k << "] = MALLOC(" << size << ") =======\n\n";  
                   // #endif
                    
                    
                    n[k]=(unsigned char *)MALLOC(size); // do the allocation
                    if(n[k] != NULL){
                       s[k]=size; // remember the size
                       totalAllocatedBytes = totalAllocatedBytes + s[k];
                       totalSizeOfNodes = totalSizeOfNodes + sizeof(Node);
                       #ifdef DEBUGGINGPRT
                          cout << "\t\t\t\tsuccessfully allocated memory of size: " << size << endl;   
                          //// printf("\t\tRETALLOC size %d at address %ld (block size %d at Nodeaddress %ld)\n",s[k], (Node*) ((uintptr_t)n[k]-(uintptr_t)wholememory),  s[k]+sizeof(Node), (Node*)( (uintptr_t)n[k]-(uintptr_t)sizeof(Node)-(uintptr_t)wholememory) );
                          
                          printf("\n\t\t\t\t << MALLOC() >>  relative address: %8ld size: %8d  (Node: %8ld Nodesize: %8d)\n", (Node*)((uintptr_t)n[k]-(uintptr_t)wholememory),  s[k], (Node*)((uintptr_t)n[k]-(uintptr_t)sizeof(Node)-(uintptr_t)wholememory) , s[k]+sizeof(Node) );
                       #endif   
                       

                       n[k][0]=(unsigned char) k;  // put some data in the first and 
               
                       if(s[k]>1) 
                          n[k][s[k]-1]=(unsigned char) k; // last byte

                    } else {
                       cout << "\tFailed to allocate memory of size: " << size << endl;   
                    }
                   break;
        case 'f':
                   // #ifdef DEBUGGINGPRT
                       cout << "\n======>REQUEST: FREE(n[" << pointer_index[r] << "]) =======\n\n";

                   // #endif
                   if(n[k]) { // if it was allocated then free it
                       // check that the stuff we wrote has not changed
                       
                       if ( (n[k][0]) != (unsigned char) k)//(n[k]+s[k]+k) )
                          printf("\t\t==>Error when checking first byte! in block %d \n",k);
                       if(s[k]>1 && (n[k][s[k]-1])!=(unsigned char) k )//(n[k]-s[k]-k))
                          printf("\t\t==>Error when checking last byte! in block %d \n",k);

                       #ifdef DEBUGGINGPRT
                         cout << "\n======>REQUEST: FREE(" << hex << n[k] << ") =======\n\n";
                         printf("\n\t\t\t\t << FREE() >>  relative address: %8ld size: %8d  (Node: %8ld Nodesize: %8d)\n", (Node*)((uintptr_t)n[k]-(uintptr_t)wholememory),  s[k], (Node*)((uintptr_t)n[k]-(uintptr_t)sizeof(Node)-(uintptr_t)wholememory) , s[k]+sizeof(Node) );
                       #endif
                       FREE(n[k]);
                       totalFreeSpace = totalFreeSpace + s[k];      

                    } 
                   break; 
      }
  }  
#endif

////////////////////////////////////////////////////////
#ifdef RUN_COMPLETE_TEST  
  printf("\n\tExecuting %d rounds of combinations of memory allocation and deallocation...\n", NO_OF_ITERATIONS);
  printf("Test has started!\n");
   for(i=0;i<NO_OF_ITERATIONS;i++) {
    #ifdef DEBUG_MODE
      cout << "iteration: " << i << endl;
    #endif
      k=myrand() % NO_OF_POINTERS; // pick a pointer
      if(n[k]) { // if it was allocated then free it
         // check that the stuff we wrote has not changed       
         if ( (n[k][0]) != (unsigned char) k)//(n[k]+s[k]+k) )
			 printf("Error when checking first byte! in block %d \n", k); // check first byte
         if(s[k]>1 && (n[k][s[k]-1])!=(unsigned char) k )//(n[k]-s[k]-k))
			 printf("Error when checking last byte! in block %d \n", k);// check last byte
         FREE(n[k]);         
      }
      size=randomsize(); // pick a random size
      #ifdef DEBUG_MODE
        printf("\tPick random size to allocate: %d\n", size);
      #endif   
      n[k]=(unsigned char *)MALLOC(size); // do the allocation
      if(n[k] != NULL){
          total_success_count++;
          total_allocation_size += size;
#ifdef USE_BUILTIN_MALLOC
          size_t currentUsage = getMemoryUsage();
          if (currentUsage > peak_memory_usage)
              peak_memory_usage = currentUsage;
#endif

#ifdef DEBUG_MODE
          printf("\tallocated memory of size: %d\n", size);
#endif   
         s[k]=size; // remember the size
         n[k][0]=(unsigned char) k;  // put some data in the first and 
         if(s[k]>1) n[k][s[k]-1]=(unsigned char) k; // last byte
      } else {
#ifdef USE_BUDDY_SYSTEM
          int failedOrder = getOrderSize(size);
          if (failedOrder <= MAX_ORDER) failure_counts[failedOrder]++;
#else
          builtin_failure_count++;
#endif

#ifdef DEBUG_MODE   
          printf("\tFailed to allocate memory of size: %d at iteration #%d\n", size, i);
#endif 
      }    
   }
#endif
//---------------------------------------
//Test routines - End
//---------------------------------------
   
   cout << "\n<< End of simulation >>\n\n"; 

//---------------------------------------
// Performance Report
//---------------------------------------
   auto end = std::chrono::steady_clock::now(); // Record end time
   auto time_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start); // Calculate elapsed time
   std::string upper_strategy = strategy; // Convert strategy to uppercase for display
   std::transform(upper_strategy.begin(), upper_strategy.end(), upper_strategy.begin(), ::toupper); // Convert to uppercase

   cout << "========================================================" << endl;
   cout << "          << " << upper_strategy << " PERFORMANCE REPORT >>" << endl;
   cout << "========================================================" << endl;

// --------- Our Buddy System Report ---------
#if defined(USE_BUDDY_SYSTEM)
   printf("\n---<< RESULTS >>---------------------------------------------");
   std::cout << "\n\tTime elapsed: " << time_elapsed.count() / 1e6 << " seconds" << std::endl;
   std::cout << "\tTime elapsed: " << time_elapsed.count() << " microseconds" << std::endl;
   std::cout << "\n\tSuccessful allocations: " << total_success_count;
   if (total_success_count > 0)
       std::cout << "\n\tAverage allocation size: " << (total_allocation_size / total_success_count) << " bytes";
   printf("\n----------------------------------------------------------------");

   printf("\n---<< Allocation Failures by Block Order >>---\n");
   for (int i = MIN_ORDER; i <= MAX_ORDER; ++i) {
       if (failure_counts[i] > 0)
           printf("Order %2d (%8lld bytes): %d failures\n", i, (1LL << i), failure_counts[i]);
   }
   printf("------------------------------------------------\n");

// --------- Built-in malloc/free Report ---------
#elif defined(USE_BUILTIN_MALLOC)
   size_t finalMemory = getMemoryUsage();
   size_t memoryUsed = finalMemory - initialMemory;
   printf("\nFinal ");
   printMemoryUsage(finalMemory);
   printf("\n---<< RESULTS >>---------------------------------------------");
   printf("\n\tUsed ");
   printMemoryUsage(memoryUsed);
   std::cout << "\n\tTime elapsed: " << time_elapsed.count() / 1e6 << " seconds" << std::endl;
   std::cout << "\tTime elapsed: " << time_elapsed.count() << " microseconds" << std::endl;
   std::cout << "\n\tSuccessful allocations: " << total_success_count;
   if (total_success_count > 0)
       std::cout << "\n\tAverage allocation size: " << (total_allocation_size / total_success_count) << " bytes";
   std::cout << "\n\tPeak memory usage: ";
   printMemoryUsage(peak_memory_usage);
   printf("\n----------------------------------------------------------------");
   printf("\n---<< Allocation Failures >>---\n");
   printf("Total malloc() failures: %d\n", builtin_failure_count);
   printf("------------------------------------------------\n");

#endif
  printf("------------------------------------------------\n");
   return 0;
}

