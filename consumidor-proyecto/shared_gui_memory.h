#include <pthread.h>
#include <math.h>
#include <semaphore.h>

#define FULL_SEM_GUI_NAME "full_gui_sem"
#define EMPTY_SEM_GUI_NAME "empty_gui_sem"
#define BUFFER_SEM_GUI_NAME "buffer_gui_sem"
#define GUI_ENTRY_BYTE_SIZE 60
#define GUI_ENTRIES 200

typedef struct {
    int flag;
    size_t message_size;
    size_t buffer_size;
    size_t messages_created;
    size_t messages_consumed;
    size_t producers_alive;
    size_t producers_created;
    size_t consumers_alive;  
    size_t consumers_created;          
} shared_gui_metadata;

sem_t * buffer_gui_sem;
sem_t * empty_gui_sem;
sem_t * full_gui_sem;
shared_gui_metadata * metadata_gui;
sem_t * buffer_gui_sem;
const char * shared_gui_metadata_name = "shared_gui_memory_metadata";
const char * shared_gui_buffer_name = "shared_gui_buffer";
char * shared_buffer_gui;
size_t gui_entries = 200;
size_t gui_size = GUI_ENTRIES * GUI_ENTRY_BYTE_SIZE;


shared_gui_metadata * get_shared_gui_metadata(const char * shared_gui_metadata_name) {
    int shared_metadata_file_descriptor = shm_open(shared_gui_metadata_name, O_RDWR, 0666);
    if (shared_metadata_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }
    shared_gui_metadata * metadata = (shared_gui_metadata *) 
        mmap(NULL, sizeof(shared_gui_metadata), PROT_READ | PROT_WRITE, MAP_SHARED, shared_metadata_file_descriptor, 0);
    if (metadata == NULL) {
        perror("Failed to mmap metadata.");
        exit(EXIT_FAILURE);
    }
    return metadata;
}

char * create_shared_mem_buffer(const char * shared_buffer_name, size_t buffer_size) {
    int shared_buffer_file_descriptor = shm_open(shared_buffer_name, O_CREAT | O_RDWR, 0666);
    if (shared_buffer_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }
    ftruncate(shared_buffer_file_descriptor, buffer_size);
    char * shared_buffer = (char *) 
        mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_buffer_file_descriptor, 0);
    if (shared_buffer == NULL) {
        perror("Failed to mmap shared buffer.");
        exit(EXIT_FAILURE);
    }

    memset(shared_buffer, '\0', buffer_size);
    return shared_buffer;
}

shared_gui_metadata * create_shared_gui_metadata(const char * shared_gui_metadata_name, size_t buffer_size) {
    // CREATING THE SHARED BUFFER'S METADATA OBJECT (IT IS ALSO SHARED)
    int shared_metadata_file_descriptor = shm_open(shared_gui_metadata_name, O_CREAT | O_RDWR, 0666);
    if (shared_metadata_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }

    ftruncate(shared_metadata_file_descriptor, sizeof(shared_gui_metadata));

    shared_gui_metadata * shared_metadata = (shared_gui_metadata *) 
        mmap(NULL, sizeof(shared_gui_metadata), PROT_READ | PROT_WRITE, MAP_SHARED, shared_metadata_file_descriptor, 0);
    if (shared_metadata == NULL) {
        perror("Failed to get mmap buffer metadata");
        exit(EXIT_FAILURE);
    }
    printf("%zu\n", buffer_size);
    shared_metadata->buffer_size = buffer_size;
    shared_metadata->message_size = GUI_ENTRY_BYTE_SIZE-1;
    shared_metadata->consumers_alive = 0;
    shared_metadata->producers_alive = 0;
    shared_metadata->consumers_created = 0;
    shared_metadata->producers_created = 0;

    return shared_metadata;
}

void init_logger() {
    metadata_gui = get_shared_gui_metadata(shared_gui_metadata_name);
    buffer_gui_sem = sem_open(BUFFER_SEM_GUI_NAME, 0);
    if (buffer_gui_sem == SEM_FAILED) {
        perror("sem_open for buffer_sem failed");
        exit(EXIT_FAILURE);
    }
    empty_gui_sem = sem_open(EMPTY_SEM_GUI_NAME, 0);
    if (empty_gui_sem == SEM_FAILED) {
        perror("sem_open for empty_sem failed");
        sem_close(empty_gui_sem);
        exit(EXIT_FAILURE);
    }
    full_gui_sem = sem_open(FULL_SEM_GUI_NAME, 0);
    if (full_gui_sem == SEM_FAILED) {
        perror("sem_open for full_sem failed");
        sem_close(empty_gui_sem);
        sem_close(full_gui_sem);
        exit(EXIT_FAILURE);
    }
    shared_buffer_gui = get_shared_buffer(shared_gui_buffer_name, metadata_gui->buffer_size);
}


