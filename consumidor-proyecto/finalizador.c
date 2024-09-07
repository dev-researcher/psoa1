#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared_buffer_metadata.h"
#include "shared_gui_memory.h"

#define BUFFER_ENTRY_BYTE_SIZE 30

int main(int argc, char* argv[]) {
    size_t buffer_entries = 6;
    size_t buffer_size = buffer_entries * BUFFER_ENTRY_BYTE_SIZE;
    size_t gui_entries = 200;
    size_t gui_size = gui_entries * GUI_ENTRY_BYTE_SIZE;
    printf("____________________________\n");
    double lambda = 1.0 / MEAN;
    printf("____________________________\n");
    // ACCESSING THE SHARED BUFFER'S METADATA 
    const char * shared_buffer_metadata_name = "shared_memory_metadata";
    printf("____________________________\n");
    shared_buffer_metadata * metadata = get_shared_buffer_metadata(shared_buffer_metadata_name);
    printf("____________________________\n");
    sem_t * buffer_sem = sem_open(BUFFER_SEM_NAME, 0);
    if (buffer_sem == SEM_FAILED) {
        perror("sem_open for buffer_sem failed");
        exit(EXIT_FAILURE);
    }
    printf("____________________________\n");
    sem_t * empty_sem = sem_open(EMPTY_SEM_NAME, 0);
    if (empty_sem == SEM_FAILED) {
        perror("sem_open for empty_sem failed");
        sem_close(buffer_sem);
        exit(EXIT_FAILURE);
    }
    printf("____________________________\n");
    sem_t * full_sem = sem_open(FULL_SEM_NAME, 0);
    if (full_sem == SEM_FAILED) {
        perror("sem_open for full_sem failed");
        sem_close(buffer_sem);
        sem_close(empty_sem);
        exit(EXIT_FAILURE);
    }

    // ACCESING THE SHARED BUFFER
    const char * shared_buffer_name = "shared_buffer";
    char * shared_buffer = get_shared_buffer(shared_buffer_name, metadata->buffer_size);

    const char * buffer_message_index_table_name = "buffer_message_index_table";
    int * buffer_message_index_table = get_buffer_message_index_table(buffer_message_index_table_name, metadata->entries);

    printf("#####################################\n");
/////////////////////////////////////////
    // ACCESSING THE SHARED GUI'S METADATA 
    printf("____________________________\n");
    const char * shared_gui_metadata_name = "shared_gui_memory_metadata";
    shared_gui_metadata * metadat_gui = get_shared_gui_metadata(shared_gui_metadata_name);
    printf("____________________________\n");
    sem_t * buffer_gui_sem = sem_open(BUFFER_SEM_GUI_NAME, 0);
    if (buffer_gui_sem == SEM_FAILED) {
        perror("sem_open for buffer_sem failed");
        exit(EXIT_FAILURE);
    }
    printf("____________________________\n");
    sem_t * empty_gui_sem = sem_open(EMPTY_SEM_GUI_NAME, 0);
    if (empty_sem == SEM_FAILED) {
        perror("sem_open for empty_sem failed");
        sem_close(buffer_sem);
        exit(EXIT_FAILURE);
    }
    printf("____________________________\n");
    sem_t * full_gui_sem = sem_open(FULL_SEM_GUI_NAME, 0);
    if (full_sem == SEM_FAILED) {
        perror("sem_open for full_sem failed");
        sem_close(buffer_sem);
        sem_close(empty_sem);
        exit(EXIT_FAILURE);
    }
    const char * shared_gui_buffer_name = "shared_gui_buffer";
    char * shared_buffer_gui = get_shared_buffer(shared_gui_buffer_name, metadat_gui->buffer_size);

    // Finalizador
    sem_unlink(BUFFER_SEM_NAME);
    sem_unlink(EMPTY_SEM_NAME);
    sem_unlink(FULL_SEM_NAME);
    
    sem_close(empty_sem);
    sem_close(full_sem);
    sem_close(buffer_sem);

    shm_unlink(shared_buffer_metadata_name);
    shm_unlink(shared_buffer_name);
    shm_unlink(buffer_message_index_table_name);

    munmap(metadata, sizeof(shared_buffer_metadata));
    munmap(shared_buffer, sizeof(char) * buffer_size);
    munmap(buffer_message_index_table, sizeof(int) * buffer_entries);

    /////////////////////////////////////////////////
    sem_unlink(BUFFER_SEM_GUI_NAME);
    sem_unlink(EMPTY_SEM_GUI_NAME);
    sem_unlink(FULL_SEM_GUI_NAME);
    
    sem_close(empty_gui_sem);
    sem_close(full_gui_sem);
    sem_close(buffer_gui_sem);

    shm_unlink(shared_gui_metadata_name);
    shm_unlink(shared_gui_buffer_name);

    munmap(metadat_gui, sizeof(shared_gui_metadata));
    munmap(shared_buffer_gui, sizeof(char) * gui_size);

    ///////////////////////////////////////////////////

    return 0;
}