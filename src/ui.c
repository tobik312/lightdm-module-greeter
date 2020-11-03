/* Functions related to the GUI. */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <lightdm.h>

#include "callbacks.h"
#include "ui.h"
#include "utils.h"

#include "module.c"

static UI *new_ui(void);
static void setup_background_windows(Config *config, UI *ui);
static GtkWindow *new_background_window(GdkMonitor *monitor);
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window);
//static void hide_mouse_cursor(GtkWidget *window, gpointer user_data);
//static void move_mouse_to_background_window(void);
static void setup_main_window(UI *ui);
static void place_main_window(GtkWidget *main_window,UI *ui);
static void create_and_attach_layout_container(Config *config,UI *ui);
static void create_and_attach_password_field(Config *config, UI *ui);
static void create_and_attach_feedback_label(UI *ui);
static void attach_config_colors_to_screen(Config *config);

static void create_and_attach_modules(Config *config ,UI *ui);
static void attach_style_to_module(Module *module);
gboolean set_module_label(Callback *callback);
static void place_module_window(GtkWidget *widget,Callback *params);
gboolean module_click_handler(GtkWidget *widget,GdkEventButton *event,Module *module);
gboolean module_hover(GtkWidget *widget,GdkEventButton *event,gboolean *hover);

/* Initialize the Main Window & it's Children */
UI *initialize_ui(Config *config)
{
    UI *ui = new_ui();

    ui->modules = malloc(sizeof(GtkWidget*)*config->len_of_modules);
    if(ui->modules==NULL){
        g_error("Could not allocate memory for Modules UI");
    }

    setup_background_windows(config, ui);
    //move_mouse_to_background_window();
    setup_main_window(ui);
    create_and_attach_layout_container(config, ui);
    create_and_attach_password_field(config, ui);
    create_and_attach_feedback_label(ui);

    create_and_attach_modules(config, ui);

    attach_config_colors_to_screen(config);

    //Set default cursor
    gdk_window_set_cursor(gdk_get_default_root_window(), gdk_cursor_new_for_display(gdk_display_get_default(), GDK_LEFT_PTR));

    return ui;
}


/* Create a new UI with all values initialized to NULL */
static UI *new_ui(void)
{
    UI *ui = malloc(sizeof(UI));
    if (ui == NULL) {
        g_error("Could not allocate memory for UI");
    }
    ui->background_windows = NULL;
    ui->primary_window = NULL;
    ui->monitor_count = 0;
    ui->main_window = NULL;
    ui->layout_container = NULL;
    ui->password_label = NULL;
    ui->password_input = NULL;
    ui->feedback_label = NULL;

    ui->modules = NULL;

    return ui;
}


/* Create a Background Window for Every Monitor */
static void setup_background_windows(Config *config, UI *ui)
{
    GdkDisplay *display = gdk_display_get_default();
    ui->monitor_count = gdk_display_get_n_monitors(display);
    ui->background_windows = malloc((uint) ui->monitor_count * sizeof (GtkWindow *));
    for (int m = 0; m < ui->monitor_count; m++) {
        GdkMonitor *monitor = gdk_display_get_monitor(display, m);
        if (monitor == NULL) {
            break;
        }
        GtkWindow *background_window = new_background_window(monitor);
        ui->background_windows[m] = background_window;
        
        if(ui->primary_window==NULL && gdk_monitor_is_primary(monitor))
            ui->primary_window = background_window;

        gboolean show_background_image =
            (gdk_monitor_is_primary(monitor) || config->show_image_on_all_monitors) &&
            (strcmp(config->background_image, "\"\"") != 0);
        if (show_background_image) {
            GtkStyleContext *style_context =
                gtk_widget_get_style_context(GTK_WIDGET(background_window));
            gtk_style_context_add_class(style_context, "with-image");
        }
    }
}


