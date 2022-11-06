// --------------------LIBRARIES--------------------
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
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
// --------------------DEFINITIONS--------------------
// Carpark format
#define LEVELS  5   //Given from task - how many carpark levels there are
#define ENTRANCES   5   //Given from task - how many entrances there are
#define EXITS   5   //Given from task - how many exits there are

// Temperature Information
#define MEDIAN_WINDOW 5 //Given from task - 5 most recent temp readings - median temperature will be recorded as smoothes
#define TEMPCHANGE_WINDOW 30 //Given from task - 30 most recent smoothed temperatures produced by sensor - if 90% of these are 58 degrees _ the temperature is considered high enough that there must be a fire

#define FIXED_TEMP 58 // Given from task - Average temperature max limit
#define RISE_TEMP 8 //Given from task - If temperature is 8+ hotter than the 30 most recent temperatures then the temperature is considered to be growing at a fast enough rate that there must be a fire

#define SHARE_NAME "PARKING"
#define SHM_SIZE 2920 

#define START_COUNT 5
#define TEMP_COUNT 35

#define SMOOTH_TEMPS 5
#define MEDIAN_COUNT 30

bool alarm_active = false;
parking_data_t* shm;
int shm_fd;

void* temp_monitor(void* ptr) {
	int thread = *((int*)ptr);
	int temperature;
	temperature = shm->levels[thread].temp;

	// --TEMPERATURE MONITOR (VIA WHILE LOOP)--
	while(temperature != 0) {

		//Initialise List
		int temp_list[TEMP_COUNT];
		int median_list[MEDIAN_COUNT];

		//Initialise Variables
		int count = 0;
		int median_temp;
		int fixedTempCount;
		int iterations = 0;

		//First five temps
		for (int i = 0; i < LEVELS; i++) {
			temperature = shm->levels[thread].temp;
			temp_list[i] = temperature;
		}

		// Temps for smoothings
		for (count = 5; count < TEMP_COUNT; count++) {
			temperature = shm->levels[thread].temp;
			temp_list[count] = temperature;

			int tempor_list[5]; //Create temporary list
			for (int i = 0; i < 5; i++) {
				tempor_list[i] = temp_list[count - 5 + i];
			}

			// Sort temps in ascending order (from tutorial)
			int n = sizeof(tempor_list) / sizeof(tempor_list[0]);
			int ix, jx, min_ix;

			for (ix = 0; ix < n - 1; ix++)
			{
				// Find the minimum element in unsorted array
				min_ix = ix;
				for (jx = ix + 1; jx < n; jx++)
					if (tempor_list[jx] < tempor_list[min_ix])
						min_ix = jx;

				// Swap the found minimum element with the first element
				int temperature = temp_list[min_ix];
				tempor_list[min_ix] = tempor_list[ix];
				tempor_list[ix] = temperature;
			}

			// Find median temp
			median_temp = tempor_list[2];
			

			median_list[iterations] = median_temp;
			iterations++;
		}
		
		//Calcualte fixed temp -> for fire detections
		fixedTempCount = 0;
		for (int i = 0; i < MEDIAN_COUNT; i++) {

			if (median_list[i] >= FIXED_TEMP) {
				fixedTempCount++;
			}
		}

        //--Fire detected via majority of temps over threshold--
        // 27 = 90% of 30
		if (fixedTempCount >= 27) {
			alarm_active = true;
		}

		//-- Fire detection via temp rise--
		if ((median_list[MEDIAN_COUNT] - median_list[0]) > 8) {
			alarm_active = true;
		}

		usleep(2000);
	}
	return NULL;
}


int main()
{
	//-- SHARED MEMORY--
	if ((shm_fd = shm_open(SHARE_NAME, O_RDWR, 0)) < 0)
	{
		perror("shm_open");
		return 1;
	}
	if ((shm = (parking_data_t*)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void*)-1)
	{
		perror("mmap");
		return 1;
	}

	// -- CREATE THREADS FOR EACH LEVEL--
	pthread_t threads[LEVELS];

	int level[LEVELS];
	for (int i = 0; i < LEVELS; i++) {

		level[i] = i;
		pthread_create(&threads[i], NULL, temp_monitor, &level[i]);
	}

	while(shm->levels[0].temp >= 0) {
		//-- ACTIVE ALARM--
		if (alarm_active) {
			fprintf(stderr, "---- ACTIVE ALARM ----\n");

			//Alarm system and boommgatees
			for (int i = 0; i < LEVELS; i++) {
				shm->levels[i].alarm = true;
			}

			//--Display on sign evacuation message--
			char* evacuate_msg = "EVACUATE";
			for (char* p = evacuate_msg; *p != '\0'; p++) {
				for (int i = 0; i < LEVELS; i++) {
					pthread_mutex_lock(&shm->entrys[i].info_mutex);

					shm->entrys[i].display = *p;
					pthread_cond_signal(&shm->entrys[i].info_cond);

					pthread_mutex_unlock(&shm->entrys[i].info_mutex);
				}
				usleep(20000);
			}
			alarm_active = false;
		}
		else {
			for (int i = 0; i < LEVELS; ++i) {
				shm->levels[i].alarm = false;
			}
		}
		usleep(1000);
	}

	munmap(shm, 2920);
	close(shm_fd);
}
