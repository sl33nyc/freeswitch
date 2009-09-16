/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2009, Mathieu Parent <math.parent@gmail.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Mathieu Parent <math.parent@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Mathieu Parent <math.parent@gmail.com>
 *
 * mod_tts_commandline.c -- System command ASR TTS Interface
 *
 */

#include <unistd.h>
#include <switch.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_tts_commandline_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_tts_commandline_shutdown);
SWITCH_MODULE_DEFINITION(mod_tts_commandline, mod_tts_commandline_load, mod_tts_commandline_shutdown, NULL);

static switch_event_node_t *NODE = NULL;

static struct {
	char *command;
} globals;

SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_command, globals.command);

struct tts_commandline_data {
    switch_file_handle_t fh;
	char *voice_name;
	int rate;
	char file[512];
};

typedef struct tts_commandline_data tts_commandline_t;

static int load_tts_commandline_config(void)
{
	char *cf = "tts_commandline.conf";
	switch_xml_t cfg, xml, settings, param;

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", cf);
	} else {
		if ((settings = switch_xml_child(cfg, "settings"))) {
			for (param = switch_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcmp(var, "command")) {
					set_global_command(val);
				}
			}
		}
		switch_xml_free(xml);
	}

	if (switch_strlen_zero(globals.command)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No command set, please edit %s\n", cf);   
	}

	return SWITCH_STATUS_SUCCESS;
}

static void event_handler(switch_event_t *event)
{
	if (event->event_id == SWITCH_EVENT_RELOADXML) {
		if (load_tts_commandline_config() != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to reload config file\n");
		}
	}
}

static switch_status_t tts_commandline_speech_open(switch_speech_handle_t *sh, const char *voice_name, int rate, switch_speech_flag_t *flags)
{
	tts_commandline_t *info = switch_core_alloc(sh->memory_pool, sizeof(*info));

	info->voice_name = switch_core_strdup(sh->memory_pool, voice_name);
    info->rate = rate;

	switch_snprintf(info->file, sizeof(info->file), "%s%s.wav", SWITCH_GLOBAL_dirs.temp_dir, "tts_commandline");
    
    sh->private_info = info;
    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t tts_commandline_speech_close(switch_speech_handle_t *sh, switch_speech_flag_t *flags)
{
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);
    
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t tts_commandline_speech_feed_tts(switch_speech_handle_t *sh, char *text, switch_speech_flag_t *flags)
{
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);

    char *message;
    char *rate;
	message = switch_core_strdup(sh->memory_pool, globals.command);
    message = switch_string_replace(message, "${text}", text);
    message = switch_string_replace(message, "${voice}", info->voice_name);
    rate = switch_core_sprintf(sh->memory_pool, "%d", info->rate);
    message = switch_string_replace(message, "${rate}", rate);
    message = switch_string_replace(message, "${file}", info->file);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Executing: %s\n", message);
    
    if (switch_system(message, SWITCH_TRUE) < 0) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to execute command: %s\n", message);
       return SWITCH_STATUS_FALSE;
    }
	
    if (switch_core_file_open(&info->fh,
                            info->file,
                            0, //number_of_channels,
                            0, //samples_per_second,
                            SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, NULL) != SWITCH_STATUS_SUCCESS) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to open file: %s\n", info->file);
       return SWITCH_STATUS_FALSE;
    }
    
    sh->private_info = info;

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t tts_commandline_speech_read_tts(switch_speech_handle_t *sh, void *data, size_t *datalen, switch_speech_flag_t *flags)
{
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);
    
    size_t my_datalen = *datalen / 2;
    
    if (switch_core_file_read(&info->fh, data, &my_datalen) != SWITCH_STATUS_SUCCESS) {
        *datalen = my_datalen * 2;
        return SWITCH_STATUS_FALSE;
    }
    *datalen = my_datalen * 2;
    if(datalen == 0) {
        return SWITCH_STATUS_BREAK;
    } else {
        return SWITCH_STATUS_SUCCESS;
    }
}

static void tts_commandline_speech_flush_tts(switch_speech_handle_t *sh)
{
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);
    
    switch_core_file_close(&info->fh);
    if (unlink(info->file) != 0) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Sound file [%s] delete failed\n", info->file);
    }
}

static void tts_commandline_text_param_tts(switch_speech_handle_t *sh, char *param, const char *val)
{
	tts_commandline_t *info = (tts_commandline_t *) sh->private_info;
	assert(info != NULL);

	if (!strcasecmp(param, "voice")) {
        info->voice_name = switch_core_strdup(sh->memory_pool, val);
    }
}

static void tts_commandline_numeric_param_tts(switch_speech_handle_t *sh, char *param, int val)
{
}

static void tts_commandline_float_param_tts(switch_speech_handle_t *sh, char *param, double val)
{
}

SWITCH_MODULE_LOAD_FUNCTION(mod_tts_commandline_load)
{
	switch_speech_interface_t *speech_interface;
	
    load_tts_commandline_config();

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	if ((switch_event_bind_removable(modname, SWITCH_EVENT_RELOADXML, NULL, event_handler, NULL, &NODE) != SWITCH_STATUS_SUCCESS)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind our reloadxml handler!\n");
		/* Not such severe to prevent loading */
	}

	speech_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_SPEECH_INTERFACE);
	speech_interface->interface_name = "tts_commandline";
	speech_interface->speech_open = tts_commandline_speech_open;
	speech_interface->speech_close = tts_commandline_speech_close;
	speech_interface->speech_feed_tts = tts_commandline_speech_feed_tts;
	speech_interface->speech_read_tts = tts_commandline_speech_read_tts;
	speech_interface->speech_flush_tts = tts_commandline_speech_flush_tts;
	speech_interface->speech_text_param_tts = tts_commandline_text_param_tts;
	speech_interface->speech_numeric_param_tts = tts_commandline_numeric_param_tts;
	speech_interface->speech_float_param_tts = tts_commandline_float_param_tts;

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_tts_commandline_shutdown)
{
	switch_event_unbind(&NODE);

	switch_safe_free(globals.command);

	return SWITCH_STATUS_UNLOAD;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