/* Create & Configure a Background Window for a Monitor */
static GtkWindow *new_background_window(GdkMonitor *monitor)
{
    GtkWindow *background_window = GTK_WINDOW(gtk_window_new(
        GTK_WINDOW_TOPLEVEL));
    gtk_window_set_type_hint(background_window, GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_keep_below(background_window, TRUE);
    gtk_widget_set_name(GTK_WIDGET(background_window), "background");
    // Set Window Size to Monitor Size
    set_window_to_monitor_size(monitor, background_window);
    //g_signal_connect(background_window, "realize", G_CALLBACK(hide_mouse_cursor),NULL);
    // TODO: is this needed?
    g_signal_connect(background_window, "destroy", G_CALLBACK(gtk_main_quit),NULL);

    return background_window;
}


/* Set the Window's Minimum Size to the Default Screen's Size */
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window)
{
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    gtk_widget_set_size_request(
        GTK_WIDGET(window),
        geometry.width,
        geometry.height
    );
    gtk_window_move(window, geometry.x, geometry.y);
    gtk_window_set_resizable(window, FALSE);
}

/* Create & Configure the Main Window */
static void setup_main_window(UI *ui)
{
    GtkWidget *main_window = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(ui->primary_window),main_window);

    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    ui->main_window = main_window;
}

/* Move the Main Window to the Center of the Primary Monitor
 
 * This is done after the main window is shown(via the "show" signal) so that
 * the width of the window is properly calculated. Otherwise the returned size
 * will not include the size of the password label text.
 */
static void place_main_window(GtkWidget *window,UI *ui)
{
    // Get the Geometry of the Primary Monitor
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *primary_monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle primary_monitor_geometry;
    gdk_monitor_get_geometry(primary_monitor, &primary_monitor_geometry);

    GtkRequisition *size = malloc(sizeof(GtkRequisition));
    if (size == NULL) {
        g_error("Could not allocate memory for size");
    }
    gtk_widget_get_preferred_size(window,size,NULL);

    gtk_fixed_move(GTK_FIXED(ui->main_window),window,
        primary_monitor_geometry.x + primary_monitor_geometry.width / 2 - size->width /2,
        primary_monitor_geometry.y + primary_monitor_geometry.height /2 - size->height /2
    );
}


/* Add a Layout Container for All Displayed Widgets */
static void create_and_attach_layout_container(Config *config,UI *ui)
{
    ui->layout_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->layout_container, 5);
    gtk_grid_set_row_spacing(ui->layout_container, 5);
    gtk_widget_set_name(GTK_WIDGET(ui->layout_container), "main");

    gtk_fixed_put(GTK_FIXED(ui->main_window),GTK_WIDGET(ui->layout_container),0,0);

    g_signal_connect(ui->layout_container, "show", G_CALLBACK(place_main_window),ui);
}


/* Add a label & entry field for the user's password.
 *
 * If the `show_password_label` member of `config` is FALSE,
 * `ui->password_label` is left as NULL.
 */
static void create_and_attach_password_field(Config *config, UI *ui)
{
    ui->password_input = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(ui->password_input), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(ui->password_input),
                            (gfloat) config->password_alignment);
    // TODO: The width is usually a little shorter than we specify. Is there a
    // way to force this exact character width?
    // Maybe use 2 GtkBoxes instead of a GtkGrid?
    gtk_entry_set_width_chars(GTK_ENTRY(ui->password_input),
                              config->password_input_width);
    gtk_widget_set_name(GTK_WIDGET(ui->password_input), "password");
    gtk_grid_attach(ui->layout_container, ui->password_input, 0, 0, 1, 1);

    if (config->show_password_label) {
        ui->password_label = gtk_label_new(config->password_label_text);
        gtk_label_set_justify(GTK_LABEL(ui->password_label), GTK_JUSTIFY_RIGHT);
        gtk_grid_attach_next_to(ui->layout_container, ui->password_label,
                                ui->password_input, GTK_POS_LEFT, 1, 1);
    }
}

