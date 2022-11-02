// Gonzalo Vega Pérez - 2022

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <err.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

//////////// MODIFY /////////////

#define PERIOD 0.9 // in seconds (float)
#define NUMBER_OF_THREADS 4
#define ITERATIONS_PER_THREAD 5
#define LOOPS_PER_ITERATION 375000000ULL // process time ~0.5s (lab_3203 pc_05)

/////////////////////////////////

#define ONE_BILLION 1000000000

void error_and_exit(){
    fprintf(stderr, "errno=%d, err_msg=\"%s\"\n", errno,strerror(errno));
    exit(EXIT_FAILURE);
}

void get_time(struct timespec *begin_real, struct timespec *begin, 
struct timespec *end) {

    volatile unsigned long long j;

    if (clock_gettime(CLOCK_REALTIME, begin_real) != 0){
        error_and_exit();
    } 

    if (clock_gettime(CLOCK_MONOTONIC, begin) != 0){
        error_and_exit();
    }   

    for (j=0; j < LOOPS_PER_ITERATION; j++){  
    }
    
    if (clock_gettime(CLOCK_MONOTONIC, end) != 0) {
        error_and_exit();
    }
    
}

void *iterate(void *ptr) {

    // begin real: used for timestamp
    // begin & end: used for calculating cost
    // sleep_time: used for sleep time
    struct timespec begin_real, begin, end, sleep_time;

    char *message;
    message = (char *) ptr;
    
    int iteration;
    for(iteration=1; iteration <= ITERATIONS_PER_THREAD; iteration++) {

        get_time(&begin_real, &begin, &end);

        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = PERIOD * ONE_BILLION;

        long int diff_sec = (end.tv_sec - begin.tv_sec);
        long int diff_nsec = (end.tv_nsec - begin.tv_nsec);
        long int diff_total = (diff_nsec + diff_sec * ONE_BILLION);
        float loop_time = diff_total / (float)ONE_BILLION;

        printf("[%ld,%ld] %s - Iteracion %d: Coste=%.2f s.",begin_real.tv_sec, 
        begin_real.tv_nsec, message, iteration, loop_time);

        if (loop_time <= PERIOD){
            printf("\n");
            sleep_time.tv_nsec = sleep_time.tv_nsec - diff_total;

            // Fixes overflow in nsec. One Billion nsecs -> 1 sec
            while (sleep_time.tv_nsec >= ONE_BILLION){
                sleep_time.tv_sec = sleep_time.tv_sec + 1;
                sleep_time.tv_nsec = sleep_time.tv_nsec - ONE_BILLION;
            }

            DEBUG_PRINTF("SleepTime: %ld sec, %ld nsec\n",sleep_time.tv_sec,
            sleep_time.tv_nsec);

            if (nanosleep(&sleep_time, NULL) < 0){
                error_and_exit();
            }
        } else {
            printf("(fallo temporal) \n");
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char **args) {

    pthread_t thread[NUMBER_OF_THREADS];
    int irets[NUMBER_OF_THREADS];
    char *buffers[NUMBER_OF_THREADS];
    int thread_id = 0;

    int i;
    for(i = 0; i < NUMBER_OF_THREADS; i++)
    {
        int iret;
        irets[i] = iret;

        // used for naming threads
        char *buffer = malloc(snprintf(NULL, 0, "Thread %d", i+1));
        if (buffer == NULL) {
            error_and_exit();
        }

        if (sprintf(buffer, "Thread %d", i+1) < 0) {
            error_and_exit();
        }

        buffers[i] = buffer;

        DEBUG_PRINTF("Buffer: %s\n",buffer);

        irets[i] = pthread_create(&thread[i], NULL, iterate, (void*) buffer);
        if (irets[i] != 0) {
            warnx("Error while creating Thread\n");
        }
    }

    for(i = 0; i < NUMBER_OF_THREADS; i++){
        DEBUG_PRINTF("Thread %d returns: %d\n",i+1,irets[i]);
        if (pthread_join(thread[i], NULL) != 0) {
            warnx("Error while joining Thread\n");
        }
        free(buffers[i]);
    }

    return 0;
}