#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_LEN 64
#define NUM_PAGES 16
#define MEMORY_SIZE 32 // amount of addresses in main memory
#define DISK_SIZE 128 // amount of addresses in disk
#define PAGE_SIZE 8


int disk[DISK_SIZE];
int main_memory[MEMORY_SIZE];

int main_data[4]; // for use in page removal algos
//FIFO: only uses main_data[0], which references the next page to remove. it goes in a loop 0-1-2-3-0-1-2-3-0...
//LRU: when a page is added or used, set its main_data value to 0 and add 1 to all others. 
//     when evicting, remove page with highest main_data value(it was used longest ago)

int main_pages[4]; //which page in being stored in main memory, -1 = none

struct PageTableEntry{
    int validBit;
    int dirtyBit;
    int pageNum;
};

struct PageTableEntry pageTable[NUM_PAGES];


int LRU; //0 for First-In-First-Out, 1 for Least Recently Used
char input[MAX_INPUT_LEN];
char* args[3];

int running = 1;


void LRU_UpdateMainData(int pageAccessed){ //0-3
    main_data[pageAccessed] = 0;
    for(int i = 0; i < MEMORY_SIZE / PAGE_SIZE; ++i){
        if(i != pageAccessed){
            main_data[i] += 1;
        }
    }
}

void pageReplace(int newPageIndex){
    int pageFrameToOverwrite = -1; //0-3

    //try to find an open page
    for(int i = 0; i < MEMORY_SIZE / PAGE_SIZE; ++i){
        if(main_pages[i] == -1){
            pageFrameToOverwrite = i;
            break;
        }
    }

    //if no open page exists, find victim using algorithm
    if(pageFrameToOverwrite == -1){
        if(LRU){ //LRU

            pageFrameToOverwrite = 0;
            int highest = 0;

            for(int i = 0; i < MEMORY_SIZE / PAGE_SIZE; ++i){
                if(main_data[i] > highest){
                    pageFrameToOverwrite = i;
                    highest = main_data[i];
                }
            }
        }
        else{ //FIFO
            pageFrameToOverwrite = main_data[0];
            main_data[0] += 1;
            main_data[0] %= 4;
        }
    }

    //if there is a page in the page frame to evict and it has been written to:
    if(main_pages[pageFrameToOverwrite] != -1 && pageTable[main_pages[pageFrameToOverwrite]].dirtyBit == 1){

        //move from main memory back to disk
        int startingIndexInMain = pageFrameToOverwrite * PAGE_SIZE;
        int startingIndexInDisk = main_pages[pageFrameToOverwrite] * PAGE_SIZE;
        for(int i = 0; i < PAGE_SIZE; i++){
            disk[startingIndexInDisk+i] = main_memory[startingIndexInMain+i];
        }
    }

    //move disk to main memory
    int startingIndexInMain = pageFrameToOverwrite * PAGE_SIZE;
    int startingIndexInDisk = newPageIndex * PAGE_SIZE;
    for(int i = 0; i < PAGE_SIZE; i++){
        main_memory[startingIndexInMain+i] = disk[startingIndexInDisk+i];
    }
    //update page table entries
    pageTable[newPageIndex].pageNum = pageFrameToOverwrite;
    pageTable[newPageIndex].validBit = 1;
    
    if(main_pages[pageFrameToOverwrite] != -1){ //if a page was evicted
        int pageJustEvicted = main_pages[pageFrameToOverwrite];

        pageTable[pageJustEvicted].dirtyBit = 0;
        pageTable[pageJustEvicted].validBit = 0;
        pageTable[pageJustEvicted].pageNum = pageJustEvicted;

    }


    //update main metadata
    main_pages[pageFrameToOverwrite] = newPageIndex;
    if(LRU){
        main_data[pageFrameToOverwrite] = 0;
    }
    

}

void readVM(int v_addr){
    int pageTableIndex = v_addr / PAGE_SIZE;

    if(pageTable[pageTableIndex].validBit == 1){

        int page_in_main_mem = pageTable[pageTableIndex].pageNum;
        int offset = v_addr % PAGE_SIZE;
        if(LRU){
            LRU_UpdateMainData(page_in_main_mem);
        }
        
        
        printf("%d\n", main_memory[page_in_main_mem * PAGE_SIZE + offset]);
    }
    else{
        printf("A Page Fault Has Occured\n");

        pageReplace(pageTableIndex);
        readVM(v_addr);
    }
}

void writeVM(int v_addr, int toWrite){
    int pageTableIndex = v_addr / PAGE_SIZE;

        if(pageTable[pageTableIndex].validBit == 1){

        int page_in_main_mem = pageTable[pageTableIndex].pageNum;
        int offset = v_addr % PAGE_SIZE;
        
        main_memory[page_in_main_mem * PAGE_SIZE + offset] = toWrite;
        pageTable[pageTableIndex].dirtyBit = 1;

        if(LRU){
            LRU_UpdateMainData(page_in_main_mem);
        }
        
    }
    else{
        printf("A Page Fault Has Occured\n");

        pageReplace(pageTableIndex);
        writeVM(v_addr, toWrite);
        
    }
}



void initializeMemory(){
    for(int i = 0; i < MEMORY_SIZE; ++i){
        main_memory[i] = -1;
    }
    for(int i = 0; i < DISK_SIZE; ++i){
        disk[i] = -1;
    }
    for(int i = 0; i < 4; ++i){
        main_data[i] = 0;
        main_pages[i] = -1;
    }
}

void initializePages(){
    for(int i = 0; i < NUM_PAGES; ++i){
        pageTable[i].validBit = 0;
        pageTable[i].dirtyBit = 0;
        pageTable[i].pageNum = i;
    }
}


//print page table with format -> [Virtual Page Number]:[Valid bit]:[Dirty Bit]:[Page Number]
void showPageTable(){
    for(int i = 0; i < NUM_PAGES; ++i){
        printf("%d:%d:%d:%d\n", i, pageTable[i].validBit, pageTable[i].dirtyBit, pageTable[i].pageNum);
    }
}
//print a page of main memory with format -> [Physical Address]: [Content at Address]
void showMain(int page){
    if(page < 0 || page >= MEMORY_SIZE / PAGE_SIZE){
        printf("Invalid Page: Must be 0-3");
        return;
    }
    int startAddress = page * PAGE_SIZE;
    for(int i = 0; i < PAGE_SIZE; ++i){
        printf("%d: %d\n",startAddress+i, main_memory[startAddress+i]);
    }
}

int main(int argc, char* argv[]){

    initializeMemory();
    initializePages();
    LRU = (argc > 1 && strcmp(argv[1], "LRU") == 0) ? 1 : 0; //set LRU to 1 only if the second argument is LRU, otherwise set it to 0


    while(running){
        printf("> ");

        gets(input);

        args[0] = strtok(input, " ");
        args[1] = strtok(NULL, " ");
        args[2] = strtok(NULL, " ");


        if(strcmp(args[0], "read") == 0){
            readVM(atoi(args[1]));
        }
        else if(strcmp(args[0], "write") == 0){
            writeVM(atoi(args[1]), atoi(args[2]));
        }
        else if(strcmp(args[0], "showmain") == 0){
            showMain(atoi(args[1]));
        }
        else if(strcmp(args[0], "showptable") == 0){
            showPageTable();
        }
        else if(strcmp(args[0], "quit") == 0){
            running = 0;
        }
        else{
            printf("Command not understood, try again\n");
        }
    }
    return 0;
}