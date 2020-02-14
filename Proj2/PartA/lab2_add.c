/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define LAB2_ADD_NOSYNC 'F'
#define LAB2_ADD_MUTEX 'm'
#define LAB2_ADD_SPIN_LOCK 's'
#define LAB2_ADD_TESTSET 'c'

#ifndef bool
# define bool int
#endif
#ifndef true
# define true 1
#endif
#ifndef false
# define false 0
#endif

void killProg(const char* msg, int exitStat);
const char* progName = "lab2_add";
const char* usage = "Usage: lab2_add [--threads=NUM] [--iterations=NUM]";

typedef struct argStruct_t {
    long long* pointer;
    long iters;
} argStruct;

int opt_yield = 0;
char syncType = LAB2_ADD_NOSYNC;
void add(long long* pointer, long long value);
void* threadAdd(void* as_v);

int main(int argc, char** argv) {
    long numThreads = 1;
    long iterations = 1;
    bool debug = false;

    struct option options[] = {
            {"threads", required_argument, NULL, 't'},
            {"iterations", required_argument, NULL, 'i'},
            {"yield", no_argument, NULL, 'y'},
            {"sync", required_argument, NULL, 's'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optstring = "t:i:ys:d";
    int ch = getopt_long(argc, argv, optstring, options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 't':
                numThreads = atol(optarg);
                if (numThreads < 0) {
                    fprintf(stderr, "%s: '--threads' argument must be non-negative\n", progName);
                    exit(1);
                }
                break;
            case 'i':
                iterations = atol(optarg);
                if (iterations < 0) {
                    fprintf(stderr, "%s: '--iterations' argument must be non-negative\n", progName);
                    exit(1);
                }
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                if (strlen(optarg) != 1 || !(*optarg == 'm' || *optarg == 's' || *optarg == 'c')) {
                    fprintf(stderr, "%s: '--sync=' argument must be either 'm', 's', or 'c'\n", progName);
                    exit(1);
                }
                syncType = *optarg;
                break;
            case 'd':
                debug = true;
                break;
            default:
                // no valid arguments were found
                exit(1);
        }
        ch = getopt_long(argc, argv, optstring, options, &optIndex);
    }
    if (debug && syncType != LAB2_ADD_NOSYNC) {
        switch (syncType) {
            case LAB2_ADD_MUTEX:
                fprintf(stderr, "'--sync=m' detected\n");
                break;
            case LAB2_ADD_TESTSET:
                fprintf(stderr, "'--sync=c' detected\n");
                break;
            case LAB2_ADD_SPIN_LOCK:
                fprintf(stderr, "'--sync=s' detected\n");
                break;
        }
    }

    long long counter = 0;
    struct timespec timeRunning;
    if (clock_gettime(CLOCK_MONOTONIC, &timeRunning) == -1) {
        killProg("Error in clock_gettime() before iterations", 1);
    }
    long startTime = timeRunning.tv_nsec;

    pthread_t threads[numThreads];
    // make threads joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int threadRC;

    for (long i = 0; i < numThreads; i++) {
        argStruct as;
        as.pointer = &counter;
        as.iters = iterations;
        threadRC = pthread_create(threads + i, &attr, (void* (*)(void*))threadAdd, (void*) &as);
        if (threadRC != 0) {
            errno = threadRC;
            killProg("Error creating thread", 1);
        }
    }

    pthread_attr_destroy(&attr);
    // join all threads so we know they're done
    for (long i = 0; i < numThreads; i++) {
        threadRC = pthread_join(threads[i], NULL);
        if (threadRC != 0) {
            errno = threadRC;
            killProg("Error joining pthreads", 1);
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &timeRunning) == -1) {
        killProg("Error in clock_gettime() after iterations", 1);
    }
    long endTime = timeRunning.tv_nsec;
    long totalTime = endTime - startTime;

    char* testName;
    if (opt_yield) {
        switch (syncType) {
            case LAB2_ADD_NOSYNC:
                testName = "add-yield-none";
                break;
            case LAB2_ADD_MUTEX:
                testName = "add-yield-m";
                break;
            case LAB2_ADD_SPIN_LOCK:
                testName = "add-yield-s";
                break;
            case LAB2_ADD_TESTSET:
                testName = "add-yield-c";
                break;
        }
    }
    else {
        switch (syncType) {
            case LAB2_ADD_NOSYNC:
                testName = "add-none";
                break;
            case LAB2_ADD_MUTEX:
                testName = "add-m";
                break;
            case LAB2_ADD_SPIN_LOCK:
                testName = "add-s";
                break;
            case LAB2_ADD_TESTSET:
                testName = "add-c";
                break;
        }
    }
    long ops = 2 * numThreads * iterations;
    long timePerOp = totalTime / ops;

    printf("%s,%ld,%ld,%ld,%ld,%ld,%lld\n", testName, numThreads, iterations, ops, totalTime, timePerOp, counter);

    pthread_exit(NULL);
}

void add(long long* pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }
    *pointer = sum;
}

void* threadAdd(void* as_v) {
    argStruct* as = (argStruct*) as_v;
    long iters = as->iters;
    long long* pointer = as->pointer;
    for (long i = 0; i < iters; i++) {
        add(pointer, 1);
    }
    for (long i = 0; i < iters; i++) {
        add(pointer, -1);
    }
    pthread_exit(NULL);
}

void killProg(const char* msg, int exitStat) {
    fprintf(stderr, "%s: %s: %s", progName, msg, strerror(errno));
    exit(exitStat);
}
