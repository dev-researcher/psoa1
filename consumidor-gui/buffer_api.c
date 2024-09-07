#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "buffer_api.h"

extern GtkWidget *buffer_label;
extern char buffer_data[256];
extern int num_productores;
extern int num_consumidores;

void update_buffer_view(const char *new_buffer_data) {
    strcpy(buffer_data, new_buffer_data);
    char buffer_display[512];
    snprintf(buffer_display, sizeof(buffer_display), "Buffer: %s\nProductores: %d\nConsumidores: %d", buffer_data, num_productores, num_consumidores);
    gtk_label_set_text(GTK_LABEL(buffer_label), buffer_display);
}
