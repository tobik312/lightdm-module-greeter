#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "module.h"

void get_script_output(Module *module)
{
    char *buff = malloc(sizeof(char)*128);
    char *result = NULL;
    size_t length = 0;
    FILE *process_pipe = NULL;

    process_pipe = popen(module->exec,"r");

    if(process_pipe==NULL){
        pclose(process_pipe);
        g_error("Cannot run module %s script.",module->name);
    }
    
    //Read into buffer and put it into result;
    size_t data_size;
    while ((data_size = fread(buff,1,128,process_pipe))>0){
        result = realloc(result,length+data_size);
        memcpy(result+length,buff,data_size);
        length += data_size;
    }
    free(buff);

    //Remove last new line char if exist.
    if(result[length-1]==10){
        result = realloc(result,--length);
        result[length] = 0;
    }

    module->text = result;
    pclose(process_pipe);
}

void run_script(Module *module)
{
    FILE *process;
    process = popen(module->click_exec,"r");

    if(process==NULL){
        pclose(process);
        g_error("Cannot run module %s script on click handle",module->name);
    }

    pclose(process);
}

void destroy_module(Module *module)
{
    free(module->name);
    free(module->font);
    free(module->font_size);
    free(module->font_weight);
    free(module->font_style);
    free(module->text_color);
    free(module->window_color);
    free(module->border_color);
    free(module->border_width);
    free(module->offset_top);
    free(module->offset_right);
    free(module->offset_bottom);
    free(module->offset_left);
    free(module->text);
    free(module->exec);
    free(module->click_exec);
    free(module);
}
