// Needs to handle queue's for entrances and exits
// Needs to handle the time that the car has been in there
// Simulat the time it takes for the boom gates to opperate

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include "resources/shared_mem.h"
#include "resources/queue.h"
#include "resources/generatePlate.h"
#include "resources/hashTable.h"

int fd;

// Mutex for queues
pthread_mutex_t queues_mutex;
pthread_cond_t queues_cond;

// Thread information struct
typedef struct thread_info{

    queue* queues;

    parking_data_t *shm;

    int num;

} thread_info_t;

void testSegment(parking_data_t *shm)
{
    printf("SIZE OF  ENTRY VALUE %ld\n", sizeof(entrys_t));       // Check size of entrys
    printf("SIZE OF  EXIT VALUE %ld\n", sizeof(exits_t));         // Check size of exits
    printf("SIZE OF  LEVEL VALUE %ld\n", sizeof(levels_t));       // Check size of levels
    printf("SIZE OF  PARKING SEGMENT VALUE %ld\n", sizeof(*shm)); // Check size of memory segment

    /* //Test Directly Accessing Entrance Boomgate Value to address
    char *c = (char*)shm + 184;
    *c = 'c';
     printf("TEST%p\n",c);

    //est Using pointers to assign Entrance Boomgate Value to address
     shm->entrys[0].boomgate = 'A'; */

    // Check Structs are mapped correctly in memory
    printf("MEMORY ADDRESS OF SHM + 184: %p\n", (char *)(shm + 184)); // Memory address of Entrance 1 boomgate

    // Wait for Manager to change value
    // while (shm->exits[1].boomgate != 'C')
    // sleep;

    printf("SHM VALUE Returned %c\n", shm->entrys[1].boomgate); // Print Value
}

// Create Shared Memory segment on startup.
void *create_shared_memory(parking_data_t *shm)
{

    // Check for previous memory segment.Remove if exits
    shm_unlink(SHARE_NAME);

    // Using share name and both creating and setting to read and write. Read and write for owner, group (0666). Fail if negative int is returned
    int open;
    open = shm_open(SHARE_NAME, O_CREAT | O_RDWR, 0666);

    if (open < 0)
    {
        printf("Failed to create memory segment");
    }
    fd = open;

    // Configure the size of the memory segment to 2920 bytes
    if (ftruncate(fd, sizeof(parking_data_t)) == -1)
    {
        printf("Failed to set capacity of memory segment");
    };

    // Map memory segment to pysical address
    shm = mmap(NULL, sizeof(parking_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (shm == MAP_FAILED)
    {
        printf("FAILED TO MAP shared memory segment.\n");
    }
    printf("Created shared memory segment.\n");
    printf("ADDRESS OF PARKING %p\n", shm);

    return shm;
}
// Listen to boomgates and remove from queue
void* remove_from_entry_queue(void* args){



    // Listen to boom gate cond
    printf("\n\na threads\n\n");

    // Get info from args
    queue* queues = ((thread_info_t *) args)->queues;
    parking_data_t *shm = ((thread_info_t *) args)->shm;
    int num = ((thread_info_t *)args)->num;

    free(args);

    printf("ADDRESS OF PARKING %p\n", shm);

    // Lock mutex
    pthread_mutex_lock(&shm->entrys[num].LPR_mutex);

    while(1){

        // Wait for LPR condition, unlock mutex until ready
        pthread_cond_wait(&shm->entrys[num].LPR_cond, &shm->entrys[num].LPR_mutex);

        // temp sleep
        sleep(2);

        // Allocate memory for removed car
        char *removed = malloc(sizeof(char)*6);



        // strcpy(removed, );

        printf("%s", removeFromQueue(&queues[num]));
        // free mem 
        // free(removed);

        // Free conditional
        pthread_cond_signal(&shm->entrys[num].LPR_cond);
    }

}
// Spawn car thread
void* spawn_cars(void* args){
    
    // Create array of queues  
    queue* q = malloc(Num_Of_Entries * sizeof(* q));

    // Create queue's needed and boomgate listeners
    for(int i =0; i < Num_Of_Entries; i++){

        // Init queue in array
        initQueue(&q[i]);

        // Allocate memory for variables in struct
        q[i].front = malloc(sizeof(item*));
        q[i].rear = malloc(sizeof(item*));
        q[i].count = *(int*)malloc(sizeof(int*));

        // Create struct to pass to each thread
        thread_info_t *ti = malloc(sizeof(thread_info_t));
        ti->queues = q;
        ti->shm = args;
        ti->num = i;

        // Init thread for listening to boom gates and removing 
        // from front of queue
        pthread_t bg_listener_thread;

        // Create thread for listening to LPR_cond variables
        pthread_create(&bg_listener_thread, NULL, remove_from_entry_queue, ti);

    }


    parking_data_t *shm = args;

    // Loop constantly
    while(1){
        // Need to generate new car at random queue every 1-100ms
        int interval = randomNumber();
        int counter = 0;        
        // Sleep for millisecond (Keeps thread asleep majority of the time)
        sleep(interval/1000);

        // Increase counter
        counter++;

        // Test to see if counter is at interval
        if(counter == interval)
        {
            // Get number plate to add
            char* numberPlate = generateNumberPlate();
            // Time is at interval, span new car
            addToQueue(&q[randomNumber()%5], numberPlate);
            pthread_cond_broadcast(&shm->entrys[randomNumber()%5].LPR_cond);
            counter =  0;
        }
        
    }

}





int main()
{


    parking_data_t parking; // Initilize parking segment
    parking_data_t *shm;    // Holds Mapped Shm Address

    // Create thread for adding cars to queues
    pthread_t spawn_car_thread;


    // Map Parking Segment to Memory and retrive address.
    shm = create_shared_memory(&parking);

    // USED TO TEST AND VALIDATE MEMORY SEGMENT

    // Create thread for each entry
    pthread_create(&spawn_car_thread, NULL, spawn_cars, shm);


    pthread_join(spawn_car_thread, NULL);
   
    // Close shared memory
    if ((munmap(shm, SHMSZ)) == -1)
    {
        perror("munmap failed");
    }

    close(fd);

    return 0;
}