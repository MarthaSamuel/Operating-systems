#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXTHREADS 50
#define SLEEPTIME 5

// Structure to hold thread-specific data
struct ThreadData {
    int thread_id;
    unsigned long long result;
};

void *Factorial(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
  
    /* leave this sleep in place, it will help with debugging by varying the orders of the threads */
    sleep(SLEEPTIME);

    if (data->thread_id > 20) {
        fprintf(stderr, "ERROR: thread %d exit to avoid long long overflow\n", data->thread_id);
        return NULL;
    }

    data->result = 1;
    for (int j = 1; j <= data->thread_id; j++) {
        data->result *= j;
    }

    printf("Thread %d result is %llu\n", data->thread_id, data->result);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: helloT <nthreads>\n");
        exit(-1);
    }

    int nthreads = atoi(argv[1]);
    if (nthreads < 1 || nthreads > MAXTHREADS) {
        fprintf(stderr, "ERROR: numthreads must be between 1 and %d\n", MAXTHREADS);
        exit(-1);
    }

    printf("User requested %d threads\n", nthreads);

    pthread_t threads[MAXTHREADS];
    struct ThreadData thread_data[MAXTHREADS];

    // Create threads
    for (int i = 0; i < nthreads; i++) {
        thread_data[i].thread_id = i;

        int rc = pthread_create(&threads[i], NULL, Factorial, (void *)&thread_data[i]);
        if (rc) {
            fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    exit(0);
}



