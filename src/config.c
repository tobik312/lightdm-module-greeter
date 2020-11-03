/* Functions related to the Configuration */
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <glib.h>

#include "config.h"
#include "utils.h"
#include "module.h"


static gchar *parse_greeter_string(GKeyFile *keyfile, const char *group_name,
                                   const char *key_name, const gchar *fallback);
static gint parse_greeter_integer(GKeyFile *keyfile, const char *group_name,
                                  const char *key_name, const gint fallback);
static GdkRGBA *parse_greeter_color_key(GKeyFile *keyfile,const char *group_name,
                                  const char *key_name,const gchar *fallback);
static GdkRGBA *parse_greeter_color_key2(GKeyFile *keyfile,const char *group_name,
                                  const char *key_name,const GdkRGBA *fallback);
static guint parse_greeter_hotkey_keyval(GKeyFile *keyfile, const char *key_name);
static gboolean parse_greeter_password_alignment(GKeyFile *keyfile);
static gboolean is_rtl_keymap_layout(void);

/* Initialize the configuration, sourcing the greeter's configuration file */
Config *initialize_config(void)
{
    Config *config = malloc(sizeof(Config));
    if (config == NULL) {
        g_error("Could not allocate memory for Config");
    }

    // Load the key-value file
    GKeyFile *keyfile = g_key_file_new();
    gboolean keyfile_loaded = g_key_file_load_from_file(
        keyfile, CONFIG_FILE, G_KEY_FILE_NONE, NULL);
    if (!keyfile_loaded) {
        g_error("Could not load configuration file.");
    }

    // Parse values from the keyfile into a Config.
    config->login_user =
        g_strchomp(g_key_file_get_string(keyfile, "greeter", "user", NULL));
    if (strcmp(config->login_user, "CHANGE_ME") == 0) {
        g_message("User configuration value is unchanged.");
    }
    config->show_password_label =
        g_key_file_get_boolean(keyfile, "greeter", "show-password-label", NULL);
    config->password_label_text = parse_greeter_string(
        keyfile, "greeter", "password-label-text", "Password:");
    config->invalid_password_text = parse_greeter_string(
        keyfile, "greeter", "invalid-password-text", "Invalid Password");
    config->show_input_cursor =
        g_key_file_get_boolean(keyfile, "greeter", "show-input-cursor", NULL);
    config->password_alignment = parse_greeter_password_alignment(keyfile);
    config->password_input_width = parse_greeter_integer(
        keyfile, "greeter", "password-input-width", -1);
    config->show_image_on_all_monitors = g_key_file_get_boolean(
        keyfile, "greeter", "show-image-on-all-monitors", NULL);

    // Parse Hotkey Settings
    config->suspend_key = parse_greeter_hotkey_keyval(keyfile, "suspend-key");
    config->hibernate_key = parse_greeter_hotkey_keyval(keyfile, "hibernate-key");
    config->restart_key = parse_greeter_hotkey_keyval(keyfile, "restart-key");
    config->shutdown_key = parse_greeter_hotkey_keyval(keyfile, "shutdown-key");
    gchar *mod_key =
        g_key_file_get_string(keyfile, "greeter-hotkeys", "mod-key", NULL);
    if (mod_key == NULL) {
        config->mod_bit = GDK_SUPER_MASK;
    } else {
        g_strchomp(mod_key);
        if (strcmp(mod_key, "control") == 0) {
            config->mod_bit = GDK_CONTROL_MASK;
        } else if (strcmp(mod_key, "alt") == 0) {
            config->mod_bit = GDK_MOD1_MASK;
        } else if (strcmp(mod_key, "meta") == 0) {
            config->mod_bit = GDK_SUPER_MASK;
        } else {
            g_error("Invalid mod-key configuration value: '%s'\n", mod_key);
        }
    }

    // Parse Theme Settings
    // Font
    config->font =
        g_key_file_get_string(keyfile, "greeter-theme", "font", NULL);
    config->font_size =
        g_key_file_get_string(keyfile, "greeter-theme", "font-size", NULL);
    config->font_weight =
        parse_greeter_string(keyfile, "greeter-theme", "font-weight", "bold");
    config->font_style =
        parse_greeter_string(keyfile, "greeter-theme", "font-style", "normal");
    config->text_color =
        parse_greeter_color_key(keyfile,"greeter-theme", "text-color","#000000");
    config->error_color =
        parse_greeter_color_key(keyfile,"greeter-theme", "error-color","#FF0000");
    // Background
    config->background_image =
        g_key_file_get_string(keyfile, "greeter-theme", "background-image", NULL);
    if (config->background_image == NULL || strcmp(config->background_image, "") == 0) {
        config->background_image = (gchar *) "\"\"";
    }
    config->background_mode = 
        parse_greeter_string(keyfile,"greeter-theme","background-mode","auto");
    config->background_color =
        parse_greeter_color_key(keyfile,"greeter-theme" ,"background-color","#FFFFFF");
    // Window
    config->window_color =
        parse_greeter_color_key(keyfile,"greeter-theme" ,"window-color","#AAAAAA");
    config->border_color =
        parse_greeter_color_key(keyfile,"greeter-theme" ,"border-color","#BBBBBB");
    config->border_width = g_key_file_get_string(
        keyfile, "greeter-theme", "border-width", NULL);
    // Password
    config->password_color =
        parse_greeter_color_key(keyfile,"greeter-theme","password-color","#AAAAAA");
    config->password_background_color =
        parse_greeter_color_key(keyfile,"greeter-theme","password-background-color","#CCCCCC");
    gchar *temp_password_border_color = g_key_file_get_string(
        keyfile, "greeter-theme", "password-border-color", NULL);
    if (temp_password_border_color == NULL) {
        config->password_border_color = config->border_color;
    } else {
        free(temp_password_border_color);
        config->password_border_color =
            parse_greeter_color_key(keyfile,"greeter-theme","password-border-color","#BBBBBB");
    }
    config->password_border_width = parse_greeter_string(
        keyfile, "greeter-theme", "password-border-width", config->border_width);

    gint layout_spacing =
        g_key_file_get_integer(keyfile, "greeter-theme", "layout-space", NULL);
    if (layout_spacing < 0) {
        config->layout_spacing = (guint) (-1 * layout_spacing);
    } else {
        config->layout_spacing = (guint) layout_spacing;
    }

    /*Load modules from config*/
    gchar **group_names;
    gsize len_of_groups;

    group_names = g_key_file_get_groups(keyfile, &len_of_groups);

    //Calculate number's of modules
    config->len_of_modules = 0;
    config->modules = NULL;
    for(gsize i=0;i<len_of_groups;i++)
        if(g_str_has_prefix(group_names[i],"module/")) config->len_of_modules++;
    

    if(config->len_of_modules>0)
    {
        config->modules = malloc(sizeof(Module)*config->len_of_modules);
        if (config->modules==NULL){
            g_error("Could not allocate memory for Modules");
        }

        guint actual_module = 0;
        for(gsize i=0;i<len_of_groups;i++)
        {
            if(g_str_has_prefix(group_names[i],"module/")){
                config->modules[actual_module] = malloc(sizeof(Module));
                Module *module = config->modules[actual_module];
                //Name
                //7 - lenght of prefix
                module->name =  g_strndup(group_names[i]+7,sizeof(group_names));
                //Font
                module->font = parse_greeter_string(keyfile,group_names[i],"font",config->font);
                module->font_size = parse_greeter_string(keyfile,group_names[i],"font-size",config->font_size);
                module->font_weight = parse_greeter_string(keyfile,group_names[i], "font-weight",config->font_weight);
                module->font_style = parse_greeter_string(keyfile,group_names[i],"font-style",config->font_style);
                //TextColor
                module->text_color = parse_greeter_color_key2(keyfile,group_names[i],"text-color",config->text_color);
                //Window
                module->window_color = parse_greeter_color_key2(keyfile,group_names[i],"window-color",config->window_color);
                module->border_color = parse_greeter_color_key2(keyfile,group_names[i],"border-color",config->border_color);
                module->border_width = parse_greeter_string(keyfile,group_names[i],"border-width",NULL);
                module->layout_spacing = g_key_file_get_integer(keyfile,group_names[i],"layout-space",NULL);
                //Position
                gchar *position_x = parse_greeter_string(keyfile,group_names[i],"position-x","left");
                module->position_x = 0;
                if(strcmp(position_x,"right")==0)
                    module->position_x = 1;
                else if(strcmp(position_x,"center")==0)
                    module->position_x = 2;

                gchar *position_y = parse_greeter_string(keyfile,group_names[i],"position-y","top");
                module->position_y = 0;
                if(strcmp(position_y,"bottom")==0)
                    module->position_y = 1;
                else if(strcmp(position_y,"center")==0)
                    module->position_y = 2;
                //Offset
                module->offset_top = parse_greeter_string(keyfile,group_names[i],"offset-top","0px");
                module->offset_right = parse_greeter_string(keyfile,group_names[i],"offset-right","0px");
                module->offset_bottom = parse_greeter_string(keyfile,group_names[i],"offset-bottom","0px");
                module->offset_left = parse_greeter_string(keyfile,group_names[i],"offset-left","0px");
                //Text
                module->text = parse_greeter_string(keyfile,group_names[i],"text","");
                module->exec = parse_greeter_string(keyfile,group_names[i],"exec",NULL);
                module->click_exec = parse_greeter_string(keyfile,group_names[i],"click-exec",NULL);
                module->refresh = 0;
                module->refresh = (guint) g_key_file_get_uint64(keyfile,group_names[i],"refresh",NULL);
                actual_module++;
            }
        }
    }

    g_key_file_free(keyfile);

    return config;
}


