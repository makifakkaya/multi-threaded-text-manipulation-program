#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>


char DIRECTORY[25] = "./";
int NUM_THREADS;
int sizeOfArray = 8;
int indexNumber = 0;
char ** arrayOfWords;

typedef struct {
    char fileBuffer[300];
} TextFile;

TextFile textFiles[256];
int numberOfTexts = 0;

pthread_mutex_t mutexQ;
pthread_mutex_t mutexQ2;

int controlArray(char * word) {
    for (int i = 0; i < indexNumber; i++) {
        if (!strcmp(word, arrayOfWords[i])) {
            return i;
        }
    }
    return -1;
}

void takeOutWord(char * src, char * dst) {
    for (;* src; ++src)
        if (!ispunct((unsigned char) * src))
            *
            dst++ = ((unsigned char) * src);
    * dst = 0;
}

void ReadWords(TextFile * textFile) {
    char file[300];
    strcpy(file, textFile -> fileBuffer);

    FILE * f;
    f = fopen(file, "r");
    if (f) {
        while (!feof(f)) {
            char * currentWord = malloc(50);

            fscanf(f, "%s", currentWord);
            // to remove punctuation marks.
            takeOutWord(currentWord, currentWord);
            if (indexNumber >= sizeOfArray) {
                sizeOfArray *= 2;
                arrayOfWords = realloc(arrayOfWords, sizeof(char * ) * sizeOfArray);
                printf("THREAD %lu: Re-allocated array of %d pointers.\n", pthread_self(), sizeOfArray);

            }
            pthread_mutex_lock( & mutexQ2);

            int result = controlArray(currentWord);
            // if result does not equel -1 then we can not add to array.
            if (result == -1) {
                arrayOfWords[indexNumber] = currentWord;
                printf("THREAD %lu: Added '%s' at index %d\n", pthread_self(), currentWord, indexNumber);

                indexNumber++;

            } else {
                printf("THREAD %lu: The word '%s' has already located at index %d.\n", pthread_self(), currentWord, result);
            }

            pthread_mutex_unlock( & mutexQ2);

        }
        fclose(f);
    }

}

void addTextFile(TextFile textFile) {
    pthread_mutex_lock( & mutexQ);
    textFiles[numberOfTexts] = textFile;
    numberOfTexts++;
    pthread_mutex_unlock( & mutexQ);
}

void * assignTask() {
    while (1) {
        //printf("\nst girdi\n");
        TextFile textFile;
        textFile = textFiles[0];
        int found = 0;

        pthread_mutex_lock( & mutexQ);

        if (numberOfTexts > 0) {
            found = 1;
            textFile = textFiles[0];
            printf("MAIN THREAD: Assigned '%s' to worker thread %lu\n", textFile.fileBuffer, pthread_self());
            for (int i = 0; i < numberOfTexts - 1; i++) {
                textFiles[i] = textFiles[i + 1];
            }
            numberOfTexts--;
        }
        pthread_mutex_unlock( & mutexQ);
        if (found == 1) {
            ReadWords( & textFile);
        }
    }

}

int main(int argc, char * argv[]) {

    if (argc != 5) {
        return 1;
    }
    if (strcmp(argv[1], "-d") == 0 && strcmp(argv[3], "-n") == 0) {
        strcat(DIRECTORY, argv[2]);
        strcat(DIRECTORY, "/");
        NUM_THREADS = atoi(argv[4]);

    } else if (strcmp(argv[1], "-n") == 0 && strcmp(argv[3], "-d") == 0) {
        NUM_THREADS = atoi(argv[2]);
        strcat(DIRECTORY, argv[4]);
        strcat(DIRECTORY, "/");

    }

    arrayOfWords = malloc(sizeof(char * ) * sizeOfArray);

    printf("MAIN THREAD: Allocated initial array of %d pointers.\n", sizeOfArray);

    DIR * dr = opendir(DIRECTORY);
    struct dirent * de;

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init( & mutexQ, NULL);
    pthread_mutex_init( & mutexQ2, NULL);
    long i = 0;
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create( & threads[i], NULL, & assignTask, NULL) != 0) {
            perror("Error: Failed to create a thread");
        }
    }

    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de -> d_name, ".") != 0 && strcmp(de -> d_name, "..") != 0) {
            char buffer[300];
            snprintf(buffer, sizeof(char) * 300, "./input/%s", de -> d_name);
            TextFile textFile;
            strcpy(textFile.fileBuffer, buffer);

            addTextFile(textFile);
        }
    }
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error: Failed to join a thread");
        }
    }
    pthread_mutex_destroy( & mutexQ);
    pthread_mutex_destroy( & mutexQ2);
    closedir(dr);
    return 0;
}
