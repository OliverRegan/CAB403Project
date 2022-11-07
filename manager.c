#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "resources/shared_mem.h"
#include "resources/generatePlate.h"
#include "resources/hashTable.h"

int fd;
int i = 0, add = 0, gen = 0;
int notFull = 0;
parking_data_t *shm; // Initilize Shared Memory Segment
pthread_mutex_t addMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t numMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hashMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t genMutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t finishMutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t addCond = PTHREAD_COND_INITIALIZER;
// pthread_cond_t finishCond=PTHREAD_COND_INITIALIZER;
//  Function Help from https://qnaplus.com/c-program-to-sleep-in-milliseconds/ Author: Srikanta
//  Input microseconds
void threadSleep(long tms)
{
    usleep(tms * 1000);
}

void *display_sign(void *arg)
{
    // parking_data_t *shm = (parking_data_t*)arg;
    for (;;)
    {
        // Display Entrances
        printf("----------------ENTRANCES--------------\n");
        printf("LEVEL 1 LPR: %s     |     BG: %c   | 	Status: %c\n", shm->entrys[0].lpr, shm->entrys[0].boomgate, shm->entrys[0].display);
        printf("LEVEL 2 LPR: %s     |     BG: %c   | 	Status: %c\n", shm->entrys[1].lpr, shm->entrys[1].boomgate, shm->entrys[1].display);
        printf("LEVEL 3 LPR: %s     |     BG: %c   | 	Status: %c\n", shm->entrys[2].lpr, shm->entrys[2].boomgate, shm->entrys[2].display);
        printf("LEVEL 4 LPR: %s     |     BG: %c   | 	Status: %c\n", shm->entrys[3].lpr, shm->entrys[3].boomgate, shm->entrys[3].display);
        printf("LEVEL 5 LPR: %s     |     BG: %c   | 	Status: %c\n", shm->entrys[4].lpr, shm->entrys[4].boomgate, shm->entrys[4].display);

        // Display Exits
        printf("------------------EXITS----------------\n");
        printf("LEVEL 1 LPR: %s     |     BG: %c\n", shm->exits[0].lpr, shm->exits[0].boomgate);
        printf("LEVEL 2 LPR: %s     |     BG: %c\n", shm->exits[1].lpr, shm->exits[1].boomgate);
        printf("LEVEL 3 LPR: %s     |     BG: %c\n", shm->exits[2].lpr, shm->exits[2].boomgate);
        printf("LEVEL 4 LPR: %s     |     BG: %c\n", shm->exits[3].lpr, shm->exits[3].boomgate);
        printf("LEVEL 5 LPR: %s     |     BG: %c\n", shm->exits[4].lpr, shm->exits[4].boomgate);

        // Display Temperature
        printf("\n");
        for (int i = 0; i < Num_Of_Level; ++i)
        {
            if (i == Num_Of_Level - 1)
            {
                printf("| LEVEL %d TEMP: %d\n\n", i + 1, shm->levels[i].temp);
            }
            else
            {
                printf("| LEVEL %d TEMP: %d ", i + 1, shm->levels[i].temp);
            }
        }

        // PRINT ALARM
        printf("| Alarm: %d\n\n", shm->levels[0].alarm);
        threadSleep(50); // Updates Every 'x' amount of milliseconds
        system("clear");
    }
    return 0;
}

// Read Shared Memory segment on startup.
void *read_shared_memory(parking_data_t *shm)
{

    // Using share name, return already created Shared Memory Segment.
    int open;
    open = shm_open(SHARE_NAME, O_RDWR, 0666);

    if (open < 0)
    {
        printf("Failed to create memory segment");
    }
    fd = open;

    // Map memory segment to physical address
    shm = mmap(NULL, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (shm == MAP_FAILED)
    {
        printf("FAILED TO MAP shared memory segment.\n");
    }
    printf("Created shared memory segment.\n");
    printf("ADDRESS OF PARKING %p\n\n", shm);

    return shm;
}

int total = 0;

void *addToHash(void *hashpointer)
{
    htable_t *hash = hashpointer;
    pthread_mutex_lock(&numMutex);
    int a = add;
    add++;
    char *balls = (char *)malloc(6 * sizeof(char));
    pthread_mutex_unlock(&numMutex);
    while (has_room(hash))
    {
        while (*shm->entrys[a].lpr == 0)
        {
            pthread_cond_wait(&shm->entrys[a].LPR_cond, &shm->entrys[a].LPR_mutex);
        }
        // pthread_mutex_lock(&hashMutex);

        shm->entrys[a].boomgate = 'R';
        threadSleep(1000);
        shm->entrys[a].boomgate = 'O';
        if (check_plate(shm->entrys[a].lpr))
        {
            if (htable_find(hash, shm->entrys[a].lpr) == NULL)
            {
                htable_add(hash, shm->entrys[a].lpr);
                // printf("%d\n",total);
            }
        }
        else
        {
        }
        *shm->entrys[a].lpr = 0;
        threadSleep(1000);
        shm->entrys[a].boomgate = 'L';
        threadSleep(1000);
        shm->entrys[a].boomgate = 'C';
        threadSleep(1000);

        balls = generateNumberPlate();
        if (check_plate(balls))
        {
            strncpy(shm->exits[a].lpr, balls, 6);
            threadSleep(1000);
            *shm->exits[a].lpr = 0;
            htable_delete(hash, balls);
        }
        threadSleep(100);
        // pthread_mutex_unlock(&hashMutex);
    }
    free(balls);
    notFull++;
    return 0;
}

void *gen_plates(void *hashpointer)
{
    htable_t *hash = hashpointer;
    int i = 0;
    while (notFull == 0)
    {
        i = randomNumber() % 5;
        while (shm->entrys[i].boomgate != 'C')
        {
            threadSleep(100);
        }

        char *randPlate = (char *)malloc(6 * sizeof(char));
        randPlate = generateNumberPlate();
        if (htable_find(hash, randPlate) != NULL)
        {
        }
        else
        {
            strncpy(shm->entrys[i].lpr, randPlate, 6);
            if (!pthread_cond_signal(&shm->entrys[i].LPR_cond))
            {
            }
            else
            {
                perror("Signal failed\n");
            }
        }
        threadSleep(100);
        free(randPlate);
    }

    return 0;
}

int main()
{
    htable_t *hashtable = (htable_t *)malloc(sizeof(htable_t));

    pthread_t entThreads[Num_Of_Entries], addThreads;

    parking_data_t parking; // Initilize parking segment

    // Map Parking Segment to Memory and retrive address.
    shm = read_shared_memory(&parking);

    for (int l = 0; l < 5; l++)
    {
        *shm->entrys[l].lpr = 0;
    }

    htable_init(hashtable, (size_t)Num_Of_Level);

    for (int m = 0; m < Num_Of_Entries; m++)
    {
        if (pthread_create(&entThreads[m], NULL, addToHash, hashtable))
        {
            perror("Entrance Threads Failed");
        }
    }
    if (pthread_create(&addThreads, NULL, gen_plates, hashtable))
    {
        perror("Hash Threads Failed");
    }

    int run;
    pthread_t display;
    if ((run = pthread_create(&display, NULL, display_sign, shm)) != 0)
    {
        perror("Well shit");
    }

    while (has_room(hashtable))
    {

        sleep(1);
    }
    htable_print(hashtable);

    if ((munmap(shm, SHMSZ)) == -1)
    {
        perror("munmap failed");
    }
    htable_destroy(hashtable);
    return 0;
}