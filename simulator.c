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

// Mutex for queues
pthread_mutex_t entry_queues_mutex;
pthread_cond_t entry_queues_cond;
pthread_mutex_t exit_queues_mutex;
pthread_cond_t exit_queues_cond;
pthread_mutex_t parked_cars_mutex;
pthread_cond_t parked_cars_cond;

// Define hashtable for parked cars
htable_t *parked_cars;
htable_t *plates_used;

// Init queue's for global use
queue *exit_q;
queue *q;

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

    // 0 for entry, 1 for exit
    int type;

} thread_info_t;

typedef struct parking_thread_info
{

    int num;

    char *plate;

} parking_thread_info_t;

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

// Park cars thread
// PRE: Car that is removed from queue, waits for time to get through boomgate and park
// Waits allotted time, then leaves to exit queue
// POST: Car is added to a queue at exit
void *park_car(void *args)
{
    // plates file init
    FILE *plates;

    // Allocate memory for file
    plates = (FILE *)malloc(sizeof(FILE *));

    // Open file
    plates = fopen("./resources/plates.txt", "r");

    // Get plate from args
    char *plate = malloc(sizeof(char) * 6);
    int *lvlNum = malloc(sizeof(int));

    plate = ((parking_thread_info_t *)args)->plate;
    *lvlNum = ((parking_thread_info_t *)args)->num;

    int is_allowed = 0;

    free(args);

    // printf("\n\n\n\n\n%s\n\n\n\n\n", ((parking_thread_info_t *)args)->plate);

    pthread_mutex_lock(&parked_cars_mutex);
    // If not in parked cars, add it to parked cars after wait
    if (htable_find(parked_cars, plate) == NULL)
    {
        pthread_mutex_unlock(&parked_cars_mutex);
        // Check if car is in file
        // Get file pointer
        char *numberPlate = (char *)calloc(6, sizeof(char));

        char *c = malloc(sizeof(c));

        // Loop over plates
        for (*c = getc(plates); *c != EOF; *c = getc(plates))
        {

            // If not new line char, add to string
            if (*c != '\n')
            {
                // Add char to string
                strncat(numberPlate, c, 1);
            }
            else
            {
                // Check if plates match
                if (strcmp(plate, numberPlate) == 0)
                {
                    // Set is allowed to true
                    is_allowed = 1;
                    break;
                }

                // If not returning numberPlate, reset it
                memset(numberPlate, 0, (sizeof(char) * 6));
            }
        }
        // Check if allowed - free mem and end thread if not
        if (is_allowed == 1)
        {
            // Wait time for car to park - approx 15ms
            threadSleep(20);

            // Add to parked cars
            htable_add(parked_cars, plate);

            // Trigger LPR for level it enterred at
            // Lock LPR mutex
            pthread_mutex_lock(&shm->levels[*lvlNum].LPR_mutex);
            // Change LPR and trigger cond
            strncpy(shm->levels[*lvlNum].lpr, plate, 6);

            pthread_cond_signal(&shm->levels[*lvlNum].LPR_cond);

            // Unlock mutex
            pthread_mutex_unlock(&shm->levels[*lvlNum].LPR_mutex);

            // Then pick random time to sleep - random
            // Between 100 and 10000 ms
            // random number is rand() < 100 * 100 will be a random number between 100 * 10000
            threadSleep(randomNumber() * 100);

            // Leave carpark
            // Trigger the LPR on a random level
            int randLvl = randomNumber() % 5;

            // Lock LPR mutex
            pthread_mutex_lock(&shm->levels[randLvl].LPR_mutex);

            // Change LPR and trigger cond
            strncpy(shm->levels[randLvl].lpr, plate, 6);

            // Unlock mutex
            pthread_mutex_unlock(&shm->levels[randLvl].LPR_mutex);

            // Enter the exit queue on the same level
            addToQueue(&exit_q[randLvl], plate);
            printf("deleting");
            // Remove from parked cars once in exit queue
            htable_delete(parked_cars, plate);
            htable_delete(plates_used, plate);

            // Free memory and exit thread
            free(plate);
            free(lvlNum);
            fclose(plates);

            pthread_exit(0);
        }
        else
        {
            free(plate);
            free(lvlNum);
            fclose(plates);

            pthread_exit(0);
        }
        free(numberPlate);
        free(c);
    }

    else
    {
        // Already in parked cars
        // Free mem and end thread
        free(plate);
        fclose(plates);
        pthread_exit(0);
    }

    return 0;
}

