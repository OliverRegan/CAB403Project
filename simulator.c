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
#include "resources/generatePlate.h"
#include "resources/hashTable.h"
#include <stdio.h>
#include "resources/test.h"
#include <time.h>

int fd;
parking_data_t *shm; // Initilize Shared Memory Segment

// Function Help from https://qnaplus.com/c-program-to-sleep-in-milliseconds/ Author: Srikanta
// Input microseconds
int threadSleep(long tms)
{
    usleep(tms * 1000);
}

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
            threadSleep(1000); // Wait 10ms for gate to close

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
    if (ftruncate(fd, SHMSZ) == -1)
    {
        printf("Failed to set capacity of memory segment");
    };

    // Map memory segment to pysical address
    shm = mmap(NULL, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (shm == MAP_FAILED)
    {
        printf("FAILED TO MAP shared memory segment.\n");
    }
    printf("Created shared memory segment.\n");
    printf("ADDRESS OF PARKING %p\n", shm);

    return shm;
}

// Initialize queue for each entrance
queue *init_queues()
{
    // Init queue array
    queue *queues = malloc(Num_Of_Entries * sizeof(queue *));

    // Allocate memory and assign pointer to array
    for (int i = 0; i < Num_Of_Entries; i++)
    {
        // Create queue
        struct queue *newQueue = malloc(sizeof(queue));
        newQueue->count = *(int *)malloc(sizeof(int));
        newQueue->front = (item *)malloc(sizeof(item));
        newQueue->rear = (item *)malloc(sizeof(item));

        // Initialize queue
        initQueue(newQueue);

        // Add queue to array
        queues[i] = *newQueue;
    };

    // Test initialization
    if (queues[0].count != 0)
    {
        return NULL;
    };

    // Return pointer to start of array
    return queues;
}
// Add number plate to random queue after certain amount of time
void generateCarAtEntry(queue *queues)
{
    // Needs to be threaded/mutexed
    // TBD
    // Lock mutex
    // pthread_mutex_lock(&queues_mutex);
    // Pick a queue
    int chosenQueue = randomNumber() % Num_Of_Entries;

    // Generate plate and add it to queue
    addToQueue((queue *)&queues[chosenQueue], generateNumberPlate());

    // Unlock mutex
    // pthread_mutex_unlock(&queues_mutex);
}

// Entry testing
void *testEntry(void *arg)
{
    // Create array of queues
    queue *queues = init_queues();

    // Loop through and add number
    for (int i = 0; i < Num_Of_Entries; i++)
    {
        printf("%d\ncount\n", queues[i].count);
        generateCarAtEntry(queues);
        printf("%d\nshouldbemore\n", queues[i].count);
        showQueue(queues[i].front);
    }
    for (int i = 0; i < 5; i++)
    {
        printf("removbing");
        while (!isEmpty((queue *)&queues[i]))
        {
            removeFromQueue((queue *)&queues[i]);
        }
    }
    for (int i = 0; i < Num_Of_Entries; i++)
    {
        for (int j = 0; j < queues[i].count; i++)
        {
            printf("hello");
            printf("%s", removeFromQueue(&queues[i]));
        }
    }

    return NULL;
}
void *spawn_cars(void *arg)
{
    printf("\n\n\n%d\n\n\nhi");
    testEntry(arg);
    free(arg);
    return NULL;
}
int main()
{

    // Define Queue's
    queue *q1 = (queue *)malloc(sizeof(queue));
    queue *q2 = (queue *)malloc(sizeof(queue));
    queue *q3 = (queue *)malloc(sizeof(queue));
    queue *q4 = (queue *)malloc(sizeof(queue));
    queue *q5 = (queue *)malloc(sizeof(queue));

    queue queues[] = {*q1, *q2, *q3, *q4, *q5};

    parking_data_t parking; // Initilize parking segment

    // Create thread for adding cars to queues
    pthread_t spawn_car_thread;

    // Map Parking Segment to Memory and retrive address.
    shm = create_shared_memory(&parking);

    // Initialise Mutex/Condition Variables and Set Default Values for Shared Memory
    setDefaultValues(shm);

    /* //Create thread
    pthread_create(&spawn_car_thread, NULL, spawn_cars, NULL);
    if (pthread_create(&spawn_car_thread, NULL, spawn_cars, NULL) != 0)
    {
        printf("Could not create thread");
    }   */

    // Do it again bc why not

    // testEntry();
    // spawn_cars();
    // pthread_join(spawn_car_thread, NULL);

    // Create BoomGate Threads
    pthread_t *threads = malloc(sizeof(pthread_t) * (Num_Of_Entries + Num_Of_Exits));
    createBoomGateThreads(shm, threads);

    // Generate Random Temperature
    // Helper Function from https://stackoverflow.com/questions/29381843/generate-random-number-in-range-min-max
    srand(time(0));
    for (;;)
    {
        threadSleep(1 + rand() % (5 + 1 - 1)); // New Temperature 1-5ms.
        for (int i = 0; i < Num_Of_Level; i++)
        {
            int16_t temp = 18 + rand() % (50 + 1 - 18); // Calculate new temperature between 18C-50C.
            shm->levels[i].temp = temp;                 // Set new temp to each level
        }
    }

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