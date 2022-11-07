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
parking_data_t *shm; // Initilize Shared Memory Segment
//size_t parking[Num_Of_Level];

// Mutex for queues
pthread_mutex_t queues_mutex;
pthread_cond_t queues_cond;

// Function Help from https://qnaplus.com/c-program-to-sleep-in-milliseconds/ Author: Srikanta
// Input microseconds
int threadSleep(long tms)
{
    usleep(tms * 1000);

    return 0;
}

// Thread information struct
typedef struct thread_info
{

    queue *queues;

    parking_data_t *shm;

    int num;

} thread_info_t;

// BoomGate Entrance Operations
void *runEntryBG(void *arg)
{
    int num = *(int *)arg; // ENTRANCE NUM

    for (;;)

    {
        pthread_mutex_lock(&shm->entrys[num].boomgate_mutex);
        while (shm->entrys[num].boomgate == 'C' || shm->entrys[num].boomgate == 'O')
        {
            pthread_cond_wait(&shm->entrys[num].boomgate_cond, &shm->entrys[num].boomgate_mutex);
        }
        // Boom Gate Rising.
        if (shm->entrys[num].boomgate == 'R')
        {

            printf("Boomgate at EntranceBG '%d' is Rising...\n", num + 1);
            printf("Status of Entrance BG: %d is %c \n\n ", num + 1, shm->entrys[num].boomgate);
            threadSleep(10); // Wait 10ms for gate to open

            // SET BOOMGATE TO OPEN
            shm->entrys[num].boomgate = 'O';
            printf("Boomgate at EntranceBG '%d' is Opened\n", num + 1);
            printf("Status of Entrance BG: %d is %c \n\n ", num + 1, shm->entrys[num].boomgate);
            pthread_cond_broadcast(&shm->entrys[num].boomgate_cond);
            pthread_mutex_unlock(&shm->entrys[num].boomgate_mutex);
        }

        // Boom Gate Lowering
        if (shm->entrys[num].boomgate == 'L')
        {
            printf("Boomgate at EntranceBG '%d' is Lowering...\n", num + 1);
            printf("Status of Entrance BG: %d is %c \n\n ", num + 1, shm->entrys[num].boomgate);
            threadSleep(10); // Wait 10ms for gate to close

            // SET BOOMGATE TO CLOSED
            shm->entrys[num].boomgate = 'C';
            printf("Boomgate at EntranceBG '%d' is Closed...\n", num + 1);
            printf("Status of Entrance BG: %d is %c \n\n ", num + 1, shm->entrys[num].boomgate);
            pthread_cond_broadcast(&shm->entrys[num].boomgate_cond);
            pthread_mutex_unlock(&shm->entrys[num].boomgate_mutex);
        }
    }

    free(arg);
}

// BoomGate Exit Operations
void *runExitBG(void *arg)
{
    int num = *(int *)arg; // EXIT NUM
    for (;;)

    {
        pthread_mutex_lock(&shm->exits[num].boomgate_mutex);
        // Boom Gate Rising.
        while (shm->exits[num].boomgate == 'C' || shm->exits[num].boomgate == 'O')
        {
            pthread_cond_wait(&shm->exits[num].boomgate_cond, &shm->exits[num].boomgate_mutex);
        }
        if (shm->exits[num].boomgate == 'R')
        {

            printf("Boomgate at ExitBG '%d' is Rising...\n", num + 1);
            printf("Status of Exit BG: %d is %c \n\n ", num + 1, shm->exits[num].boomgate);
            threadSleep(1000); // Wait 10ms for gate to open

            // SET BOOMGATE TO OPEN
            shm->exits[num].boomgate = 'O';
            printf("Boomgate at ExitBG '%d' is Opened\n", num + 1);
            printf("Status of ExitBG: %d is %c \n\n ", num + 1, shm->exits[num].boomgate);
            pthread_cond_broadcast(&shm->exits[num].boomgate_cond);
            pthread_mutex_unlock(&shm->exits[num].boomgate_mutex);
        }
        // Boom Gate Lowering
        if (shm->exits[num].boomgate == 'L')
        {
            printf("Boomgate at ExitBG '%d' is Lowering...\n", num + 1);
            printf("Status of ExitBG BG: %d is %c \n\n ", num + 1, shm->exits[num].boomgate);
            threadSleep(10); // Wait 10ms for gate to close

            // SET BOOMGATE TO CLOSED
            shm->exits[num].boomgate = 'C';
            printf("Boomgate at ExitBG '%d' is Closed...\n", num + 1);
            printf("Status of ExitBG: %d is %c \n\n ", num + 1, shm->exits[num].boomgate);
            pthread_cond_broadcast(&shm->exits[num].boomgate_cond);
            pthread_mutex_unlock(&shm->exits[num].boomgate_mutex);
        }
    }

    free(arg);
}

