#ifndef MODULE_H
#define MODULE_H

#include <gdk/gdk.h>
#include <glib.h>

// Represents the Module Configuration. Parsed from config file.
typedef struct Module_ {
    gchar    *name;

    gchar    *font;
    gchar    *font_size;
    gchar    *font_weight;
    gchar    *font_style;
    GdkRGBA  *text_color;
    GdkRGBA  *window_color;
    GdkRGBA  *border_color;
    gchar    *border_width;
    gint     layout_spacing;

    gint     position_x;
    gint     position_y;
    gchar    *offset_top;
    gchar    *offset_right;
    gchar    *offset_bottom;
    gchar    *offset_left;

    gchar    *text;
    gchar    *exec;
    gchar    *click_exec;
    guint     refresh;
} Module;

void get_script_output(Module *module);
void run_script(Module *module);
void destroy_module(Module *module);

#endif