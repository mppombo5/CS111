/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
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
#ifndef __unused
# define __unused __attribute__((unused))
#endif

const char* progName = "lab2_list";
const char* usage = "lab2_list [--iterations=#] [--threads=#] [--yield=[idl]";
int debug = 0;

int opt_yield = 0;

char syncType = LAB2_LIST_NOSYNC;
pthread_mutex_t listMutex;
volatile short spinLock;

void killProg(const char* msg, int exitStat);
void segfaultHandler(int sig);
char* randString();

void* threadFunc(void* as_v);

// members: shared list, element array, number of iterations
typedef struct argStruct_t {
    SortedList_t* list;
    SortedListElement_t* elmtArr;
    long iters;
    long threadID;
} argStruct;

int main(int argc, char** argv) {
    long numThreads = 1;
    long iters = 1;

    if (signal(SIGSEGV, &segfaultHandler) == SIG_ERR) {
        killProg("Unable to register handler for SIGSEGV", 1);
    }

    struct option options[] = {
            {"threads", required_argument, NULL, 't'},
            {"iterations", required_argument, NULL, 'i'},
            {"yield", required_argument, NULL, 'y'},
            {"sync", required_argument, NULL, 's'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optstring = "t:i:y:s:d";
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
                iters = atol(optarg);
                if (iters < 0) {
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

    // initialize empty list
    SortedList_t list_t;
    SortedList_t* list = &list_t;
    list->next = list;
    list->prev = list;

    // generate random keys, stick them in an array
    long numElmts = iters * numThreads;
    SortedListElement_t randElmts[numElmts];
    for (int i = 0; i < numElmts; i++) {
        randElmts[i].key = randString();
    }

    // get start time
    struct timespec timeRunning;
    if (clock_gettime(CLOCK_MONOTONIC, &timeRunning) == -1) {
        killProg("Error in clock_gettime() before iterations", 1);
    }
    long startTime = timeRunning.tv_nsec;

    // threads and their return codes
    pthread_t threads[numThreads];
    int threadRC;

    // make threads joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // initialize mutex locks
    if (syncType == LAB2_LIST_MUTEX) {
        pthread_mutex_init(&listMutex, NULL);
    }

    // array of argStructs for each thread
    argStruct structs[numThreads];

    // start threads
    for (long i = 0; i < numThreads; i++) {
        argStruct* as = structs + i;
        as->list = list;
        as->iters = iters;
        as->elmtArr = randElmts + (i * iters);
        as->threadID = i;

        threadRC = pthread_create(threads + i, &attr, &threadFunc, (void*) as);
        if (threadRC != 0) {
            errno = threadRC;
            killProg("Error creating thread", 1);
        }
        if (debug) {
            fprintf(stderr, "thread %ld created\n", i);
        }
    }

    // join them all back up
    pthread_attr_destroy(&attr);
    for (long i = 0; i < numThreads; i++) {
        threadRC = pthread_join(threads[i], NULL);
        if (threadRC != 0) {
            errno = threadRC;
            killProg("Error joining threads", 1);
        }
        if (debug) {
            fprintf(stderr, "thread %ld joined\n", i);
        }
    }

    // destroy locks
    if (syncType == LAB2_LIST_MUTEX) {
        pthread_mutex_destroy(&listMutex);
    }

    // get end time, and hence total running time
    if (clock_gettime(CLOCK_MONOTONIC, &timeRunning) == -1) {
        killProg("Error in clock_gettime() after iterations", 1);
    }
    long endTime = timeRunning.tv_nsec;
    long totalTime = endTime - startTime;

    if (SortedList_length(list) != 0) {
        killProg("Corrupted list; length is not 0 after thread operations", 2);
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

    int numLists = 1;

    long ops = numThreads * iters * 3;
    long timePerOp = totalTime / ops;

    // print csv line
    // name,threads,iters,lists(1),ops,runtime,avg
    printf("list-%s-%s,%ld,%ld,%d,%ld,%ld,%ld\n", yieldopts, syncopts, numThreads, iters, numLists, ops, totalTime, timePerOp);

    pthread_exit(NULL);
}

void* threadFunc(void* as_v) {
    argStruct* as = (argStruct*) as_v;
    SortedList_t* list = as->list;
    long iters = as->iters;
    SortedListElement_t* elmtArr = as->elmtArr;
    long id = as->threadID;

    if (debug) {
        fprintf(stderr, "thread %ld starting insertions\n", id);
    }
    for (long i = 0; i < iters; i++) {
        if (debug) {
            fprintf(stderr, "thread %ld inserting item %ld\n", id, i);
        }

        // lock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_lock(&listMutex);
                break;
            case LAB2_LIST_SPINLOCK:
                while (__sync_lock_test_and_set(&spinLock, 1)) while(spinLock);
                break;
            default: break;
        }

        SortedList_insert(list, elmtArr + i);

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(&listMutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(&spinLock);
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

    // lock
    switch (syncType) {
        case LAB2_LIST_MUTEX:
            pthread_mutex_lock(&listMutex);
            break;
        case LAB2_LIST_SPINLOCK:
            while (__sync_lock_test_and_set(&spinLock, 1)) while(spinLock);
            break;
        default: break;
    }

    int listSize = SortedList_length(list);
    if (listSize == -1) {
        killProg("Corrupted list found in call to SortedList_length()", 2);
    }

    // unlock
    switch (syncType) {
        case LAB2_LIST_MUTEX:
            pthread_mutex_unlock(&listMutex);
            break;
        case LAB2_LIST_SPINLOCK:
            __sync_lock_release(&spinLock);
            break;
        default: break;
    }

    for (long i = 0; i < iters; i++) {
        if (debug) {
            fprintf(stderr, "thread %ld starting lookup for %ldth item inserted\n", id, i);
        }

        // lock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_lock(&listMutex);
                break;
            case LAB2_LIST_SPINLOCK:
                while (__sync_lock_test_and_set(&spinLock, 1)) while(spinLock);
                break;
            default: break;
        }

        SortedListElement_t* elmt = SortedList_lookup(list, elmtArr[i].key);
        if (elmt == NULL) {
            killProg("Corrupted list; known element not found in call to SortedList_lookup()", 2);
        }

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(&listMutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(&spinLock);
                break;
            default: break;
        }

        if (debug) {
            fprintf(stderr, "thread %ld found %ldth item\n", id, i);
        }

        // lock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_lock(&listMutex);
                break;
            case LAB2_LIST_SPINLOCK:
                while (__sync_lock_test_and_set(&spinLock, 1)) while(spinLock);
                break;
            default: break;
        }

        int del = SortedList_delete(elmt);
        if (del == 1) {
            killProg("Corrupted list found in call to SortedList_delete()", 2);
        }

        // unlock
        switch (syncType) {
            case LAB2_LIST_MUTEX:
                pthread_mutex_unlock(&listMutex);
                break;
            case LAB2_LIST_SPINLOCK:
                __sync_lock_release(&spinLock);
                break;
            default: break;
        }
    }
    pthread_exit(NULL);
}

// shamelessly ripped from Stack Overflow, since I figure the point of this lab
// isn't to judge if we know how to generate random strings.
// generate a random string of length LAB2_LIST_RANDSTRLEN
char* randString() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-?!";
    char* str = malloc(sizeof(char) * (LAB2_LIST_RANDSTRLEN + 1));
    if (str == NULL) {
        killProg("Unable to create random key; error in malloc()", 1);
    }

    for (int i = 0; i < LAB2_LIST_RANDSTRLEN; i++) {
        int setKey = rand() % (int) (sizeof(charset) - 1);
        str[i] = charset[setKey];
    }
    str[LAB2_LIST_RANDSTRLEN] = '\0';
    return str;
}

void killProg(const char* msg, int exitStat) {
    int err = errno;
    if (err != 0) {
        fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
    }
    else {
        fprintf(stderr, "%s: %s\n", progName, msg);
    }
    exit(exitStat);
}

void segfaultHandler(int sig __unused) {
    killProg("Segfault detected; exiting with error code 2", 2);
}
