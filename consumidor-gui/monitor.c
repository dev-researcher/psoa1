#include <gtk/gtk.h>
#include "log_api.h"
#include "buffer_api.h"
#include "process_api.h"

GtkWidget *log_view;
GtkTextBuffer *log_buffer;
GtkWidget *buffer_label;
char buffer_data[256];
int num_productores = 0;
int num_consumidores = 0;

void init_gui() {
    g_timeout_add(5000, (GSourceFunc)update_buffer_view, "Buffer inicializado");
}

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *scrolled_window;
    GtkWidget *btn_create_productor, *btn_create_consumidor;

    // Crear ventana
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Monitor del Sistema");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

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

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.GtkApplication", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}