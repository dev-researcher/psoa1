#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "shared_buffer_metadata.h"
#include "shared_gui_memory.h"


char * consume_message(char * buffer, size_t message_size, size_t message_index);
char * generate_current_time_string();
char * generate_random_key_string();
char * pid_to_string();
char * concatenate_strings(char *strings[], size_t num_strings);
size_t insert_message_in_buffer(char *buffer, size_t buffer_size, const char *message);


int main(int argc, char* argv[]) {
    double lambda = 1 / MEAN;

    const char * shared_buffer_metadata_name = "shared_memory_metadata";
    shared_buffer_metadata * metadata = get_shared_buffer_metadata(shared_buffer_metadata_name);

    sem_t * buffer_sem = sem_open(BUFFER_SEM_NAME, 0);
    if (buffer_sem == SEM_FAILED) {
        perror("sem_open for buffer_sem failed");
        exit(EXIT_FAILURE);
    }

    sem_t * empty_sem = sem_open(EMPTY_SEM_NAME, 0);
    if (empty_sem == SEM_FAILED) {
        perror("sem_open for empty_sem failed");
        sem_close(buffer_sem);
        exit(EXIT_FAILURE);
    }

    sem_t * full_sem = sem_open(FULL_SEM_NAME, 0);
    if (full_sem == SEM_FAILED) {
        perror("sem_open for full_sem failed");
        sem_close(buffer_sem);
        sem_close(empty_sem);
        exit(EXIT_FAILURE);
    }


    sem_wait(buffer_sem);
        metadata->consumers_alive += 1;
    sem_post(buffer_sem);


    const char * shared_buffer_name = "shared_buffer";
    char * shared_buffer = get_shared_buffer(shared_buffer_name, metadata->buffer_size);

    const char * buffer_message_index_table_name = "buffer_message_index_table";
    int * buffer_message_index_table = get_buffer_message_index_table(buffer_message_index_table_name, metadata->entries);


    /////////////////////////////////////////
    // ACCESSING THE SHARED GUI'S METADATA 
    char * pid_string = pid_to_string();

    init_logger();
    // se incrementa el numero de productores vivos ademas del numero de productores alguna vez creados
    sem_wait(buffer_gui_sem);
        metadata_gui->consumers_alive += 1;
        metadata_gui->consumers_created += 1;
    sem_post(buffer_gui_sem);
    // ACCESING THE LOGs SHARED BUFFER
    // se agrega un log para el UI con el evento de creación de un nuevo productor

    char * strings_to_concatenate[] = {"Nuevo consumidor creado, PID: : ", pid_string};
    char * concatenated_string = string_concat(strings_to_concatenate, 2);
    add_log(concatenated_string);
    free(concatenated_string);

    while (1) {
        wait_random_time(lambda);
        sem_wait(full_sem);
            sem_wait(buffer_sem);
                size_t message_index = -1;
                for (int index = 0; index < metadata->entries; index += 1) {
                    if (buffer_message_index_table[index] != -1) {
                        message_index = buffer_message_index_table[index];
                        buffer_message_index_table[index] = -1;
                        break;
                    }
                }

                char * message = consume_message(shared_buffer, metadata->message_size,  message_index);

                free(message);
            sem_post(buffer_sem);
        sem_post(empty_sem);

        char * strings_to_concatenate_log[]  = {" - Un nuevo mensaje ha sido consumido por el PID: ", pid_string};
        concatenated_string = string_concat(strings_to_concatenate_log, 2);
        add_log(concatenated_string);
    }
}



char * consume_message(char * buffer, size_t message_size, size_t message_index) {
    char * message = (char *)malloc(sizeof(char) * message_size);
    int current_message_character = 0;

    while (current_message_character < message_size) {
        message[current_message_character] = buffer[message_index];
        buffer[message_index] = '\0';
        current_message_character += 1;
        message_index += 1;
    }
    return message;
}

char * pid_to_string() {
    pid_t pid = getpid();
    char * hex_pid_string = (char *)malloc(sizeof(char) * 9);
    snprintf(hex_pid_string, (sizeof(char) * 9), "%08X", pid);
    return hex_pid_string;
}


size_t insert_message_in_buffer(char *buffer, size_t buffer_size, const char *message) {
    size_t message_len = strlen(message);
    if (message_len + 1 > buffer_size) {
        printf("Buffer overflow: Not enough space for the new message.\n");
        exit(1);
    }
    size_t position = 0;
    while (position < buffer_size && buffer[position] != '\0') {
        position++;
    }
    if (position + message_len < buffer_size) {
        strcpy(&buffer[position], message);
    } else {
        printf("+++++++++++++++++++++++++\n");
        printf("%zu\n", message_len);
        printf("%zu\n", buffer_size);
        printf("%zu\n", position);
        printf("Not enough space to insert the message at the found position.\n");
        exit(1);
    }
    return position;
}

size_t calculate_total_length(char *strings[], size_t num_strings) {
    size_t total_length = 0;
    for (size_t i = 0; i < num_strings; i++) {
        total_length += strlen(strings[i]);
    }
    return total_length;
}

char * concatenate_strings(char *strings[], size_t num_strings) {
    size_t total_length = calculate_total_length(strings, num_strings);
    
    char *result = (char *)malloc(total_length + 1);
    if (result == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    result[0] = '\0';

    for (size_t i = 0; i < num_strings; i++) {
        strcat(result, strings[i]);
    }
    
    return result;
}


char * generate_current_time_string() {
    size_t string_size = sizeof(char)*20;
    char * current_time_string = (char *)malloc(string_size);
    time_t current_time;
    struct tm *local_time;
    time(&current_time);
    local_time = localtime(&current_time);
    strftime(current_time_string, string_size, "%Y-%m-%d %H:%M:%S", local_time);
    return current_time_string;
}

char * generate_random_key_string() {
    srand(time(NULL));
    int random_key = rand() % 100;
    char *random_key_string = (char *)malloc(3 * sizeof(char)); 
    if (random_key_string == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    snprintf(random_key_string, 3, "%02d", random_key);

    return random_key_string;
}