#ifndef HASH_TABLE
#define HASH_TABLE
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

// Hashtable for parked cars
typedef struct car car_t;
struct car{
    // Plate details - plate used as key
    char *plate;

    // Next parked car in hashtable
    car_t *next;
    
    double tv;
};

// Define the hashtable itself

struct htable{
    // Arrays for cars
    car_t **buckets;
    // Size of hashtable
    size_t size;
    int counts;
};
typedef struct htable htable_t;

bool has_room(htable_t *hashTable){
	if(hashTable->counts<100){
		return true;
	}
	
	return false;
}

// Print car for testing
void car_print(car_t *car)
{
	char* balls=car->plate;
    //printf("plate=%s", balls);
}

// Initialize the hashtable
bool htable_init(htable_t *hashTable, size_t n)
{
    // Allocate size of hashtable - how many buckets
    hashTable->size = n;
    hashTable->counts=0;
    // Reset buckets to be 0's
    hashTable->buckets = (car_t **)calloc(n, sizeof(car_t *));
    // Test if calloc has reset buckets and return
    return hashTable->buckets != 0;
}

// Hashing function
// Popular hashing function fount on ln47 of hashtable_search.c from week 5 pracs
size_t djb_hash(char *s)
{
    size_t hash = 5381;
    int c;
    while ((c = *s++) != '\0')
    {
        // hash = hash * 33 + c
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Find hash offset for new entry in hashtable
size_t htable_index(htable_t *hashTable, char *plate)
{
    // Return hashed number / buckets remainder
    return djb_hash(plate) % hashTable->size;
}

// Get the bucket of car with plate
car_t *htable_bucket(htable_t *hashTable, char *plate)
{
    // Get pointer to correct bucket that has car in it
    return hashTable->buckets[htable_index(hashTable, plate)];
}

// Find car in hashtable
// PRE: true
// POST: Return NULL if not found OR return *car to car with plate
car_t *htable_find(htable_t *hashTable, char *key)
{
    // Start loop at first element in bucket that the key would be in
    for (car_t *i = htable_bucket(hashTable, key); i != NULL; i = i->next)
    {
        // Test if key of current car is == to searched key
        if (strcmp(i->plate, key) == 0)
        {
            return i;
        }
    }
    return NULL;
}

// Add to hashtable
// PRE: htable_find(hashTable, key) == NULL
// POST: htable_find(hashTable, key) != NULL OR return false
bool htable_add(htable_t *hashTable, char *plate)
{
	size_t bucket = htable_index(hashTable, plate);
    // Create new head for hashtable bucket
    car_t *newHead = (car_t *)malloc(sizeof(car_t));
    // Check if newHead has been allocated
    if (newHead == NULL)
    {
        return false;
    }
	struct timeval tv;
    // Get bucket
    gettimeofday(&tv, NULL);
	newHead->tv = (tv.tv_usec)*1000 ; // convert tv_sec & tv_usec to millisecond
	//printf(" %.1lf \n",newHead->tv);
    //printf(" |%s| please go to floor: %zu\n", plate, bucket);
	

    // Shuffle current head along
    newHead->next = hashTable->buckets[bucket];
	hashTable->counts++;
	// Assign value to newHead
	
    newHead->plate = strdup(plate);
    // Assign new car to bucket
    hashTable->buckets[bucket] = newHead;
    
    return true;
}

// Print the hash table.
// PRE: true
// POST: hash table is printed to screen
void htable_print(htable_t *hashTable)
{
    //printf("hash table with %d buckets\n", (int)hashTable->size);
    for (size_t i = 0; i < hashTable->size; ++i)
    {
        //printf("bucket %d: ", (int)i);
        if (hashTable->buckets[i] == NULL)
        {
            //printf("empty\n");
        }
        else
        {
            for (car_t *j = hashTable->buckets[i]; j != NULL; j = j->next)
            {
                car_print(j);
                if (j->next != NULL)
                {
                    //printf(" -> ");
                }
            }
            //printf("\n");
            
        }
    }
    
}

// Delete item from hashtable
// PRE: htabl_find(hashTable, key) != NULL
// POST: htabl_find(hashTable, key) == NULL
void htable_delete(htable_t *hashTable, char *plate)
{

    // Init variables for head of bucket, current and previous for looping through table
    car_t *head = htable_bucket(hashTable, plate);
    // Current begins at head of bucket
    car_t *current = head;
    // Init previous to NULL, will be adjusted through loop
    car_t *previous = NULL;
    // Loop through until reach end of bucket
    
    while (current != NULL)
    {
        // Check to see if plates match
        if (strcmp(current->plate, plate) == 0)
        {
            // If car is first in list
            if (previous == NULL)
            {
                hashTable->buckets[htable_index(hashTable, plate)] = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            // Free allocated memory
            free(current);
            free(plate);
            break;
        }

        // Increment current & previous
        previous = current;
        current = current->next;
    }
}

// htable_destroy
// Pre: htable_init(hashTable)
// POST: No memory allocated for hash table
void htable_destroy(htable_t *hashTable)
{
    // Free the linked lists
    for (size_t i = 0; i < hashTable->size; ++i)
    {
        car_t *bucket = hashTable->buckets[i];
        while (bucket != NULL)
        {
            car_t *next = bucket->next;
            free(bucket);
            bucket = next;
        }
    }

    // free buckets array
    free(hashTable->buckets);
    hashTable->buckets = NULL;
    hashTable->size = 0;
}

// Iterates through each bucket of hashtable 'h', printing out keys with value 'search'
// pre: htab_init(h)
// post: each key with value 'search' has been printed to stdout
void htable_search_value(htable_t *hashTable, char *search)
{
    for (size_t i = 0; i < hashTable->size; ++i)
    {
        for (car_t *bucket = hashTable->buckets[i]; bucket != NULL; bucket = bucket->next)
        {
            
        }
    }
    //printf("\n");
}

#endif