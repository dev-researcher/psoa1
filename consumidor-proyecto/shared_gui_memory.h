#include <pthread.h>
#include <math.h>
#include <semaphore.h>

#define FULL_SEM_GUI_NAME "full_gui_sem"
#define EMPTY_SEM_GUI_NAME "empty_gui_sem"
#define BUFFER_SEM_GUI_NAME "buffer_gui_sem"
#define GUI_ENTRY_BYTE_SIZE 60

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
