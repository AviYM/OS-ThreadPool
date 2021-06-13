/*
 * Avi Miletzky
 */

#include "threadPool.h"

void handlesErrors(char *error) {
    write(STDERR_FILENO, error, strlen(error));
    exit(ERROR);
}

void *run(void *arg) {
    ThreadPool *threadPool = (ThreadPool *) arg;

    while (threadPool->shouldWaitForTasks) {
        Task *task = NULL;

        if (pthread_mutex_lock(&threadPool->destroyLocker)) {
            handlesErrors(S_C_ERROR);
        }

        if (osIsQueueEmpty(threadPool->tasksQue) && threadPool->isDestroyed) {
            if (pthread_mutex_unlock(&threadPool->destroyLocker)) {
                handlesErrors(S_C_ERROR);
            }
            break;
        }

        if (pthread_mutex_unlock(&threadPool->destroyLocker)) {
            handlesErrors(S_C_ERROR);
        }

        if (pthread_mutex_lock(&threadPool->queLocker)) {
            handlesErrors(S_C_ERROR);
        }

        if (osIsQueueEmpty(threadPool->tasksQue)) {

            if (pthread_cond_wait(&threadPool->cond, &threadPool->queLocker)) {
                handlesErrors(S_C_ERROR);
            }
        }

        if (threadPool->shouldWaitForTasks) {
            task = (Task *) osDequeue(threadPool->tasksQue);
        }

        if (pthread_mutex_unlock(&threadPool->queLocker)) {
            handlesErrors(S_C_ERROR);
        }

        if (task != NULL) {
            task->funcPtr(task->params);
            free(task);
        }
    }
}

void freeThreadPool(ThreadPool *threadPool) {
    pthread_cond_destroy(&threadPool->cond);
    pthread_mutex_destroy(&threadPool->queLocker);
    while (!osIsQueueEmpty(threadPool->tasksQue)) {
        free(osDequeue(threadPool->tasksQue));
    }
    osDestroyQueue(threadPool->tasksQue);
    free(threadPool->threads);
    free(threadPool);
}

ThreadPool *tpCreate(int numOfThreads) {

    int i;
    ThreadPool *threadPool;

    if ((threadPool = (ThreadPool *) malloc(sizeof(ThreadPool))) == NULL) {
        handlesErrors(S_C_ERROR);
    }
    if ((threadPool->threads = (pthread_t *) malloc(
            numOfThreads * sizeof(pthread_t))) == NULL) {
        handlesErrors(S_C_ERROR);
    }

    threadPool->threadsNum = numOfThreads;
    threadPool->tasksQue = osCreateQueue();
    threadPool->isDestroyed = false;
    threadPool->shouldWaitForTasks = true;

    if (pthread_mutex_init(&threadPool->queLocker, NULL)
        || pthread_mutex_init(&threadPool->destroyLocker, NULL)
        || pthread_cond_init(&threadPool->cond, NULL)) {
        handlesErrors(S_C_ERROR);
    }

    for (i = 0; i < numOfThreads; i++) {
        pthread_t thId;
        if (pthread_create(&thId, NULL, run, threadPool)) {
            handlesErrors(S_C_ERROR);
        }
        threadPool->threads[i] = thId;
    }

    return threadPool;
}

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    int i;

    if (threadPool == NULL) {
        return;
    }

    if (pthread_mutex_lock(&threadPool->destroyLocker)) {
        handlesErrors(S_C_ERROR);
    }

    if (threadPool->isDestroyed) {
        if (pthread_mutex_unlock(&threadPool->destroyLocker)) {
            handlesErrors(S_C_ERROR);
        }
        return;
    }

    threadPool->isDestroyed = true;
    if(!shouldWaitForTasks) {
        threadPool->shouldWaitForTasks = false;
    }

    if (pthread_mutex_unlock(&threadPool->destroyLocker)) {
        handlesErrors(S_C_ERROR);
    }

    for (i = 0; i < threadPool->threadsNum; ++i) {
        pthread_join(threadPool->threads[i], NULL);
    }

    freeThreadPool(threadPool);
}

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *),
                 void *param) {
    Task *task;

    if (threadPool == NULL || computeFunc == NULL) {
        return ERROR;
    }

    if (pthread_mutex_lock(&threadPool->destroyLocker)) {
        handlesErrors(S_C_ERROR);
    }

    if (threadPool->isDestroyed) return ERROR;

    if (pthread_mutex_unlock(&threadPool->destroyLocker)) {
        handlesErrors(S_C_ERROR);
    }

    if ((task = (Task *) malloc(sizeof(Task))) == NULL) {
        handlesErrors(S_C_ERROR);
    }
    task->funcPtr = computeFunc;
    task->params = param;

    if (pthread_mutex_lock(&threadPool->queLocker)) {
        handlesErrors(S_C_ERROR);
    }

    osEnqueue(threadPool->tasksQue, task);
    if(osIsQueueEmpty(threadPool->tasksQue)) {
        if (pthread_cond_signal(&(threadPool->cond))) {
            handlesErrors(S_C_ERROR);
        }
    }

    if (pthread_mutex_unlock(&threadPool->queLocker)) {
        handlesErrors(S_C_ERROR);
    }
    return SUCCESS;
}