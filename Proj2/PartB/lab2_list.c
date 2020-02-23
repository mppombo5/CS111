/* NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include "SortedList.h"

// random string length
#define LAB2_LIST_RANDSTRLEN 32

// sync type constants
#define LAB2_LIST_NOSYNC 'F'
#define LAB2_LIST_MUTEX 'm'
#define LAB2_LIST_SPINLOCK 's'

// miscellaneous useful things
#ifndef BILLION
# define BILLION 1000000000LL
#endif

#ifndef __unused
# define __unused __attribute__((unused))
#endif

// constant strings for reference
const char* const progName = "lab2_list";
const char* const usage = "lab2_list [--iterations=#] [--threads=#] [--lists=#] [--yield=[idl]";
const char* const randCharset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-?!";

// global variables
char syncType = LAB2_LIST_NOSYNC;
int opt_yield = 0;
int debug = 0;

// various utility functions
void killProg(const char* msg, int exitStat);
void segfaultHandler(int sig __unused);
char* randString();
void* threadFunc(void* as_v);
unsigned long strHash(const char* str);
long long timeElapsed(struct timespec* start, struct timespec* end);

// members: shared list, element array, number of iterations
typedef struct argStruct_t {
    long numIters;
    SortedList_t* listArr;
    long numLists;
    SortedListElement_t* elmtArr;
    pthread_mutex_t* mutexArr;
    volatile short* spinlockArr;
    long threadID;
} argStruct;

int main(int argc, char** argv) {
    long numThreads = 1;
    long numIters = 1;
    long numLists = 1;

    if (signal(SIGSEGV, &segfaultHandler) == SIG_ERR) {
        killProg("Unable to register handler for SIGSEGV", 1);
    }

    struct option options[] = {
            {"threads", required_argument, NULL, 't'},
            {"iterations", required_argument, NULL, 'i'},
            {"yield", required_argument, NULL, 'y'},
            {"sync", required_argument, NULL, 's'},
            {"lists", required_argument, NULL, 'l'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optstring = "t:i:y:s:l:d";
    int ch = getopt_long(argc, argv, optstring, options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 't':
                numThreads = atol(optarg);
                if (numThreads < 0) {
                    killProg("'--threads' argument must be non-negative", 1);
                }
                break;
            case 'i':
                numIters = atol(optarg);
                if (numIters < 0) {
                    killProg("'--iterations' argument must be non-negative", 1);
                }
                break;
            case 'y':
                for (size_t i = 0; optarg[i] != '\0'; i++) {
                    switch (optarg[i]) {
                        case 'i':
                            opt_yield |= INSERT_YIELD;
                            break;
                        case 'd':
                            opt_yield |= DELETE_YIELD;
                            break;
                        case 'l':
                            opt_yield |= LOOKUP_YIELD;
                            break;
                        default:
                            killProg("'--yield' option must be any combination of 'i', 'd', and 'l'", 1);
                    }
                }
                break;
            case 's':
                if (strlen(optarg) != 1) {
                    killProg("'--sync' option must be either 's' or 'm'", 1);
                }
                switch (*optarg) {
                    case 's':
                        syncType = LAB2_LIST_SPINLOCK;
                        break;
                    case 'm':
                        syncType = LAB2_LIST_MUTEX;
                        break;
                    default:
                        killProg("'--sync' option must be either 's' or 'm'", 1);
                        break;
                }
                break;
            case 'l':
                numLists = atol(optarg);
                if (numLists < 1) {
                    killProg("'--lists' argument must be nonzero and positive", 1);
                }
                break;
            case 'd':
                debug = 1;
                break;
            default:
                // invalid argument
                fprintf(stderr, "%s\n", usage);
                exit(1);
        }
        ch = getopt_long(argc, argv, optstring, options, &optIndex);
    }

    // create a bunch of empty lists
    SortedList_t listArr[numLists];
    for (int i = 0; i < numLists; i++) {
        SortedList_t* cur = listArr + i;
        cur->next = cur;
        cur->prev = cur;
    }

    // generate elements with random keys, stick them in an array
    long numElmts = numIters * numThreads;
    SortedListElement_t randElmts[numElmts];
    for (int i = 0; i < numElmts; i++) {
        randElmts[i].key = randString();
    }

    // threads and their return codes
    pthread_t threadArr[numThreads];
    int threadRC;

    // make threads joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // initialize mutexes for each list
    pthread_mutex_t* mutexArr;
    if (syncType == LAB2_LIST_MUTEX) {
        mutexArr = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * numLists);
        if (mutexArr == NULL) {
            killProg("Error in malloc() while allocating mutexes", 1);
        }
        for (int i = 0; i < numLists; i++) {
            pthread_mutex_init(mutexArr + i, NULL);
        }
    }

    // initialize spinlock list
    volatile short* spinlockArr;
    if (syncType == LAB2_LIST_SPINLOCK) {
        spinlockArr = (volatile short*) calloc(numLists, sizeof(volatile short));
        if (spinlockArr == NULL) {
            killProg("Error in calloc() while allocating spinlocks", 1);
        }
    }

    // array of argStructs for each thread
    argStruct structs[numThreads];

    // get start time
    struct timespec startTimespec;
    if (clock_gettime(CLOCK_MONOTONIC, &startTimespec) == -1) {
        killProg("Error in clock_gettime() before iterations", 1);
    }

    // start threads
    for (long i = 0; i < numThreads; i++) {
        argStruct* as = structs + i;
        as->numIters = numIters;
        as->listArr = listArr;
        as->numLists = numLists;
        as->elmtArr = randElmts + (i * numIters);
        as->mutexArr = mutexArr;
        as->spinlockArr = spinlockArr;
        as->threadID = i;

        threadRC = pthread_create(threadArr + i, &attr, &threadFunc, (void*) as);
        if (threadRC != 0) {
            errno = threadRC;
            killProg("Error creating thread", 1);
        }
        if (debug) {
            fprintf(stderr, "thread %ld created\n", i);
        }
    }

    // variable to sum up each thread mean wait-for-lock time
    long long avgLockTime = 0;

    // join them all back up
    pthread_attr_destroy(&attr);
    for (long i = 0; i < numThreads; i++) {
        void* tmp;
        threadRC = pthread_join(threadArr[i], &tmp);
        if (threadRC != 0) {
            errno = threadRC;
            killProg("Error joining threads", 1);
        }
        avgLockTime += (long long) tmp;
        if (debug) {
            fprintf(stderr, "thread %ld joined\n", i);
        }
    }

    // calculate average wait-for-lock time by dividing avgLockTime by the number of threads
    // but make it 0 if there was no synchronization
    avgLockTime = (syncType == LAB2_LIST_NOSYNC) ? 0 : (avgLockTime / numThreads);

    // get end time, and hence total running time
    struct timespec endTimespec;
    if (clock_gettime(CLOCK_MONOTONIC, &endTimespec) == -1) {
        killProg("Error in clock_gettime() after iterations", 1);
    }
    long long totalTime = timeElapsed(&startTimespec, &endTimespec);

    // destroy mutexes
    if (syncType == LAB2_LIST_MUTEX) {
        for (int i = 0; i < numLists; i++) {
            pthread_mutex_destroy(mutexArr + i);
        }
        free((void*) mutexArr);
    }

    // free spinlock array
    if (syncType == LAB2_LIST_SPINLOCK) {
        free((void*) spinlockArr);
    }

    for (int i = 0; i < numLists; i++) {
        if (SortedList_length(listArr + i) != 0) {
            killProg("Corrupted list; length of at least one list is not 0 after thread operations", 2);
        }
    }

    // build the CSV string
    char yieldopts[5];
    char* _ins = (opt_yield & INSERT_YIELD) ? "i" : "";
    char* _del = (opt_yield & DELETE_YIELD) ? "d" : "";
    char* _look = (opt_yield & LOOKUP_YIELD) ? "l" : "";
    if (sprintf(yieldopts, "%s%s%s", _ins, _del, _look) < 0) {
        killProg("Error in call to sprintf()", 1);
    }
    if (strlen(yieldopts) == 0) {
        strcpy(yieldopts, "none");
    }

    char* syncopts = "none";
    switch (syncType) {
        case LAB2_LIST_MUTEX:
            syncopts = "m";
            break;
        case LAB2_LIST_SPINLOCK:
            syncopts = "s";
            break;
        default: break;
    }

    long ops = numThreads * numIters * 3;
    long long timePerOp = totalTime / ops;

    // print csv line
    // name,threads,iters,lists(1),ops,runtime,avg
    printf("list-%s-%s,%ld,%ld,%ld,%ld,%lld,%lld,%lld\n", yieldopts, syncopts, numThreads, numIters, numLists, ops, totalTime, timePerOp, avgLockTime);

    pthread_exit(NULL);
}

/*
 * listArr, mutexArr, and spinlockArr all have 'numLists' number of elements.
 * Each one at position j correspond to the jth position; therefore, in this function,
 * setting a pointer equal to mutexArr + j or spinlockArr + j correspond to the lock
 * needed to operate on that list.
 */
