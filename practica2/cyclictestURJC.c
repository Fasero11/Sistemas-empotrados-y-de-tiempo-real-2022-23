// Gonzalo Vega Pérez - 2022

#define __USE_GNU
#define _GNU_SOURCE 
#include <sched.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define MAX_LOOP_TIME 60 // in seconds
#define SLEEP_TIME 5000000 // in nanoseconds (5000000ns = 5 ms)
#define ONE_BILLION 1000000000
#define READS_PER_SECOND 250 // in lab pc

// Custom structure for saving everything related to latencies
struct thread_latency{
    int core_id;                // Id of the thread/core executing
    long int max_latency;       // Maximum latency obtained
    long int avg_latency;       // Average latency obtained
    long int total_latency;     // Addition of all latencies obtained
    int number_of_measurements; // Number of latencies obtained
    long int all_latencies[READS_PER_SECOND * MAX_LOOP_TIME]; // Array containing all latencies obtained
};

void create_csv(){
    FILE *fpt;
    fpt = fopen("cyclictestURJC.csv",  "w+");
    fprintf(fpt,"CPU,NUMERO_ITERACION,LATENCIA\n");
    fclose(fpt);
}

void export_to_csv(struct thread_latency thread_latency){
    int core_id, i, n_measurements;
    long int lat;
    core_id = thread_latency.core_id;
    n_measurements = thread_latency.number_of_measurements;
    FILE *fpt;
    fpt = fopen("cyclictestURJC.csv",  "a");
    for (i = 0; i < n_measurements; i++){
        lat = thread_latency.all_latencies[i];
        fprintf(fpt,"%d,%d,%ld\n",core_id,i,lat);
    }
    fclose(fpt);
    
}

long int get_plan_latency(struct timespec *lat_begin, struct timespec *lat_end,
struct timespec *sleep_time){
    // Time before sleep.
    if (clock_gettime(CLOCK_MONOTONIC, lat_begin) != 0){
        warnx("clock_gettime() failed. %s\n",strerror(errno));
        exit(1);
    }   

    // Sleep. (Task leaves CPU)
    if (nanosleep(sleep_time, NULL) < 0){
        warnx("nanosleep() failed. %s\n",strerror(errno));
        exit(1);
    }

    // Done sleeping. Task starts executing
    if (clock_gettime(CLOCK_MONOTONIC, lat_end) != 0){
    warnx("clock_gettime() failed. %s\n",strerror(errno));
    exit(1);
    }

    // Latency is the time from the end of sleep()
    // until the next line of code (clock_gettime()) is executed
    return((lat_end->tv_nsec - lat_begin->tv_nsec) + 
    ONE_BILLION * (lat_end->tv_sec - lat_begin->tv_sec) - SLEEP_TIME);

}

