#include <gtk/gtk.h>
#include "process_api.h"
#include "log_api.h"  // bitácora
#include "buffer_api.h"  // para actualizar el buffer

extern int num_productores;
extern int num_consumidores;

void create_new_process(GtkWidget *widget, gpointer data) {
    gboolean is_productor = GPOINTER_TO_INT(data);

    if (is_productor) {
        num_productores++;
        add_log_message("Nuevo Productor creado");
    } else {
        num_consumidores++;
        add_log_message("Nuevo Consumidor creado");
    }

    // Simular una actualización en el buffer
    update_buffer_view("Datos actualizados en el buffer");
}