// Initilise Boomgate Threads
void createBoomGateThreads(parking_data_t *shm, pthread_t *threads)
{
    for (int i = 0; i < Num_Of_Entries; i++)
    {
        int *entNum = malloc(sizeof(int));
        *entNum = i;
        if (pthread_create(threads + i, NULL, runEntryBG, entNum) != 0)
        {
            printf("Error, could not create thread for boomgate.");
        };
    }

    for (int i = 0; i < Num_Of_Exits; i++)
    {
        int *extNum = malloc(sizeof(int));
        *extNum = i;
        if (pthread_create(threads + Num_Of_Entries + i, NULL, runExitBG, extNum) != 0)
        {
            printf("Error, could not create thread for boomgate.");
        };
    }
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
void *remove_from_entry_queue(void *args)
{

    // Listen to boom gate cond

    // Get info from args
    queue *queues = ((thread_info_t *)args)->queues;
    parking_data_t *shm = ((thread_info_t *)args)->shm;
    int num = ((thread_info_t *)args)->num;

    free(args);

    printf("ADDRESS OF PARKING %p\n", shm);

    // Lock mutex
    pthread_mutex_lock(&shm->entrys[num].LPR_mutex);

    while (1)
    {
        // Wait for LPR condition, unlock mutex until ready
        pthread_cond_wait(&shm->entrys[num].boomgate_cond, &shm->entrys[num].LPR_mutex);

        // temp sleep
        threadSleep(2);

        // Allocate memory for removed car
        char *removed = malloc(sizeof(char) * 6);
        if (queues[num].count != 0)
        {

            // Lock queues mutex
            pthread_mutex_lock(&queues_mutex);

            // remove from queue
            removed = removeFromQueue(&queues[num]);

            // Assign removed value to LPR
            strcpy(shm->entrys[num].lpr, removed);

            // Signal LPR cond of that entry
            pthread_cond_signal(&shm->entrys[num].LPR_cond);

            // Print removed - get rid  of this if needed
            printf("\nRemoving from entry queue:\n%s\n\n", removed);

            // Unlock queues mutex
            pthread_mutex_unlock(&queues_mutex);

            // Unlock LPR mutex
            pthread_mutex_unlock(&shm->entrys[num].LPR_mutex);
        }
    }
}
// Spawn car thread
void *spawn_cars(void *args)
{

    // Create array of queues
    queue *q = malloc(Num_Of_Entries * sizeof(*q));

    // Create queue's needed and boomgate listeners
    for (int i = 0; i < Num_Of_Entries; i++)
    {

        // Init queue in array
        initQueue(&q[i]);

        // Allocate memory for variables in struct
        q[i].front = malloc(sizeof(item *));
        q[i].rear = malloc(sizeof(item *));
        q[i].count = *(int *)malloc(sizeof(int *));

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
        // pthread_join(bg_listener_thread, NULL);
    }

    //parking_data_t *shm = args;

    // Init plates hash table for keeping track of allocated plates
    htable_t *plates_used = (htable_t *)malloc(sizeof(htable_t));

    // Init hashtable
    htable_init(plates_used, (size_t)Num_Of_Entries);

    while (1)
    {
        // Need to generate new car at random queue every 1-100ms
        int interval = (randomNumber()%100)+1;
        int counter = 0;
        // Sleep for millisecond (Keeps thread asleep majority of the time)
        // Increase counter
        counter++;
				
        // Test to see if counter is at interval
        if (counter == interval)
        {
            // Get number plate to add
            char *numberPlate=(char*)calloc(6,sizeof(char));
            
            while (1)
            {
				while(!(has_room(plates_used))){
					sleep(1);
					system("clear");
                    htable_print(plates_used);
                    
                    }
				
				
                numberPlate = generateNumberPlate();

                if (htable_find(plates_used, numberPlate) == NULL)
                {
                    // Sleep for alloted time when number plate is found
                    threadSleep(interval);
                    htable_add(plates_used, numberPlate);
                    htable_print(plates_used);
                    system("clear");
                    // Break and continue
                    break;
                }
                // reloop and reallocate number plate
                else
                {
                    free(numberPlate);
                    continue;
                }
            }
            pthread_mutex_lock(&queues_mutex);
            // Time is at interval, span new car
            int thread_num = randomNumber() % 5;
            addToQueue(&q[thread_num], numberPlate);
            // pthread_cond_signal(&shm->entrys[thread_num].LPR_cond);
            counter = 0;
            pthread_mutex_unlock(&queues_mutex);
        }
    }
}

// Temperature simulator
// For 500 counts generatre random set of temperatures for each level
void *temp_sensor(void *arg)
{
    int count = 0;
    for(;;)
    {
        if (count < 500)
        {
            for(int i = 0; i < Num_Of_Level; i++)
                shm->levels[i].temp = (short)((rand() % 10) + 20);

            usleep(((rand() % 5) + 20) * 2000);
        }
        else
        {
            for(int i = 0; i < Num_Of_Level; i++)
                shm->levels[i].temp = (short)((rand() % 10) + 58);

            usleep(((rand() % 5) + 20) * 2000);
            
        }
        count++;
    }
}

	

int main()
{

    parking_data_t parking; // Initilize parking segment

    // Create thread for adding cars to queues
    pthread_t spawn_car_thread;

     //Create thread for temperature sensor
    pthread_t temp_sensor_thread;

    // Map Parking Segment to Memory and retrive address.
    shm = create_shared_memory(&parking);
	
    // Initialise Mutex/Condition Variables and Set Default Values for Shared Memory
    setDefaultValues(shm);
    
    // Create thread for each entry
    pthread_create(&spawn_car_thread, NULL, spawn_cars, shm);

    // // Create BoomGate Threads
    pthread_t *threads = malloc(sizeof(pthread_t) * (Num_Of_Entries + Num_Of_Exits));
    createBoomGateThreads(shm, threads);

    // Initialise temp sensor threads
    pthread_create(&temp_sensor_thread, NULL, temp_sensor, NULL);
	
	srand(time(0));

    //INitialise conditions
    for (int i = 0; i < Num_Of_Level; ++i) {
        shm->levels[i].temp = 1;
        shm->levels[i].alarm = false;
    };

	
    pthread_join(spawn_car_thread, NULL);
    // Clean Up Threads and Shared Memory Mapping
    for (int i = 0; i < Num_Of_Entries + Num_Of_Exits; i++)
    {
        pthread_join(threads[i], NULL);
    }
    if ((munmap(shm, SHMSZ)) == -1)
    {
        perror("munmap failed");
    }
	
    close(fd);

    return 0;
}