#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define PING 0
#define PONG 1

volatile unsigned NumRounds = 0;
volatile unsigned PrevVal = PONG;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char *Message[2] = {"PING", "PONG"};

void pingpongprint(int thisval)
{
    pthread_mutex_lock(&mutex);
    while (PrevVal == thisval)
    {
        pthread_cond_wait(&cond, &mutex);
    }

    printf("%s\n", Message[thisval]);
    PrevVal = thisval;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void *PingerPonger(void *tidptr)
{
    int tid = *((int *)tidptr);

    for (unsigned dex = 0; dex <= NumRounds; dex++)
    {
        pingpongprint(tid);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: %s <numrounds>\n", argv[0]);
        exit(-1);
    }

    NumRounds = atoi(argv[1]);
    if (NumRounds < 1)
    {
        fprintf(stderr, "ERROR: NumRounds must be >= 1\n");
        exit(-1);
    }

    int ping_tid = PING;
    int pong_tid = PONG;

    pthread_t pingthread;
    pthread_t pongthread;
    int rc;

    rc = pthread_create(&pingthread, NULL, PingerPonger, (void *)&ping_tid);
    if (rc)
    {
        fprintf(stderr, "ERROR; could not create PING thread. return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    rc = pthread_create(&pongthread, NULL, PingerPonger, (void *)&pong_tid);
    if (rc)
    {
        fprintf(stderr, "ERROR; could not create PONG thread. return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    rc = pthread_join(pingthread, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR joining with PING (rc==%d)\n", rc);
        exit(-1);
    }

    rc = pthread_join(pongthread, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR joining with PONG (rc==%d)\n", rc);
        exit(-1);
    }

    printf("SUCCESS!  (parent exiting)\n");
    return 0;
}
