/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005/2006, Anthony Minessale II <anthmct@yahoo.com>
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
 * Anthony Minessale II <anthmct@yahoo.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthmct@yahoo.com>
 * Michael Jerris <mike@jerris.com>
 * Paul D. Tinsley <pdt at jackhammer.org>
 *
 *
 * switch_core_directory.c -- Main Core Library (Directory Interface)
 *
 */
#include <switch.h>
#include "private/switch_core.h"

SWITCH_DECLARE(switch_status_t) switch_core_directory_open(switch_directory_handle_t *dh,
														   char *module_name, char *source, char *dsn, char *passwd, switch_memory_pool_t *pool)
{
	switch_status_t status;

	if ((dh->directory_interface = switch_loadable_module_get_directory_interface(module_name)) == 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "invalid directory module [%s]!\n", module_name);
		return SWITCH_STATUS_GENERR;
	}

	if (pool) {
		dh->memory_pool = pool;
	} else {
		if ((status = switch_core_new_memory_pool(&dh->memory_pool)) != SWITCH_STATUS_SUCCESS) {
			return status;
		}
		switch_set_flag(dh, SWITCH_DIRECTORY_FLAG_FREE_POOL);
	}

	return dh->directory_interface->directory_open(dh, source, dsn, passwd);
}

SWITCH_DECLARE(switch_status_t) switch_core_directory_query(switch_directory_handle_t *dh, char *base, char *query)
{
	return dh->directory_interface->directory_query(dh, base, query);
}

SWITCH_DECLARE(switch_status_t) switch_core_directory_next(switch_directory_handle_t *dh)
{
	return dh->directory_interface->directory_next(dh);
}

SWITCH_DECLARE(switch_status_t) switch_core_directory_next_pair(switch_directory_handle_t *dh, char **var, char **val)
{
	return dh->directory_interface->directory_next_pair(dh, var, val);
}

SWITCH_DECLARE(switch_status_t) switch_core_directory_close(switch_directory_handle_t *dh)
{
	return dh->directory_interface->directory_close(dh);
}

SWITCH_DECLARE(switch_status_t) switch_core_speech_open(switch_speech_handle_t *sh,
														char *module_name,
														char *voice_name, unsigned int rate, switch_speech_flag_t *flags, switch_memory_pool_t *pool)
{
	switch_status_t status;

	if ((sh->speech_interface = switch_loadable_module_get_speech_interface(module_name)) == 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "invalid speech module [%s]!\n", module_name);
		return SWITCH_STATUS_GENERR;
	}

	switch_copy_string(sh->engine, module_name, sizeof(sh->engine));
	sh->flags = *flags;
	if (pool) {
		sh->memory_pool = pool;
	} else {
		if ((status = switch_core_new_memory_pool(&sh->memory_pool)) != SWITCH_STATUS_SUCCESS) {
			return status;
		}
		switch_set_flag(sh, SWITCH_SPEECH_FLAG_FREE_POOL);
	}
	sh->rate = rate;
	sh->name = switch_core_strdup(pool, module_name);
	return sh->speech_interface->speech_open(sh, voice_name, rate, flags);
}
