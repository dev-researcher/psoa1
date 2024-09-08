
#include <gtk/gtk.h>
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
#include "../consumidor-gui/log_api.h"
#include "../consumidor-gui/buffer_api.h"
#include "../consumidor-gui/process_api.h"

#include "shared_buffer_metadata.h"
#include "shared_gui_memory.h"

#define BUFFER_ENTRY_BYTE_SIZE 30

GtkWidget *log_view;
GtkTextBuffer *log_buffer;
GtkWidget *buffer_label;
char buffer_data[256];
int num_productores = 0;
int num_consumidores = 0;


int ui_buffer_current_pos = -1;
sem_t * buffer_gui_sem;
sem_t * empty_gui_sem;
sem_t * full_gui_sem;

shared_buffer_metadata * create_shared_buffer_metadata(const char * shared_buffer_metadata_name, size_t buffer_size) {
    // CREATING THE SHARED BUFFER'S METADATA OBJECT (IT IS ALSO SHARED)
    int shared_metadata_file_descriptor = shm_open(shared_buffer_metadata_name, O_CREAT | O_RDWR, 0666);
    if (shared_metadata_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }

    ftruncate(shared_metadata_file_descriptor, sizeof(shared_buffer_metadata));

    shared_buffer_metadata * shared_metadata = (shared_buffer_metadata *) 
        mmap(NULL, sizeof(shared_buffer_metadata), PROT_READ | PROT_WRITE, MAP_SHARED, shared_metadata_file_descriptor, 0);
    if (shared_metadata == NULL) {
        perror("Failed to get mmap buffer metadata");
        exit(EXIT_FAILURE);
    }

    size_t buffer_entries = buffer_size / BUFFER_ENTRY_BYTE_SIZE;

    shared_metadata->buffer_size = buffer_size;
    shared_metadata->message_size = BUFFER_ENTRY_BYTE_SIZE-1;
    shared_metadata->consumers_alive = 0;
    shared_metadata->producers_alive = 0;
    shared_metadata->entries = buffer_entries;

    return shared_metadata;
}

char * create_shared_buffer(const char * shared_buffer_name, size_t buffer_size) {
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

int * create_buffer_message_index_table(const char * buffer_message_index_table_name, size_t entries) {
    int buffer_message_index_table_file_descriptor = shm_open(buffer_message_index_table_name, O_CREAT | O_RDWR, 0666);
    if (buffer_message_index_table_file_descriptor == -1) {
        perror("shm_open");
        exit(1);
    }
    ftruncate(buffer_message_index_table_file_descriptor, sizeof(int)*entries);
    int * index_table = (int *) 
        mmap(NULL, sizeof(int)*entries, PROT_READ | PROT_WRITE, MAP_SHARED, buffer_message_index_table_file_descriptor, 0);
    if (index_table == NULL) {
        perror("Failed to mmap index table.");
        exit(EXIT_FAILURE);
    }

    memset(index_table, -1, entries);
    for (int i = 0; i < entries; i += 1) {
        index_table[i] = -1;
    }
    return index_table;
}

void init_gui() {
    g_timeout_add(5000, (GSourceFunc)update_buffer_view, "Buffer inicializado");
}

void *loadEvents(void *threadid)
{
    int gui_total_size = GUI_ENTRIES * GUI_ENTRY_BYTE_SIZE;
    while(1) {
        while(ui_buffer_current_pos < gui_total_size && shared_buffer_gui[ui_buffer_current_pos+1] != '\0')
        {

            printf("%d\n", ui_buffer_current_pos);
            add_log_message(shared_buffer_gui+ui_buffer_current_pos+1);
           for(; ui_buffer_current_pos < gui_total_size && shared_buffer_gui[ui_buffer_current_pos+1] != '\0'; ui_buffer_current_pos++) {
            }
        }
        sleep(2);
    }
    pthread_exit(NULL);
}

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *scrolled_window;
    GtkWidget *btn_create_productor, *btn_create_consumidor;

    // Crear ventana
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Monitor del Sistema");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    // Crear un grid para colocar los widgets
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Crear un área de texto para la bitácora
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_grid_attach(GTK_GRID(grid), scrolled_window, 0, 0, 2, 1);
    
    log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    gtk_container_add(GTK_CONTAINER(scrolled_window), log_view);

    // Crear un label para mostrar el estado del buffer
    buffer_label = gtk_label_new("Buffer vacío");
    gtk_grid_attach(GTK_GRID(grid), buffer_label, 0, 1, 2, 1);
    pthread_t thread;
    pthread_create(&thread, NULL, loadEvents, NULL);

    gtk_widget_show_all(window);
}

int main(int argc, char* argv[]) {
    size_t buffer_entries = 6;
    size_t buffer_size = buffer_entries * BUFFER_ENTRY_BYTE_SIZE;
    const char * shared_buffer_metadata_name = "shared_memory_metadata";
    shared_buffer_metadata * metadata = create_shared_buffer_metadata(shared_buffer_metadata_name, buffer_size);

    const char * shared_buffer_name = "shared_buffer";
    char * shared_buffer = create_shared_buffer(shared_buffer_name,  buffer_size);

    const char * buffer_message_index_table_name = "buffer_message_index_table";
    int * index_table = create_buffer_message_index_table( buffer_message_index_table_name, buffer_entries);

    sem_t * buffer_sem = sem_open(BUFFER_SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (buffer_sem == SEM_FAILED) {
        perror("sem_open for buffer_sem failed");
        exit(EXIT_FAILURE);
    }

    sem_t * empty_sem = sem_open(EMPTY_SEM_NAME, O_CREAT | O_EXCL, 0666, buffer_entries);
    if (empty_sem == SEM_FAILED) {
        perror("sem_open for empty_sem failed");
        sem_close(buffer_sem);
        sem_unlink(BUFFER_SEM_NAME);
        exit(EXIT_FAILURE);
    }

    sem_t * full_sem = sem_open(FULL_SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (full_sem == SEM_FAILED) {
        perror("sem_open for full_sem failed");
        sem_close(buffer_sem);
        sem_close(empty_sem);
        sem_unlink(BUFFER_SEM_NAME);
        sem_unlink(EMPTY_SEM_NAME);
        exit(EXIT_FAILURE);
    }


    ///////////////////
    // Shared UI 
    create_logger();
    ///////////////////

    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.GtkApplication", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

   for (int i = 0; i < 20; i += 1){
    sleep(1);
    print_buffer(shared_buffer, buffer_size);
    print_buffer(shared_buffer_gui, buffer_size);
   }

    // UI

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
    munmap(index_table, sizeof(int) * buffer_entries);

    /////////////////////////////////////////////////
    finalize_logger();
    ///////////////////////////////////////////////////

    return 0;
}