/* Cleanup any memory allocated for the Config */
void destroy_config(Config *config)
{
    free(config->login_user);
    free(config->font);
    free(config->font_size);
    free(config->font_weight);
    free(config->font_style);
    free(config->text_color);
    free(config->error_color);
    free(config->background_image);
    free(config->background_mode);
    free(config->background_color);
    free(config->window_color);
    free(config->border_color);
    free(config->border_width);
    free(config->password_label_text);
    free(config->invalid_password_text);
    free(config->password_color);
    free(config->password_background_color);
    free(config->password_border_color);
    free(config->password_border_width);

    for(guint i=0;i<config->len_of_modules;i++)
        destroy_module(config->modules[i]);

    free(config);
}


/* Parse a string from the config file, returning the fallback value if the key
 * is not present in the group.
 */
static gchar *parse_greeter_string(GKeyFile *keyfile, const char *group_name,
                                   const char *key_name, const gchar *fallback)
{
    gchar *parsed_string = g_key_file_get_string(keyfile, group_name, key_name, NULL);
    if (parsed_string == NULL) {
        return (gchar *) fallback;
    } else {
        return parsed_string;
    }
}

/* Parse an integer from the config file, returning the fallback value if the
 * key is not present in the group, or if the value is not an integer.
 */
static gint parse_greeter_integer(GKeyFile *keyfile, const char *group_name,
                                  const char *key_name, const gint fallback)
{
    GError *parse_error = NULL;
    gint parse_result = g_key_file_get_integer(
        keyfile, group_name,key_name,&parse_error);
    if (parse_error != NULL) {
        if (parse_error->code == G_KEY_FILE_ERROR_INVALID_VALUE) {
            // Read the value as a string so we can log it
            gchar *value = g_key_file_get_string(
                keyfile, group_name, key_name, NULL);
            g_warning("Invalid integer for %s.%s: `%s`",
                      group_name, key_name, value);
            free(value);
        }
        g_error_free(parse_error);
        return fallback;
    }
    return parse_result;
}

