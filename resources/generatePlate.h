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
    // Init mutex
    pthread_mutex_init(&rand_mutex, NULL);
    // Lock mutex
    pthread_mutex_lock(&rand_mutex);
    // Get random number
    int random = rand() % 100;
    // Unlock mutex for next call to function
    pthread_mutex_unlock(&rand_mutex);
    return random;
};

// Number plate generator
char *generateNumberPlate()
{

    // Need to create hash table for generated plates

    // Get file pointer
    FILE *plates = (FILE *)malloc(sizeof(FILE *));

    // Open file
    plates = fopen("./resources/plates.txt", "r");

    // Random numbe for testing
    int rand = randomNumber();

    // 50/50 whether car from list
    if (rand % 2 == 0)
    {

        // Pick from list
        // Get random number under 100
        rand = randomNumber() % 100;
        // Counter for choosing random plate from  file
        int counter = 0;
        int charCounter = 0;
        // String in progress
        char *numberPlate = (char *)calloc(6, sizeof(char));
        char *c = malloc(sizeof(c));
        // // Loop through all characters in plates
        for (*c = getc(plates); *c != EOF; *c = getc(plates))
        {

            // Get number of chars
            charCounter++;

            // If not new line char, add to string
            if (*c != '\n')
            {
                // Add char to string
                strncat(numberPlate, c, 1);
            }
            else
            {
                // Increment num lines
                counter++;

                // Return a number plate after a random number of lines under
                if (counter == rand)
                {

                    // Return current - after it is used it needs to be freed
                    return numberPlate;
                }

                // If not returning numberPlate, reset it
                memset(numberPlate, 0, (sizeof(char) * 6));
                // Reset pointer
                rewind(plates);
            }
        }


        // Close connection and free memory
        fclose(plates);
        free(plates);
        free(c);
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
        char *numberPlate = (char *)malloc(6 * sizeof(int));

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
        return (numberPlate);
    }
    return NULL;
};