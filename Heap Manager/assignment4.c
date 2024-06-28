#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE_SIZE 64
#define HEAP_SIZE 127
#define DEBUG 0
//0-126

unsigned char heap[HEAP_SIZE];
unsigned char MASK = 1; //00000001

char input[MAX_LINE_SIZE];
char* args[3];


//given an unsigned char representing a block header, returns the size of the block (first 7 bits)
unsigned char getSize(unsigned char header){
    return header >> 1;
}

//given an unsigned char representing a block header, returns if the block is allocated 0=free 1=allocated
unsigned char getIfAllocated(unsigned char header){
    return header & MASK;
}


//create an unsigned int representing a block header, give the block size and if the block is allocated
unsigned char makeHeader(unsigned char size, unsigned char isAllocated){

    if(DEBUG){printf("header created     size:%d, isallocated:%d\n", size, isAllocated);}

    return (size << 1) | (isAllocated & MASK);
}

//creates the first block
void initializeHeap(){
    for(int i = 0; i < HEAP_SIZE; i++){
        heap[i] = 0;
    }
    heap[0] = makeHeader(127, 0);
    
}

//returns the address of the next block, given an address of a header
unsigned char getNextBlock(unsigned char address){
    return address + getSize(heap[address]);
}

unsigned char mymalloc(int c){
    //best fit: find best fitting free block
    unsigned int bestFitSize = 128;
    unsigned char bestFitAddr; //of header

    for(unsigned char currentAddr = 0; currentAddr < 127; currentAddr = getNextBlock(currentAddr)){
        //printf("%x, %x, %x\n", currentAddr, getIfAllocated(currentAddr), getSize(currentAddr));
        if(!getIfAllocated(heap[currentAddr]) && getSize(heap[currentAddr]) >= c && getSize(heap[currentAddr]) < bestFitSize){
            bestFitAddr = currentAddr;
            bestFitSize = getSize(heap[currentAddr]);
        }
    }
    if(bestFitSize >= 128){
        if(DEBUG){printf("no address found\n");}
        return 0; //no address found
    }
    //bestFitAddr now stores the best fit address header, and bestFitSize stores its size (not including header)
    if(bestFitSize != c){
        //splitting block A into B|C
        unsigned char Bsize = c+1;
        unsigned char C_headerAddr = bestFitAddr+c+1;
        unsigned char Csize = bestFitSize - c - 1; //check this later

        heap[bestFitAddr] = makeHeader(Bsize, 1);
        if(bestFitSize != c+1){
            heap[C_headerAddr] = makeHeader(Csize, 0);
        }
        
    }
    else{
        heap[bestFitAddr] = makeHeader(c, 1);
    }
    //printf("%d\n", bestFitAddr+1);
    return bestFitAddr+1;
    
}
void myfree(unsigned char target);


void myrealloc(unsigned char targetAddr, unsigned char newSize){
    unsigned char headerAddr = targetAddr - 1;
    if(DEBUG){printf("realloc: size=%d\n",getSize(heap[headerAddr]));}

    if(getSize(heap[headerAddr]) == newSize+1){
        return;
    }
    else if(getSize(heap[headerAddr]) > newSize+1){
        //split block
        
        unsigned char Bsize = newSize+1;
        unsigned char C_headerAddr = headerAddr+newSize+1;
        unsigned char Csize = getSize(heap[headerAddr]) - newSize -1;
        
        heap[headerAddr] = makeHeader(Bsize, 1);

        heap[C_headerAddr] = makeHeader(Csize, 0);

        if(DEBUG){printf("created at addr: %d\n", headerAddr);}
        if(DEBUG){printf("created at addr: %d\n", C_headerAddr);}
        myfree(C_headerAddr+1);

        printf("%d\n", targetAddr);
    }
    else if(getSize(heap[headerAddr]) < newSize+1){
        //case 2: new size > current size

        //check block to the right
        //if its large enough, split it
        //otherwise do case 2b

        unsigned char nextBlockAddr = getNextBlock(headerAddr);
        //needed size = newSize+1
        
        if(getSize(heap[headerAddr]) + getSize(heap[nextBlockAddr]) > newSize+1){
            //A | BC
            //5, 20
            //AB | C
            //12, 13

            unsigned char leftNewSize = getSize(heap[headerAddr]) + newSize+1;
            unsigned char rightNewSize = getSize(heap[nextBlockAddr]) - (newSize+1);
            
            heap[headerAddr] = makeHeader(leftNewSize, 1);
            heap[getNextBlock(headerAddr)] = makeHeader(rightNewSize, 0);

            printf("%d\n", targetAddr);
        }
        else{
            unsigned char newAddr = mymalloc(newSize);
            myfree(targetAddr);
            printf("%d\n", newAddr);
            //return newAddr;
        }
    }


}
void myfree(unsigned char target){ //target = payload, //target-1 = header
    unsigned char blockAddr = target-1;
    unsigned char blockHeader = heap[target-1];

    if(getIfAllocated(heap[getNextBlock(blockAddr)])){ //no coalescing
        if(DEBUG){printf("created at addr: %d\n", target-1);}
        heap[target-1] = makeHeader(getSize(blockHeader), 0);
    }
    else{ //coalescing
        //printf("this block: %d, next block at: %d,next block size: %d\n", getSize(blockHeader), getNextBlock(blockAddr), getSize(heap[getNextBlock(blockAddr)]));
        if(DEBUG){printf("created at addr: %d\n", target-1);}
        heap[target-1] = makeHeader(getSize(blockHeader) + getSize(heap[getNextBlock(blockAddr)]), 0);    
    }
}

void blocklist(){
    for(unsigned char currentAddr = 0; currentAddr < 127; currentAddr = getNextBlock(currentAddr)){
        printf("%d, %d, %s\n", currentAddr+1, getSize(heap[currentAddr]) - 1, getIfAllocated(heap[currentAddr]) ? "allocated" : "free");
    }
}

void writemem(unsigned char target, char* input){
    int l = strlen(input);
    for(unsigned char i = 0; i < l; ++i){
        heap[target + i] = input[i];
    }
}

void printmem(unsigned char target, int numToPrint){

    for(int i = 0; i < numToPrint; ++i){
        if(i == 0){
            printf("%x", heap[target+i]); 
        }
        else{
            printf(" %x", heap[target+i]);
        }
        
    }
    printf("\n");
}

int main(){

    initializeHeap();

    int running = 1;
    while(running){
        printf(">");
        gets(input);

        args[0] = strtok(input, " ");
        args[1] = strtok(NULL, " ");
        args[2] = strtok(NULL, " ");


        if(strcmp(args[0], "malloc") == 0){
            printf("%d\n",mymalloc(atoi(args[1])));
        }
        else if(strcmp(args[0], "realloc") == 0){
            myrealloc(atoi(args[1]), atoi(args[2]));
        }
        else if(strcmp(args[0], "free") == 0){
            myfree(atoi(args[1]));
        }
        else if(strcmp(args[0], "blocklist") == 0){
            blocklist();
        }
        else if(strcmp(args[0], "writemem") == 0){
            writemem(atoi(args[1]), args[2]);
        }
        else if(strcmp(args[0], "printmem") == 0){
            printmem(atoi(args[1]), atoi(args[2]));
        }
        else if(strcmp(args[0], "printall") == 0){ //for debug porpoises
            for(int i = 0; i < HEAP_SIZE; i++){
                if(heap[i] != 0){
                    if(DEBUG){printf("%d: %x\n", i, heap[i]);}
                }
            }
        }
        else if(strcmp(args[0], "quit") == 0){
            running = 0;
        }
    }
    return 0;
}