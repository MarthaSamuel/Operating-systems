#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_PATH_LENGTH 2048

// Simple bubble sort algorithm
void bubbleSort(int arr[], int n) {
    int i, j;
    for (i = 0; i < n-1; i++)
        for (j = 0; j < n-i-1; j++)
            if (arr[j] > arr[j+1]) {
                // Swap arr[j] and arr[j+1]
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
}

void sortAndWriteFile(const char *inputFilename, const char *outputFilename) {
    int fd_input = open(inputFilename, O_RDONLY);
    if (fd_input == -1) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Get the size of the input file
    struct stat st;
    if (fstat(fd_input, &st) == -1) {
        perror("Error getting file size");
        exit(EXIT_FAILURE);
    }
    off_t file_size = st.st_size;
    int num_integers = file_size / sizeof(int);

    // Read integers from the file
    int *data = malloc(file_size);
    if (read(fd_input, data, file_size) == -1) {
        perror("Error reading from input file");
        exit(EXIT_FAILURE);
    }

    // Close the input file
    if (close(fd_input) == -1) {
        perror("Error closing input file");
        exit(EXIT_FAILURE);
    }

    // Sort the array
    bubbleSort(data, num_integers);

    // Open the output file
    int fd_output = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_output == -1) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    // Write the sorted integers to the output file
    if (write(fd_output, data, file_size) == -1) {
        perror("Error writing to output file");
        exit(EXIT_FAILURE);
    }

    // Close the output file
    if (close(fd_output) == -1) {
        perror("Error closing output file");
        exit(EXIT_FAILURE);
    }

    free(data);
}

void processDirectory(const char *directory) {
    DIR *dir = opendir(directory);
    if (!dir) {
        fprintf(stderr, "Error opening directory: %s\n", directory);
        exit(EXIT_FAILURE);
    }

    // Check if the directory is empty
    struct dirent *entry = readdir(dir);
    if (!entry) {
        fprintf(stderr, "Error: Directory %s is empty.\n", directory);
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    // Rewind the directory stream to process entries
    rewinddir(dir);
    // Flag to check if any regular files were found
    int filesFound = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strncmp(entry->d_name, "unsorted_", 9) == 0) {
            char inputPath[MAX_PATH_LENGTH];
            char outputPath[MAX_PATH_LENGTH];
            snprintf(inputPath, sizeof(inputPath), "%s/%s", directory, entry->d_name);
            snprintf(outputPath, sizeof(outputPath), "%s/sorted/sorted_%s", directory, entry->d_name + 9);

            sortAndWriteFile(inputPath, outputPath);
            filesFound = 1;
        }
    }

    closedir(dir);
    // Check if no regular files were found
    if (!filesFound) {
        fprintf(stderr, "Error: No regular files found in directory %s\n", directory);
        exit(EXIT_FAILURE);
    }
}

int isFileSorted(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    int *data = malloc(file_size);
    if (read(fd, data, file_size) == -1) {
        perror("Error reading from file");
        exit(EXIT_FAILURE);
    }

    close(fd);

    for (int i = 0; i < (file_size / sizeof(int)) - 1; i++) {        if (data[i] > data[i + 1]) {
            free(data);
            return 0;  // Not sorted
        }
    }

    free(data);
    return 1;  // Sorted
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *directory = argv[1];

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
        // The sorted sub-directory doesn't exist, create it
        if (mkdir(sortedPath, S_IRWXU) == -1) {
            perror("Error creating sorted sub-directory");
            exit(EXIT_FAILURE);
        }
    } else {
        // Check if the sorted sub-directory is writable
        if (!(st_sorted.st_mode & S_IWUSR)) {
            fprintf(stderr, "Error: Sorted sub-directory is read-only.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Process the directory
    processDirectory(directory);

    printf("Sorting completed successfully.\n");

    // Check if the sorted files are actually sorted
    DIR *dir_sorted = opendir(sortedPath);
    if (!dir_sorted) {
        perror("Error opening sorted directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry_sorted;
    while ((entry_sorted = readdir(dir_sorted)) != NULL) {
        if (entry_sorted->d_type == DT_REG && strncmp(entry_sorted->d_name, "sorted_", 7) == 0) {
            char filePath[MAX_PATH_LENGTH * 2];
            snprintf(filePath, sizeof(filePath), "%s/%s", sortedPath, entry_sorted->d_name);

            if (!isFileSorted(filePath)) {
                fprintf(stderr, "File %s is not sorted.\n", entry_sorted->d_name);
                exit(EXIT_FAILURE);
            }
        }
    }

    closedir(dir_sorted);

    printf("All sorted files are verified to be sorted.\n");

    return 0;
}