void* threadFunc(void* as_v) {
    // a TON of local variables to be declared, just so we're not constantly reading from memory
    argStruct* as = (argStruct*) as_v;
    long numIters = as->numIters;
    SortedList_t* listArr = as->listArr;
    long numLists = as->numLists;
    SortedListElement_t* elmtArr = as->elmtArr;
    pthread_mutex_t* mutexArr = as->mutexArr;
    volatile short* spinlockArr = as->spinlockArr;
    long id = as->threadID;

    // local mutex and spinlock pointers, as well as the current list to operate on
    pthread_mutex_t* mutex;
    volatile short* spinlock;
    SortedList_t* list;

    // total (average) time spend on locks (in nanoseconds)
    long long avgLockTime = 0;
    struct timespec startTime;
    struct timespec endTime;

    if (debug) {
        fprintf(stderr, "thread %ld starting insertions\n", id);
    }
    long long insertLockTime = 0;
    for (long i = 0; i < numIters; i++) {
        if (debug) {
            fprintf(stderr, "thread %ld inserting item %ld\n", id, i);
        }

        // hash the key and get the corresponding list to operate on
        unsigned long listPos = strHash(elmtArr[i].key) % numLists;
        list = listArr + listPos;

        // assign locks and lock
        if (clock_gettime(CLOCK_MONOTONIC, &startTime) == -1) {
            killProg("Unable to get time before obtaining lock during insertion", 1);
        }
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                mutex = mutexArr + listPos;
                pthread_mutex_lock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                spinlock = spinlockArr + listPos;
                while (__sync_lock_test_and_set(spinlock, 1));
                break;
            default: break;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &endTime) == -1) {
            killProg("Unable to get time after obtaining lock during insertion", 1);
        }
        insertLockTime += timeElapsed(&startTime, &endTime);

        SortedList_insert(list, elmtArr + i);

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(spinlock);
                break;
            default: break;
        }

        if (debug) {
            fprintf(stderr, "thread %ld inserted item %ld\n", id, i);
        }
    }
    if (debug){
        fprintf(stderr, "thread %ld finished insertions\n", id);
    }
    // average wait-for-lock for insertions
    avgLockTime += insertLockTime / numIters;

    // enumerate the total list, locking each sub-list as you go
    long long enumLockTime = 0;
    for (int listPos = 0; listPos < numLists; listPos++) {
        list = listArr + listPos;

        // assign and lock
        if (clock_gettime(CLOCK_MONOTONIC, &startTime) == -1) {
            killProg("Unable to get time before obtaining lock during insertion", 1);
        }
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                mutex = mutexArr + listPos;
                pthread_mutex_lock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                spinlock = spinlockArr + listPos;
                while (__sync_lock_test_and_set(spinlock, 1));
                break;
            default: break;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &endTime) == -1) {
            killProg("Unable to get time after obtaining lock during insertion", 1);
        }
        enumLockTime += timeElapsed(&startTime, &endTime);

        // actual enumeration
        int listSize = SortedList_length(list);
        if (listSize == -1) {
            killProg("Corrupted list found in call to SortedList_length()", 2);
        }

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(spinlock);
                break;
            default: break;
        }
    }
    // average wait-for-lock for enumerations
    avgLockTime += enumLockTime / numLists;

    // lookup and deletion for each inserted element
    long long lookdelLockTime = 0;
    for (long i = 0; i < numIters; i++) {
        if (debug) {
            fprintf(stderr, "thread %ld starting lookup for %ldth item inserted\n", id, i);
        }

        SortedListElement_t* curElmt = elmtArr + i;
        unsigned long listPos = strHash(curElmt->key) % numLists;
        list = listArr + listPos;

        // assign/lock
        if (clock_gettime(CLOCK_MONOTONIC, &startTime) == -1) {
            killProg("Unable to get time before obtaining lock during insertion", 1);
        }
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                mutex = mutexArr + listPos;
                pthread_mutex_lock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                spinlock = spinlockArr + listPos;
                while (__sync_lock_test_and_set(spinlock, 1));
                break;
            default: break;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &endTime) == -1) {
            killProg("Unable to get time after obtaining lock during insertion", 1);
        }
        lookdelLockTime += timeElapsed(&startTime, &endTime);

        SortedListElement_t* elmt = SortedList_lookup(list, curElmt->key);
        if (elmt == NULL) {
            killProg("Corrupted list; known element not found in call to SortedList_lookup()", 2);
        }

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(spinlock);
                break;
            default: break;
        }

        if (debug) {
            fprintf(stderr, "thread %ld found %ldth item\n", id, i);
        }

        // already assigned, just lock
        if (clock_gettime(CLOCK_MONOTONIC, &startTime) == -1) {
            killProg("Unable to get time before obtaining lock during insertion", 1);
        }
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_lock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                while (__sync_lock_test_and_set(spinlock, 1));
                break;
            default: break;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &endTime) == -1) {
            killProg("Unable to get time after obtaining lock during insertion", 1);
        }
        lookdelLockTime += timeElapsed(&startTime, &endTime);

        int del = SortedList_delete(elmt);
        if (del == 1) {
            killProg("Corrupted list found in call to SortedList_delete()", 2);
        }

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(mutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(spinlock);
                break;
            default: break;
        }
    }
    avgLockTime += lookdelLockTime / (2 * numIters);

    /*
     * calculate and return average lock time, using avgLockTime.
     * three different means were added, so divide it by three
     * to get the overall mean.
     */
    avgLockTime /= 3;
    pthread_exit((void*) avgLockTime);
}