/* Parse a greeter-colors group key into a newly-allocated GdkRGBA value */
static GdkRGBA *parse_greeter_color_key(GKeyFile *keyfile,const char *group_name,const char *key_name,
                                        const gchar *fallback)
{
    gchar *color_string = g_key_file_get_string(
        keyfile,group_name, key_name, NULL);
    if (color_string==NULL) {
        if(fallback==NULL) return NULL;
        color_string = (gchar*) fallback;
    }else if (strcmp(color_string, "transparent")==0) {
        //Use transparency
        strcpy(color_string,"rgba(0,0,0,0)");
    }else if (strstr(color_string, "#") != NULL) {
        // Remove quotations from hex color strings
        remove_char(color_string, '"');
        remove_char(color_string, '\'');
    }

    GdkRGBA *color = malloc(sizeof(GdkRGBA));

    gboolean color_was_parsed = gdk_rgba_parse(color, color_string);
    if (!color_was_parsed) {
        g_critical("Could not parse the '%s' setting: %s",
                   key_name, color_string);
    }

    return color;
}
static GdkRGBA *parse_greeter_color_key2(GKeyFile *keyfile,const char *group_name,
                                  const char *key_name,const GdkRGBA *fallback)
{
    GdkRGBA *color = parse_greeter_color_key(keyfile,group_name,key_name,NULL);
    if(color==NULL){
        free(color);
        return (GdkRGBA*) fallback;
    }
    return color;
}
/* Parse a greeter-hotkeys key into the GDKkeyval of it's first character */
static guint parse_greeter_hotkey_keyval(GKeyFile *keyfile, const char *key_name)
{
    gchar *key = g_key_file_get_string(
        keyfile, "greeter-hotkeys", key_name, NULL);

    if (strcmp(key, "") == 0) {
        g_error("Configuration contains empty key for '%s'\n", key_name);
    }

    return gdk_unicode_to_keyval((guint) key[0]);
}

/* Parse the password input alignment, properly handling RTL layouts.
 *
 * Note that the gboolean returned by this function is meant to be used with
 * the `gtk_entry_set_alignment` function.
 */
static gboolean parse_greeter_password_alignment(GKeyFile *keyfile) {
    gboolean initial_alignment;

    gchar *password_alignment_text = g_key_file_get_string(
        keyfile, "greeter", "password-alignment", NULL);
    if (password_alignment_text == NULL) {
        initial_alignment = 1;
    } else {
        if (strcmp(g_strchomp(password_alignment_text), "left") == 0) {
            initial_alignment = 0;
        } else {
            initial_alignment = 1;
        }
        free(password_alignment_text);
    }

    // The left/right values are switched for RTL layouts.
    if (is_rtl_keymap_layout()) {
        if (initial_alignment == 0) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return initial_alignment;
    }
}

/* Determine if the default Display's Keymap is in the Right-to-Left direction
 */
static gboolean is_rtl_keymap_layout(void) {
    GdkDisplay *display = gdk_display_get_default();
    if (display == NULL) {
        return FALSE;
    }
    GdkKeymap *keymap = gdk_keymap_get_for_display(display);
    PangoDirection text_direction = gdk_keymap_get_direction(keymap);
    return text_direction == PANGO_DIRECTION_RTL;
}