// Listen to boomgates and remove from queue
void *remove_from_queue(void *args)
{

    // Listen to boom gate cond

    // Get info from args
    queue *queues = ((thread_info_t *)args)->queues;
    parking_data_t *shm = ((thread_info_t *)args)->shm;
    int num = ((thread_info_t *)args)->num;
    int type = ((thread_info_t *)args)->type;

    free(args);

    // Lock mutex
    if (type == 0)
    {
        pthread_mutex_lock(&shm->entrys[num].LPR_mutex);
    }
    else
    {
        pthread_mutex_lock(&shm->exits[num].LPR_mutex);
    }
    while (1)
    {
        if (type == 0)
        {
            // Wait for LPR condition, unlock mutex until ready
            pthread_cond_wait(&shm->entrys[num].boomgate_cond, &shm->entrys[num].LPR_mutex);
        }
        else
        {

            // Wait for LPR condition, unlock mutex until ready
            pthread_cond_wait(&shm->exits[num].boomgate_cond, &shm->exits[num].LPR_mutex);
        }

        // temp sleep
        threadSleep(2);

        // Allocate memory for removed car
        char *removed = malloc(sizeof(char) * 6);

        if (queues[num].count != 0)
        {

            // Lock queues mutex
            if (type == 0)
            {
                pthread_mutex_lock(&entry_queues_mutex);
            }
            else
            {
                pthread_mutex_lock(&exit_queues_mutex);
            }

            // remove from queue
            removed = removeFromQueue(&queues[num]);

            if (type == 0)
            {
                // Assign removed value to LPR
                strncpy(shm->entrys[num].lpr, removed, 6);

                // Signal LPR cond of that entry
                pthread_cond_signal(&shm->entrys[num].LPR_cond);
            }
            else
            {
                // Assign removed value to LPR
                strncpy(shm->exits[num].lpr, removed, 6);

                // Signal LPR cond of that entry
                pthread_cond_signal(&shm->exits[num].LPR_cond);
            }
            // Print removed - get rid  of this if needed

            // If type is entry, start a park car thread to manage the car itself
            if (type == 0)
            {
                // Create and start thread to remove from parking
                pthread_t *parking_thread = malloc(sizeof(pthread_t));

                // Parking thread info
                parking_thread_info_t *p_info = malloc(sizeof(parking_thread_info_t));
                p_info->num = num;
                p_info->plate = removed;

                pthread_create(parking_thread, NULL, park_car, p_info);
            }
            else
            {

                // Free number plate once it has been copied to LPR
                free(removed);
            }

            // Unlock LPR mutex

            if (type == 0)
            {
                pthread_mutex_unlock(&shm->entrys[num].LPR_mutex);
            }
            else
            {
                pthread_mutex_unlock(&shm->exits[num].LPR_mutex);
            }

            // Unlock queues mutex
            if (type == 0)
            {
                pthread_mutex_unlock(&entry_queues_mutex);
            }
            else
            {
                pthread_mutex_unlock(&exit_queues_mutex);
            }
        }
    }
}
// Spawn car thread
void *spawn_cars(void *args)
{
    // plates file init
    FILE *plates;
    // Create array of queues
    q = malloc(Num_Of_Entries * sizeof(*q));

    // Create exit queues
    exit_q = malloc(Num_Of_Entries * sizeof(*exit_q));

    // Create queue's needed and boomgate listeners
    for (int i = 0; i < Num_Of_Entries; i++)
    {

        // Init queue in array
        initQueue(&q[i]);
        initQueue(&exit_q[i]);

        // Allocate memory for variables in struct
        q[i].front = malloc(sizeof(item *));
        q[i].rear = malloc(sizeof(item *));
        q[i].count = *(int *)malloc(sizeof(int *));
        exit_q[i].front = malloc(sizeof(item *));
        exit_q[i].rear = malloc(sizeof(item *));
        exit_q[i].count = *(int *)malloc(sizeof(int *));

        // Create struct to pass to each thread
        thread_info_t *entry_ti = malloc(sizeof(thread_info_t));
        entry_ti->queues = q;
        entry_ti->shm = args;
        entry_ti->num = i;
        entry_ti->type = 0;

        thread_info_t *exit_ti = malloc(sizeof(thread_info_t));
        exit_ti->queues = exit_q;
        exit_ti->shm = args;
        exit_ti->num = i;
        exit_ti->type = 1;

        // Init thread for listening to boom gates and removing
        // from front of queue
        pthread_t bg_listener_thread_entry;
        pthread_t bg_listener_thread_exit;

        // Create thread for listening to LPR_cond variables
        pthread_create(&bg_listener_thread_entry, NULL, remove_from_queue, entry_ti);
        pthread_create(&bg_listener_thread_exit, NULL, remove_from_queue, exit_ti);
        // pthread_join(bg_listener_thread, NULL);
    }

    // Init plates hash table for keeping track of allocated plates
    plates_used = (htable_t *)malloc(sizeof(htable_t));

    // Init hashtable
    htable_init(plates_used, (size_t)Num_Of_Entries);

    while (1)
    {
        // Need to generate new car at random queue every 1-100ms
        int interval = randomNumber();
        int counter = 0;
        // Sleep for millisecond (Keeps thread asleep majority of the time)

        // Increase counter
        counter++;

        // Test to see if counter is at interval
        if (counter == interval)
        {
            // Get number plate to add
            char *numberPlate;
            ;
            while (1)
            {

                numberPlate = generateNumberPlate();

                if (htable_find(plates_used, numberPlate) == NULL)
                {
                    // Sleep for alloted time when number plate is found
                    threadSleep(interval);

                    htable_add(plates_used, numberPlate);

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
            pthread_mutex_lock(&entry_queues_mutex);
            // Time is at interval, span new car
            int thread_num = randomNumber() % 5;
            addToQueue(&q[thread_num], numberPlate);
            // pthread_cond_signal(&shm->entrys[thread_num].LPR_cond);
            counter = 0;
            pthread_mutex_unlock(&entry_queues_mutex);
        }
    }
    fclose(plates);
}

int main()
{

    parking_data_t parking; // Initilize parking segment

    // Create thread for adding cars to queues
    pthread_t spawn_car_thread;

    // Allocate memory for parked cars
    parked_cars = (htable_t *)malloc(sizeof(htable_t));

    // Initiate parked cars hashtable - 5 buckets, will allocate hard 100 car limit in function
    htable_init(parked_cars, 5);

    // Map Parking Segment to Memory and retrive address.
    shm = create_shared_memory(&parking);

    // Initialise Mutex/Condition Variables and Set Default Values for Shared Memory
    setDefaultValues(shm);

    // Create thread for each entry
    pthread_create(&spawn_car_thread, NULL, spawn_cars, shm);

    // // Create BoomGate Threads
    pthread_t *threads = malloc(sizeof(pthread_t) * (Num_Of_Entries + Num_Of_Exits));
    createBoomGateThreads(shm, threads);

    threadSleep(10);

    pthread_cond_signal(&shm->exits[2].boomgate_cond);

    pthread_join(spawn_car_thread, NULL);

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