#include "log_api.h"

extern GtkTextBuffer *log_buffer;  // variable global def en monitor.c

void add_log_message(const char *message) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(log_buffer, &end);
    gtk_text_buffer_insert(log_buffer, &end, message, -1);
    gtk_text_buffer_insert(log_buffer, &end, "\n", -1);
}