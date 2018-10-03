/*
CSC139 
Spring 2017
First Assignment
Peklar, Andrew
Section #01
OSs Tested on: Linux
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

// Size of shared memory block
// Pass this to ftruncate and mmap
#define SHM_SIZE 8192

// Global pointer to the shared memory block
// This should receive the return value of mmap
// Don't change this pointer in any function
void* gShmPtr;

// You won't necessarily need all the functions below
void Producer(int, int, int);
void InitShm(int, int);
void SetBufSize(int);
void SetItemCnt(int);
void SetIn(int);
void SetOut(int);
void SetHeaderVal(int, int);
int GetBufSize();
int GetItemCnt();
int GetIn();
int GetOut();
int GetHeaderVal(int);
void WriteAtBufIndex(int, int);
int ReadAtBufIndex(int);
int GetRand(int, int);


int main(int argc, char* argv[])
{
    pid_t pid;
    int bufSize; // Bounded buffer size
    int itemCnt; // Number of items to be produced
    int randSeed; // Seed for the random number generator 

    if(argc != 4){
        printf("Invalid number of command-line arguments\n");
        exit(1);
    }

    bufSize = atoi(argv[1]); 
    if(bufSize > 2000) { //bufSize cannot exceed 2000
        printf("Buffer size cannot exceed 2000\n");
        exit(1);
    }

    itemCnt = atoi(argv[2]);
    randSeed = atoi(argv[3]);

    // Function that creates a shared memory segment and initializes its header
    InitShm(bufSize, itemCnt);        

    /* fork a child process */ 
    pid = fork();

    if (pid < 0) { /* error occurred */
        fprintf(stderr, "Fork Failed\n");
        exit(1);
    }
    else if (pid == 0) { /* child process */
        printf("Launching Consumer \n");
        execlp("./consumer","consumer",NULL);
    }
    else { /* parent process */
        /* parent will wait for the child to complete */
        printf("Starting Producer\n");
        
        // The function that actually implements the production
        Producer(bufSize, itemCnt, randSeed);
        
        printf("Producer done and waiting for consumer\n");
       // wait(NULL);     
        printf("Consumer Completed\n");
    }
    
    return 0;
}


void InitShm(int bufSize, int itemCnt)
{
    int in = 0;
    int out = 0;
    const char *name = "OS_HW1_AndrewPeklar"; // Name of shared memory object to be passed to shm_open

    int shm_fd;


    //creates the shared memory object
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

    // configure the size of the shared memory object 
    ftruncate(shm_fd , SHM_SIZE);

    // memory map the shared memory object 
    gShmPtr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    // Write code here to set the values of the four integers in the header
    SetBufSize(bufSize);
    SetItemCnt(itemCnt);
    SetIn(in);
    SetOut(out);           
}

void Producer(int bufSize, int itemCnt, int randSeed)
{
    int in = 0;		//first fill
    int out = 0;	//first full

    int i;
    int val;
	
    //seed value used by GetRand()
    srand(randSeed);

    //buffer is full at bufsize-1 --> launches consumer.
    //out = in + 1; while loop is the "wait"
    //write and overwrite with random value a{0...1000}
    for(i=0; i < itemCnt; i++) {
	in  = GetIn(); 
        while(((in + 1) % GetBufSize()) == GetOut()); 
        WriteAtBufIndex(in, GetRand(0, 1000));
        val = ReadAtBufIndex(in); 
	//i+1 for item count starting at 1; index vals start at 0
        printf("Producing Item %4d with value %3d at Index %d\n", i+1, val, in);
        SetIn((in + 1) % bufSize);
    }      
	
    printf("Producer Completed\n");
}

// Set the value of shared variable "bufSize"
void SetBufSize(int val)
{
        SetHeaderVal(0, val);
}

// Set the value of shared variable "itemCnt"
void SetItemCnt(int val)
{
        SetHeaderVal(1, val);
}

// Set the value of shared variable "in"
void SetIn(int val)
{
        SetHeaderVal(2, val);
}

// Set the value of shared variable "out"
void SetOut(int val)
{
        SetHeaderVal(3, val);
}

// Set the value of the ith value in the header
void SetHeaderVal(int i, int val)
{
        void* ptr = gShmPtr + i*sizeof(int);
        memcpy(ptr, &val, sizeof(int));
}

// Get the value of shared variable "bufSize"
int GetBufSize()
{       
        return GetHeaderVal(0);
}

// Get the value of shared variable "itemCnt"
int GetItemCnt()
{
        return GetHeaderVal(1);
}

// Get the value of shared variable "in"
int GetIn()
{
        return GetHeaderVal(2);
}

// Get the value of shared variable "out"
int GetOut()
{             
        return GetHeaderVal(3);
}

// Get the ith value in the header
int GetHeaderVal(int i)
{
        int val;
        void* ptr = gShmPtr + i*sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

// Write the given val at the given index in the bounded buffer 
void WriteAtBufIndex(int indx, int val)
{
        // Skip the four-integer header and go to the given index 
        void* ptr = gShmPtr + 4*sizeof(int) + indx*sizeof(int);
        memcpy(ptr, &val, sizeof(int));
}

// Read the val at the given index in the bounded buffer
int ReadAtBufIndex(int indx)
{
        int val;

        // Skip the four-integer header and go to the given index
        void* ptr = gShmPtr + 4*sizeof(int) + indx*sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

// Get a random number in the range [x, y]
int GetRand(int x, int y)
{
    int r = rand();
    r = x + r % (y-x+1);
    return r;
}
