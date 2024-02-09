#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_PATH_LENGTH 2048

struct ThreadArgs {
    const char *directory;
    int startId;
    int endId;
};

// Dummy sorting verification function (replace with actual logic)
int isSorted(int *data, size_t size) {
    for (size_t i = 0; i < size - 1; ++i) {
        if (data[i] > data[i + 1]) {
            return 0;  // Not sorted
        }
    }
    return 1;  // Sorted
}

void *checkFiles(void *args) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    const char *directory = threadArgs->directory;

    for (int id = threadArgs->startId; id <= threadArgs->endId; ++id) {
        char unsortedPath[MAX_PATH_LENGTH];
        char sortedPath[MAX_PATH_LENGTH];

        snprintf(unsortedPath, sizeof(unsortedPath), "%s/unsorted_%d.bin", directory, id);
        snprintf(sortedPath, sizeof(sortedPath), "%s/sorted/sorted_%d.bin", directory, id);

        // Check if the sorted file exists
        int sortedFile = open(sortedPath, O_RDONLY);
        if (sortedFile == -1) {
            fprintf(stderr, "Error: Sorted file %s not found.\n", sortedPath);
            pthread_exit((void *)-1);
        }

        // Map the sorted file into memory
        int *sortedData = mmap(NULL, sizeof(int), PROT_READ, MAP_SHARED, sortedFile, 0);
        if (sortedData == MAP_FAILED) {
            perror("Error mapping sorted file");
            close(sortedFile);
            pthread_exit((void *)-1);
        }

        // Check if the unsorted file exists
        int unsortedFile = open(unsortedPath, O_RDONLY);
        if (unsortedFile == -1) {
            fprintf(stderr, "Error: Unsorted file %s not found.\n", unsortedPath);
            munmap(sortedData, sizeof(int));
            close(sortedFile);
            pthread_exit((void *)-1);
        }

        // Map the unsorted file into memory
        int *unsortedData = mmap(NULL, sizeof(int), PROT_READ, MAP_SHARED, unsortedFile, 0);
        if (unsortedData == MAP_FAILED) {
            perror("Error mapping unsorted file");
            munmap(sortedData, sizeof(int));
            close(sortedFile);
            close(unsortedFile);
            pthread_exit((void *)-1);
        }

        // Perform sorting verification logic
        if (!isSorted(sortedData, sizeof(int) / sizeof(int))) {
            fprintf(stderr, "Error: File %s is not properly sorted.\n", sortedPath);
            munmap(sortedData, sizeof(int));
            munmap(unsortedData, sizeof(int));
            close(sortedFile);
            close(unsortedFile);
            pthread_exit((void *)-1);
        }

        munmap(sortedData, sizeof(int));
        munmap(unsortedData, sizeof(int));
        close(sortedFile);
        close(unsortedFile);
    }

    pthread_exit(NULL);
}

int countSortedFiles(const char *sortedPath) {
    int count = 0;
    DIR *sortedDir = opendir(sortedPath);
    if (sortedDir) {
        struct dirent *entry;
        while ((entry = readdir(sortedDir)) != NULL) {
            if (entry->d_type == DT_REG && strncmp(entry->d_name, "sorted_", 7) == 0) {
                count++;
            }
        }
        closedir(sortedDir);
    }
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *directory = argv[1];
    int numThreads = atoi(argv[2]);

    // Check if the directory exists
    DIR *dir = opendir(directory);
    if (!dir) {
        fprintf(stderr, "Error opening directory: %s\n", directory);
        exit(EXIT_FAILURE);
    }
    closedir(dir);

    // Check if the sorted sub-directory exists, and create it if needed
    char sortedPath[MAX_PATH_LENGTH];
    snprintf(sortedPath, sizeof(sortedPath), "%s/sorted", directory);

    struct stat st_sorted;
    if (stat(sortedPath, &st_sorted) == -1) {
        // The sorted sub-directory doesn't exist, exit with an error
        fprintf(stderr, "Error: Sorted sub-directory not found.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize thread information
    pthread_t threads[numThreads];
    struct ThreadArgs threadArgs[numThreads];

    // Count the number of sorted files
    int numFiles = countSortedFiles(sortedPath);

    // Divide the work among threads
    int filesPerThread = numFiles / numThreads;
    int remainingFiles = numFiles % numThreads;
    int startId = 0;

    for (int i = 0; i < numThreads; ++i) {
        int endId = startId + filesPerThread - 1;
        
        // Distribute remaining files among the threads
        if (remainingFiles > 0) {
            endId++;
            remainingFiles--;
        }

        threadArgs[i].directory = directory;
        threadArgs[i].startId = startId;
        threadArgs[i].endId = endId;

        // Create a thread for each subset of files
        if (pthread_create(&threads[i], NULL, checkFiles, (void *)&threadArgs[i]) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }

        startId = endId + 1;
    }

    // Wait for all threads to finish
    for (int i = 0; i < numThreads; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
            exit(EXIT_FAILURE);
        }
    }

    printf("All files are properly sorted.\n");

    return 0;
}