void create_logger() {
    metadata_gui = create_shared_gui_metadata(shared_gui_metadata_name, gui_size);
    shared_buffer_gui = create_shared_mem_buffer(shared_gui_buffer_name,  gui_size);

    buffer_gui_sem = sem_open(BUFFER_SEM_GUI_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (buffer_gui_sem == SEM_FAILED) {
        perror("sem_open for buffer_sem failed");
        exit(EXIT_FAILURE);
    }

    empty_gui_sem = sem_open(EMPTY_SEM_GUI_NAME, O_CREAT | O_EXCL, 0666, gui_entries);
    if (empty_gui_sem == SEM_FAILED) {
        perror("sem_open for empty_sem failed");
        sem_close(buffer_gui_sem);
        sem_unlink(BUFFER_SEM_GUI_NAME);
        exit(EXIT_FAILURE);
    }

    full_gui_sem = sem_open(FULL_SEM_GUI_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (full_gui_sem == SEM_FAILED) {
        perror("sem_open for full_sem failed");
        sem_close(buffer_gui_sem);
        sem_close(empty_gui_sem);
        sem_unlink(BUFFER_SEM_GUI_NAME);
        sem_unlink(EMPTY_SEM_GUI_NAME);
        exit(EXIT_FAILURE);
    }

}

void finalize_logger() {
    sem_unlink(BUFFER_SEM_GUI_NAME);
    sem_unlink(EMPTY_SEM_GUI_NAME);
    sem_unlink(FULL_SEM_GUI_NAME);
    
    sem_close(empty_gui_sem);
    sem_close(full_gui_sem);
    sem_close(buffer_gui_sem);

    shm_unlink(shared_gui_metadata_name);
    shm_unlink(shared_gui_buffer_name);

    munmap(metadata_gui, sizeof(shared_gui_metadata));
    munmap(shared_buffer_gui, sizeof(char) * gui_size);
}

char * log_time() {
    size_t string_size = sizeof(char)*20;
    char * current_time_string = (char *)malloc(string_size);
    time_t current_time;
    struct tm *local_time;
    time(&current_time);
    local_time = localtime(&current_time);
    strftime(current_time_string, string_size, "%Y-%m-%d %H:%M:%S", local_time);
    return current_time_string;
}

size_t string_length(char *strings[], size_t num_strings) {
    size_t total_length = 0;
    for (size_t i = 0; i < num_strings; i++) {
        total_length += strlen(strings[i]);
    }
    return total_length;
}

char * string_concat(char *strings[], size_t num_strings) {
    size_t total_length = string_length(strings, num_strings);
    
    char *result = (char *)malloc(total_length);
    if (result == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < num_strings; i++) {
        strcat(result, strings[i]);
    }
    
    return result;
}

size_t insert_log_shared_memory(char *message) {
    size_t message_len = strlen(message);
    if (message_len + 1 > metadata_gui->buffer_size) {
        printf("Buffer overflow: Not enough space for the new message.\n");
        exit(1);
    }
    size_t position = 0;
    while (position < metadata_gui->buffer_size && shared_buffer_gui[position] != '\0') {
        position++;
    }
    printf("+++++++++++++++++++++++++\n");
     printf("%zu\n", position);
    if (position + message_len < metadata_gui->buffer_size) {
        strcpy(&shared_buffer_gui[position], message);
    } else {
        printf("+++++++++++++++++++++++++\n");
        printf("%zu\n", message_len);
        printf("%zu\n", metadata_gui->buffer_size);
        printf("%zu\n", position);
        printf("Not enough space to insert the message at the found position.\n");
        exit(1);
    }
    return position;
}


void add_log(char * message) {
    sem_wait(empty_gui_sem);
        sem_wait(buffer_gui_sem);
            char * current_time_string = log_time();
            char * strings_to_concatenate[] = {current_time_string, " - " ,message, "\n\0"};
            char * concatenated_string = string_concat(strings_to_concatenate, 4);

            size_t inserted_index = insert_log_shared_memory(concatenated_string);

            free(current_time_string);
            free(concatenated_string);
        sem_post(buffer_gui_sem);
    sem_post(empty_gui_sem);
}

