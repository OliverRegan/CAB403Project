
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t htable_mutex;

// Hashtable for parked cars
typedef struct car car_t;
struct car
{
    // Plate details - plate used as key
    char *plate;

    time_t *entry_time;

    // Next parked car in hashtable
    car_t *next;
};

// Define the hashtable itself
typedef struct htable htable_t;
struct htable
{
    // Arrays for cars
    car_t **buckets;
    // Size of hashtable
    size_t size;
};

// Print car for testing
void car_print(car_t *car)
{
    printf("plate=%s, Entry time:%ln", car->plate, car->entry_time);
};

// Initialize the hashtable
bool htable_init(htable_t *hashTable, size_t n)
{
    pthread_mutex_lock(&htable_mutex);
    // Allocate size of hashtable - how many buckets
    hashTable->size = n;
    // Reset buckets to be 0's
    hashTable->buckets = (car_t **)calloc(n, sizeof(car_t) * n);
    // Test if calloc has reset buckets and return
    pthread_mutex_unlock(&htable_mutex);
    return hashTable->buckets != 0;
};

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
};

// Find hash offset for new entry in hashtable
size_t htable_index(htable_t *hashTable, char *plate)
{
    // Return hashed number / buckets remainder
    return djb_hash(plate) % hashTable->size;
};

// Get the bucket of car with plate
car_t *htable_bucket(htable_t *hashTable, char *plate)
{
    // Get pointer to correct bucket that has car in it
    return hashTable->buckets[htable_index(hashTable, plate)];
};

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
};

// Add to hashtable
// PRE: htable_find(hashTable, key) == NULL
// POST: htable_find(hashTable, key) != NULL OR return false
bool htable_add(htable_t *hashTable, char *plate)
{
    pthread_mutex_lock(&htable_mutex);
    // Create new head for hashtable bucket
    car_t *newHead = (car_t *)malloc(sizeof(car_t));

    // Check if newHead has been allocated
    if (newHead == NULL)
    {
        return false;
    }

    // printf("%s", plate);

    // Get bucket
    size_t bucket = htable_index(hashTable, plate);
    // Shuffle current head along
    newHead->next = hashTable->buckets[bucket];
    // Assign value to newHead
    newHead->plate = plate;
    // Assign time value
    // newHead->entry_time = localtime()
    // Assign new car to bucket
    hashTable->buckets[bucket] = newHead;
    pthread_mutex_unlock(&htable_mutex);
    return true;
};

// Print the hash table.
// PRE: true
// POST: hash table is printed to screen
void htable_print(htable_t *hashTable)
{
    printf("hash table with %d buckets\n", (int)hashTable->size);
    for (size_t i = 0; i < hashTable->size; ++i)
    {
        printf("bucket %d: ", (int)i);
        if (hashTable->buckets[i] == NULL)
        {
            printf("empty\n");
        }
        else
        {
            for (car_t *j = hashTable->buckets[i]; j != NULL; j = j->next)
            {
                car_print(j);
                if (j->next != NULL)
                {
                    printf(" -> ");
                }
            }
            printf("\n");
        }
    }
}

// Delete item from hashtable
// PRE: htabl_find(hashTable, key) != NULL
// POST: htabl_find(hashTable, key) == NULL
void htable_delete(htable_t *hashTable, char *plateInput)
{
    char *plate = plateInput;

    pthread_mutex_lock(&htable_mutex);
    // Init variables for head of bucket, current and previous for looping through table
    car_t *head = htable_bucket(hashTable, plate);
    // Current begins at head of bucket
    car_t *current = head;
    // Init previous to NULL, will be adjusted through loop
    car_t *previous = NULL;

    // Loop through until reach end of bucket
    while (current != NULL)
    {
        printf("\n\n%s\n%s\n\n", current->plate, plate);

        // Check to see if plates match
        if (strcmp(current->plate, plate))
        {
            // printf("\n\n%s\n", plate);

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

            break;
        }

        // Increment current & previous
        previous = current;
        current = current->next;
    }
    pthread_mutex_unlock(&htable_mutex);
};

// htable_destroy
// Pre: htable_init(hashTable)
// POST: No memory allocated for hash table
void htable_destory(htable_t *hashTable)
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

// // Iterates through each bucket of hashtable 'h', printing out keys with value 'search'
// // pre: htab_init(h)
// // post: each key with value 'search' has been printed to stdout
// void htable_search_value(htable_t *hashTable, char *search)
// {
//     for (size_t i = 0; i < hashTable->size; ++i)
//     {
//         for (car_t *bucket = hashTable->buckets[i]; bucket != NULL; bucket = bucket->next)
//         {
//             if (bucket->plate == search){

//             }
//         }
//     }
// }
