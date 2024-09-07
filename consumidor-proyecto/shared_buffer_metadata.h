#include <pthread.h>
#include <math.h>
#include <semaphore.h>

//  gcc `pkg-config --cflags gtk4` -o gui gui.c `pkg-config --libs gtk4`

#define MEAN 2.0

#define FULL_SEM_NAME "full_sem"
#define EMPTY_SEM_NAME "empty_sem"
#define BUFFER_SEM_NAME "buffer_sem"

typedef struct {
    int flag;
    size_t message_size;
    size_t buffer_size;
    size_t entries;
    size_t producers_alive;
    size_t consumers_alive;            
} shared_buffer_metadata;

void wait_random_time(double lambda) {
    double u = (double)rand() / RAND_MAX;
    double wait_time_seconds = -log(1 - u) / lambda;     
    usleep(wait_time_seconds);
}

char * get_shared_buffer(const char * shared_buffer_name, size_t buffer_size) {
    int shared_buffer_file_descriptor = shm_open(shared_buffer_name, O_RDWR, 0666);
    if (shared_buffer_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }
    char * shared_buffer = (char *) mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_buffer_file_descriptor, 0);
    if (shared_buffer == NULL) {
        perror("Failed to mmap shared buffer.");
        exit(EXIT_FAILURE);
    }

    return shared_buffer;
}

shared_buffer_metadata * get_shared_buffer_metadata(const char * shared_buffer_metadata_name) {
    int shared_metadata_file_descriptor = shm_open(shared_buffer_metadata_name, O_RDWR, 0666);
    if (shared_metadata_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }
    shared_buffer_metadata * metadata = (shared_buffer_metadata *) 
        mmap(NULL, sizeof(shared_buffer_metadata), PROT_READ | PROT_WRITE, MAP_SHARED, shared_metadata_file_descriptor, 0);
    if (metadata == NULL) {
        perror("Failed to mmap metadata.");
        exit(EXIT_FAILURE);
    }

    return metadata;
}

int * get_buffer_message_index_table(const char * buffer_message_index_table_name, size_t entries) {
    int buffer_message_index_table_file_descriptor = shm_open(buffer_message_index_table_name, O_RDWR, 0666);
    if (buffer_message_index_table_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }
    int * index_table = (int *) mmap(NULL, sizeof(int) * entries, PROT_READ | PROT_WRITE, MAP_SHARED, buffer_message_index_table_file_descriptor, 0);
    if (index_table == NULL) {
        perror("Failed to mmap index table.");
        exit(EXIT_FAILURE);
    }

    return index_table;
}

void print_buffer(char * buffer, size_t buffer_length) {
    for (size_t i = 0; i < buffer_length; i += 1) {
        if (buffer[i] == '\0') {
            printf("_");
        } else {
            printf("%c", buffer[i]);
        }
    }
    printf("\n");
}

void print_index_table(int * table, size_t entries) {
    for (size_t i = 0; i < entries; i += 1) {
        printf(" %d ", table[i]);
    }
    printf("\n");
}