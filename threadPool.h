/*
 * Avi Miletzky 203043401
 */

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <malloc.h>
#include <sys/param.h>
#include "osqueue.h"

#define ERROR -1
#define SUCCESS 0
#define S_C_ERROR "Error in system call"

typedef struct task {
    void (*funcPtr)(void *arg);
    void *params;
} Task;

typedef struct thread_pool {
    int threadsNum;
    OSQueue *tasksQue;
    pthread_t *threads;
    pthread_mutex_t queLocker;
    pthread_mutex_t destroyLocker;
    pthread_cond_t cond;
    bool isDestroyed;
    bool shouldWaitForTasks;

} ThreadPool;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool,
                 void (*computeFunc)(void *), void *param);

#endif
