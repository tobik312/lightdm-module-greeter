#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "module.h"


typedef struct UI_ {
    GtkWindow   **background_windows;
    GtkWindow   *primary_window;
    int         monitor_count;
    GtkWidget   *main_window;
    GtkGrid     *layout_container;
    GtkWidget   *password_label;
    GtkWidget   *password_input;
    GtkWidget   *feedback_label;

    GtkWidget   **modules;
} UI;

typedef struct Callback_{
    UI *ui;
    Module *module;
    GtkLabel *label;
} Callback;


UI *initialize_ui(Config *config);

#endif