/* Add a label for feedback to the user */
static void create_and_attach_feedback_label(UI *ui)
{
    ui->feedback_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(ui->feedback_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_no_show_all(ui->feedback_label, TRUE);
    gtk_widget_set_name(GTK_WIDGET(ui->feedback_label), "error");

    GtkWidget *attachment_point;
    gint width;
    if (ui->password_label == NULL) {
        attachment_point = ui->password_input;
        width = 1;
    } else {
        attachment_point = ui->password_label;
        width = 2;
    }

    gtk_grid_attach_next_to(ui->layout_container, ui->feedback_label,
                            attachment_point, GTK_POS_BOTTOM, width, 1);
}

/* Attach a style provider to the screen, using color options from config */
static void attach_config_colors_to_screen(Config *config)
{
    GtkCssProvider* provider = gtk_css_provider_new();

    GdkRGBA *caret_color;
    if (config->show_input_cursor) {
        caret_color = config->password_color;
    } else {
        caret_color = config->password_background_color;
    }

    char *css;
    int css_string_length = asprintf(&css,
        "* {\n"
            "font-family: %s;\n"
            "font-size: %s;\n"
            "font-weight: %s;\n"
            "font-style: %s;\n"
        "}\n"
        "#main label {\n"
            "color: %s;\n"
        "}\n"
        "#main label#error {\n"
            "color: %s;\n"
        "}\n"
        "#background {\n"
            "background-color: %s;\n"
        "}\n"
        "#background.with-image {\n"
            "background-image: image(url(%s), %s);\n"
            "background-repeat: no-repeat;\n"
            "background-position: center;\n"
            "background-size: %s;\n"
        "}\n"
        "#main, #password{\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "border-style: solid;\n"
        "}\n"
        "#main{\n"
            "background-color: %s;\n"
            "padding: %ipx;\n"
        "}\n"
        "#password {\n"
            "color: %s;\n"
            "caret-color: %s;\n"
            "background-color: %s;\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "background-image: none;\n"
            "box-shadow: none;\n"
            "border-image-width: 0;\n"
        "}\n"
        // *
        , config->font
        , config->font_size
        , config->font_weight
        , config->font_style
        // label
        , gdk_rgba_to_string(config->text_color)
        // label#error
        , gdk_rgba_to_string(config->error_color)
        // #background
        , gdk_rgba_to_string(config->background_color)
        // #background.image-background
        , config->background_image
        , gdk_rgba_to_string(config->background_color)
        , config->background_mode
        // #main, #password
        , config->border_width
        , gdk_rgba_to_string(config->border_color)
        // #main
        , gdk_rgba_to_string(config->window_color)
        , config->layout_spacing
        // #password
        , gdk_rgba_to_string(config->password_color)
        , gdk_rgba_to_string(caret_color)
        , gdk_rgba_to_string(config->password_background_color)
        , config->password_border_width
        , gdk_rgba_to_string(config->password_border_color)
    );

    if (css_string_length >= 0) {
        gtk_css_provider_load_from_data(provider, css, -1, NULL);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
    }

    g_object_unref(provider);
}

/*ModuleWindow*/
/*Create module windows attach style, set label value and set interval function*/
static void create_and_attach_modules(Config *config, UI *ui)
{
    for(guint i=0;i<config->len_of_modules;i++){
        Module *module = config->modules[i];

        /*EventWrapper*/
        ui->modules[i] = gtk_event_box_new();
        gtk_widget_set_name(ui->modules[i],module->name);
        gtk_widget_set_events(ui->modules[i],GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

        /*ModuleBox*/
        GtkWidget *module_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
        gtk_container_add(GTK_CONTAINER(ui->modules[i]),module_box);
        
        /*Label*/
        GtkWidget *label = GTK_WIDGET(gtk_label_new(module->text));
        gtk_container_add(GTK_CONTAINER(module_box),label);        

        attach_style_to_module(module);

        gtk_fixed_put(GTK_FIXED(ui->main_window),GTK_WIDGET(ui->modules[i]),0,0);

        Callback *callback = NULL;
        callback = malloc(sizeof(Callback));
        if(callback==NULL){
            g_error("Could not allocate memory for Callback");
        }
        callback->ui = ui;
        callback->module = module;
        callback->label = GTK_LABEL(label);

        set_module_label(callback);
        if(module->refresh>0)
            g_timeout_add_seconds(module->refresh,(GSourceFunc)set_module_label,callback);

        if(module->click_exec!=NULL){
            g_signal_connect(ui->modules[i],"button_press_event",G_CALLBACK(module_click_handler),module);
            g_signal_connect(ui->modules[i],"enter-notify-event",G_CALLBACK(module_hover),(gboolean*) TRUE);
            g_signal_connect(ui->modules[i],"leave-notify-event",G_CALLBACK(module_hover),(gboolean*) FALSE);
        }

        g_signal_connect(ui->modules[i],"show",G_CALLBACK(place_module_window),callback);
    }
}
/*Attach basic style to module*/
static void attach_style_to_module(Module *module)
{
    GtkCssProvider* provider = gtk_css_provider_new();
    char *style;
    int style_string_length = asprintf(&style,
        "#%s{\n"
            "margin: %s %s %s %s;\n"
        "}\n"
        "#%s box{\n"
            "font-family: %s;\n"
            "font-size: %s;\n"
            "font-weight: %s;\n"
            "font-style: %s;\n"
            "color: %s;\n"
            "background-color: %s;\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "border-style: solid;\n"
            "padding: %ipx;\n"
        "}\n"
        ,module->name
        ,module->offset_top,module->offset_right,module->offset_bottom,module->offset_left
        ,module->name
        ,module->font
        ,module->font_size
        ,module->font_weight
        ,module->font_style
        ,gdk_rgba_to_string(module->text_color)
        ,gdk_rgba_to_string(module->window_color)
        ,module->border_width
        ,gdk_rgba_to_string(module->border_color)
        ,module->layout_spacing
        //TODO COLORs get from default
    );

    if(style_string_length>=0){
        gtk_css_provider_load_from_data(provider,style, -1, NULL);
        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER + 2);
    }
}
/*Change text into module label*/
gboolean set_module_label(Callback *callback)
{
    if(callback->module->exec!=NULL)
        get_script_output(callback->module);
    
    gtk_label_set_text(callback->label,callback->module->text);
    return TRUE;
}
/*Place module window*/
static void place_module_window(GtkWidget *widget,Callback *params)
{
    UI *ui = params->ui;
    Module *module = params->module;

    // Get the Geometry of the Primary Monitor
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *primary_monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle primary_monitor_geometry;
    gdk_monitor_get_geometry(primary_monitor, &primary_monitor_geometry);

    GtkRequisition *size = malloc(sizeof(GtkRequisition));
    if (size == NULL) {
        g_error("Could not allocate memory for size");
    }
    gtk_widget_get_preferred_size(widget,size,NULL);

    gint x = 0;
    gint y = 0;

    if(module->position_x==1){
        x = primary_monitor_geometry.x + primary_monitor_geometry.width - size->width;
    }else if(module->position_x==2){
        x = primary_monitor_geometry.x + primary_monitor_geometry.width / 2 - size->width /2;
    }

    if(module->position_y==1){
        y = primary_monitor_geometry.y + primary_monitor_geometry.height - size->height;
    }else if(module->position_y==2){
        y = primary_monitor_geometry.y + primary_monitor_geometry.height /2 - size->height /2;
    }

    gtk_fixed_move(GTK_FIXED(ui->main_window),widget,x,y);
}
/*Module click handler function*/
gboolean module_click_handler(GtkWidget *widget,GdkEventButton *event,Module *module)
{
    run_script(module);
    return TRUE;
}
/*Module on hover change mouse function*/
gboolean module_hover(GtkWidget *widget,GdkEventButton *event,gboolean *hover)
{
    gdk_window_set_cursor(gdk_get_default_root_window(), 
        gdk_cursor_new_from_name(
            gdk_display_get_default(),
            (hover) ? "pointer" : "default"
        )
    );
    return TRUE;
}