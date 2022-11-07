#ifndef GENERATE_PLATE
#define GENERATE_PLATE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>
#include <string.h>

#define SHMSZ 2920
#define Num_Of_Entries 5
#define Num_Of_Exits 5
#define Num_Of_Level 5
#define Max_Per_Level 20

// Mutex for random numbers
pthread_mutex_t rand_mutex;
pthread_cond_t rand_cond;

int randomNumber()
{
    pthread_mutex_init(&rand_mutex, NULL);
    // Lock mutex
    pthread_mutex_lock(&rand_mutex);
    // Get random number
    int random = (rand() % 100)+1;
    // Unlock mutex for next call to function
    pthread_mutex_unlock(&rand_mutex);
    return random;
};

bool check_plate(char* cars){
	// Get file pointer
    
    if(cars==NULL){
		return false;
	}
    
    FILE *plates = (FILE *)malloc(sizeof(FILE *));
	
    // Open file
    plates = fopen("./resources/plates.txt", "r");
    char* numplate=(char*)calloc(7,sizeof(char));
    
    if(plates == NULL){
        return false;
    }
	while(fgets(numplate, 7, plates)!=NULL){
		if(strcmp(numplate,cars)==0){
		fclose(plates);
		return true;
		}
	}
    fclose(plates);
    return false;
}

// Number plate generator
char *generateNumberPlate()
{

    // Need to create hash table for generated plates
    // Get file pointer
    FILE *plates = (FILE *)malloc(sizeof(FILE *));

    // Open file
    plates = fopen("./resources/plates.txt", "r");
	// Init mutex
    pthread_mutex_lock(&rand_mutex);
    // Random numbe for testing
    int rand = randomNumber();
	
    // 50/50 whether car from list
    if (rand % 2 == 0)
    {
        // Pick from list
        // Get random number under 100
        int rand2 = randomNumber() % 100;
        // Counter for choosing random plate from  file
        int counter = 0;
			// Open file
		char* numplate= (char*)malloc(6*sizeof(char));
		while((fgets(numplate, 7, plates))!=0){
			counter++;
			if(counter==(rand2*2)+1){		
				return numplate;
			}
			
		}
        // Close connection and free memory
        fclose(plates);
        return NULL;
    }
    else
    {
        // Randommly generated number plate
        // ASCII limits for char
        int alphabetStart = 65;
        int alphabetEnd = 90;

        // ASCII limits for numbers
        int numStart = 48;
        int numEnd = 57;

        // Init number plate
        char *numberPlate = (char *)malloc(7 * sizeof(char));

        // Numbers for the first 3
        for (int i = 0; i < 3; i++)
        {
            numberPlate[i] = (randomNumber() % (numEnd - numStart)) + numStart;
        }

        // Alphatbet chars for the last 3
        for (int i = 3; i < 6; i++)
        {
            numberPlate[i] = (randomNumber() % (alphabetEnd - alphabetStart)) + alphabetStart;
        }
        // Memory needs to be freed when car leaves
        
        numberPlate[6]='\0';
        return (numberPlate);
    }
    return NULL;
};

#endif