void *calculate_latency(void *ptr) {

    long int loop_time, lat, max_lat, total_lat;

    int core_id, number_of_measurements;
    struct thread_latency *thread_latency = ((struct thread_latency *)ptr);
    core_id = thread_latency->core_id;
    
    struct timespec sleep_time, lat_begin, lat_end, cd_begin, cd_end;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = SLEEP_TIME; // sleep for 5 miliseconds

    if (clock_gettime(CLOCK_MONOTONIC, &cd_begin) != 0){
        warnx("clock_gettime() failed. %s\n",strerror(errno));
        exit(1);
    }   

    if (clock_gettime(CLOCK_MONOTONIC, &cd_end) != 0){
        warnx("clock_gettime() failed. %s\n",strerror(errno));
        exit(1);
    }   

    loop_time = cd_end.tv_sec - cd_begin.tv_sec;
    number_of_measurements = 0;
    total_lat  = 0;
    max_lat = 0;
    lat = 0;
    while (loop_time <= MAX_LOOP_TIME){
        //DEBUG_PRINTF("loop_time: %ld\n",loop_time);

        if (clock_gettime(CLOCK_MONOTONIC, &cd_end) != 0){
            warnx("clock_gettime() failed. %s\n",strerror(errno));
            exit(1);
        }   

        lat = get_plan_latency(&lat_begin, &lat_end, &sleep_time);

        total_lat = total_lat + lat;

        if (lat > max_lat){
            max_lat = lat;
        }

        DEBUG_PRINTF("[%d]  lat: %ld\n",core_id, lat);

        // Save current latency in latencies array and add 1 to number_of_meassurements
        thread_latency->all_latencies[number_of_measurements++] = lat;
        
        loop_time = cd_end.tv_sec - cd_begin.tv_sec;
        //DEBUG_PRINTF("Slept for: [%ld s :%ld ns]\n",
        //lat_end.tv_sec - lat_begin.tv_sec, lat_end.tv_nsec - lat_begin.tv_nsec);
    }
    DEBUG_PRINTF("total_lat: %ld\n", total_lat);
    DEBUG_PRINTF("number_of_measurements: %d\n", number_of_measurements);
    printf("[%d]\tlatencia media = %09ld ns. | max = %09ld ns\n",
    core_id, total_lat/number_of_measurements, max_lat);

    thread_latency->avg_latency = total_lat/number_of_measurements;
    thread_latency->max_latency = max_lat;
    thread_latency->total_latency = total_lat;
    thread_latency->number_of_measurements = number_of_measurements;

    pthread_exit(NULL);
}

int main(int argc, char **args) {
    long int total_max_lat, total_avg_lat;
    int latency_target_fd, core_id, number_of_cores;
    cpu_set_t cpuset;    // Create set of CPUs
    struct sched_param param;

    //// Set DMA Minimum Latency ////
    static int32_t latency_target_value = 0;
    latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    write(latency_target_fd, &latency_target_value, 4);
    ////////////////////////////////

    number_of_cores = (int) sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t thread[number_of_cores];
    // Array of thread_latency structures.
    struct thread_latency thread_latencies[number_of_cores];

    DEBUG_PRINTF("Number of cores: %d\n", number_of_cores);

    param.sched_priority = 99;  // 99 = Real Time

    // Create Threads and set their sched and affinity.
    for (core_id = 0; core_id < number_of_cores; core_id++){
        
        CPU_ZERO(&cpuset);          // Clear set of CPUs
        CPU_SET(core_id, &cpuset);  // Add current CPU to set

        // Fill one thread struct for each thread.
        thread_latencies[core_id].core_id = core_id;

        DEBUG_PRINTF("Creating thread: %d\n", core_id);
        if (pthread_create(&thread[core_id], NULL, calculate_latency,
         (void *) &thread_latencies[core_id]) < 0) {
            warnx("Error while creating Thread. %s\n",strerror(errno));
            exit(1);
        }

        pthread_setschedparam(thread[core_id], SCHED_FIFO, &param);
        pthread_setaffinity_np(thread[core_id], sizeof(cpu_set_t), &cpuset);
    }

    // Join all Threads
    for (core_id = 0; core_id < number_of_cores; core_id++){
        DEBUG_PRINTF("Joining Thread %d...\n", core_id);
        if (pthread_join(thread[core_id], NULL) != 0) {
            warnx("Error while joining Thread%s\n",strerror(errno));
            exit(1);
        }
    }

    create_csv();
    // Calculate and print Avg and Max latencies of all threads together.
    for (core_id = 0; core_id < number_of_cores; core_id++){
        long int avg_lat = thread_latencies[core_id].avg_latency;
        long int max_lat =  thread_latencies[core_id].max_latency;

        total_avg_lat = total_avg_lat + avg_lat;

        if (max_lat > total_max_lat){
            total_max_lat = max_lat;
        }

        // Export current thread information to a .csv file
        export_to_csv(thread_latencies[core_id]);
    }

    printf("\nTotal\tlatencia media = %09ld ns. | max = %09ld ns\n",
    total_avg_lat/number_of_cores, total_max_lat);

    return 0;
}