// As stated in Lab 2A, shamelessly ripped from StackOverflow.
char* randString() {
    char* str = (char*) malloc(sizeof(char) * (LAB2_LIST_RANDSTRLEN + 1));
    if (str == NULL) {
        killProg("Unable to create random key; error in malloc()", 1);
    }
    for (int i = 0; i < LAB2_LIST_RANDSTRLEN; i++) {
        int setKey = rand() % strlen(randCharset);
        str[i] = randCharset[setKey];
    }
    str[LAB2_LIST_RANDSTRLEN] = '\0';
    return str;
}

// based on the "djb2" hashing algorithm.
unsigned long strHash(const char* str) {
    unsigned long hash = 5381;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

// return, in nanoseconds, the total time elapsed between the time at 'end' and the time at 'start'
long long timeElapsed(struct timespec* start, struct timespec* end) {
    long long seconds = end->tv_sec - start->tv_sec;
    long long rawNanoseconds = end->tv_nsec - start->tv_nsec;
    long long nanoseconds = (rawNanoseconds < 0) ? (BILLION + rawNanoseconds) : rawNanoseconds;
    return (seconds * BILLION) + nanoseconds;
}

void killProg(const char* msg, int exitStat) {
    if (errno == 0) {
        fprintf(stderr, "%s: %s\n", progName, msg);
    }
    else {
        fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
    }
    exit(exitStat);
}

void segfaultHandler(int sig __unused) {
    killProg("Segfault detected; exiting with error code 2", 2);
}
