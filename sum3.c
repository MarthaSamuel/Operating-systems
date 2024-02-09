#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXTHREADS 10

pthread_mutex_t totalMutex = PTHREAD_MUTEX_INITIALIZER;
volatile unsigned long long Round = 0ULL;
volatile unsigned long long Total = 0ULL;

void *Summation(void *tidptr)
{
    // int tid = *((int *)tidptr); // tid is not used in this function

    unsigned long long localTotal = 0ULL;

    for (unsigned long long dex = 0ULL; dex <= Round; dex++)
    {
        localTotal += dex;
    }

    // Acquire the mutex before updating the shared Total variable
    pthread_mutex_lock(&totalMutex);
    Total += localTotal;
    // Release the mutex after updating the shared Total variable
    pthread_mutex_unlock(&totalMutex);

    return NULL;
}

void doOneRound(unsigned long long thisRound, int numthreads)
{
    int tids[MAXTHREADS];
    pthread_t t[MAXTHREADS];
    int rc;

    /* start all of the threads */
    for (int i = 0; i < numthreads; i++)
    {
        tids[i] = i;
        rc = pthread_create(&t[i], NULL, Summation, (void *)&tids[i]);
        if (rc)
        {
            fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    /* wait for threads to complete */
    for (int i = 0; i < numthreads; i++)
    {
        rc = pthread_join(t[i], NULL);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR joining with thread %d (error==%d)\n", tids[i], rc);
            exit(-1);
        }
    }
}

void checkResult(unsigned long long thisRound, int numthreads)
{
    /*
     * note: credit for this closed-form solution goes to Johann Carl Friedrich Gauss.
     * it will not work for very large values of thisRound
     * because the multiplication will cause it to overflow. but it is good enough
     * for our purposes.
     */
    unsigned long long calc = (thisRound * (thisRound + 1ULL)) / 2ULL;
    calc *= (unsigned long long)numthreads;

    if (Total != calc)
    {
        printf("PARENT: ERROR! Round %llu total should have been %llu but was %llu\n", thisRound, calc, Total);
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "USAGE: %s <nthreads> <max>\n", argv[0]);
        exit(-1);
    }

    int numthreads = atoi(argv[1]);
    if ((numthreads < 1) || (numthreads > MAXTHREADS))
    {
        fprintf(stderr, "ERROR: numthreads must be >= 1 and <= %d\n", MAXTHREADS);
        exit(-1);
    }

    unsigned long long numrounds = strtoull(argv[2], NULL, 0);
    if (numrounds <= 0ULL)
    {
        fprintf(stderr, "ERROR: number of rounds must be a positive unsigned long long (not '%s')\n", argv[2]);
        exit(-1);
    }

    printf("PARENT: input %d threads %llu rounds\n", numthreads, numrounds);

    for (Round = 1ULL; Round < numrounds; Round++)
    {
        Total = 0ULL;

        doOneRound(Round, numthreads);
        checkResult(Round, numthreads);
    }

    printf("PARENT: SUCCESS! exiting after final Round %llu (Total: %llu)\n", numrounds - 1, Total);

    return (0);
}
