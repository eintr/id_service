/** \cond 0 */
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include "cJSON.h"
/** \endcond */


#include "conf.h"

static cJSON *global_conf;

static int conf_check_legal(cJSON *config);

static cJSON *conf_create_default_config(void)
{
	cJSON *conf;

	conf = cJSON_CreateObject();

	if (DEFAULT_DEBUG_VALUE) {
		cJSON_AddItemToObject(conf, "Debug", cJSON_CreateTrue());
	} else {
		cJSON_AddItemToObject(conf, "Debug", cJSON_CreateFalse());
	}
	cJSON_AddNumberToObject(conf, "MonitorPort", DEFAULT_MONITOR_PORT);
	cJSON_AddNumberToObject(conf, "ConcurrentMax", DEFAULT_CONCURRENT_MAX);
	cJSON_AddStringToObject(conf, "WorkingDir", INSTALL_PREFIX);
	cJSON_AddStringToObject(conf, "IDConfigDir", DEFAULT_IDCONFIG_PATH);

	return conf;
}

int conf_new(const char *filename)
{
	FILE *fp;
	cJSON *conf;
	int ret = 0;

	if (filename != NULL) {
		fp = fopen(filename, "r");
		if (fp == NULL) {
			//log error
			fprintf(stderr, "fopen(): %m");
			return -1;
		} 
		conf = cJSON_fParse(fp);
		fclose(fp);
		ret = conf_load_json(conf);
	} else {
		global_conf = conf_create_default_config();
	}

	return ret;
}

int conf_delete()
{
	if (global_conf) {
		cJSON_Delete(global_conf);
		global_conf = NULL;
		return 0;
	}
	return -1;
}


int conf_reload(const char *filename)
{
	FILE *fp;
	cJSON *conf;
	int ret;

	if (filename == NULL) {
		return -1;
	}
	fp = fopen(filename, "r");
	if (fp == NULL) {
		//log error
		return -1;
	}

	conf = cJSON_fParse(fp);
	fclose(fp);

	ret = conf_load_json(conf);
	if (ret < 0) {
		cJSON_Delete(conf);
	}

	return ret;
}

int conf_load_json(cJSON *conf)
{
	if (conf == NULL) {
		//log error;
		return -1;
	}
	if (conf_check_legal(conf) != 0) {
		//log error
		return -1;
	}

	if (global_conf) {
		cJSON_Delete(global_conf);
	}

	global_conf = conf;

	return 0;
}

int conf_get_concurrent_max(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "ConcurrentMax");
	if (tmp) {
		if (tmp->type!=cJSON_Number) {
			return -1;
		}
		return tmp->valueint;
	}
	return DEFAULT_CONCURRENT_MAX;
}

int conf_get_log_level(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "LogLevel");
	if (tmp) {
		if (tmp->type!=cJSON_Number) {
			return -1;
		}
		return tmp->valueint;
	}
	return 6;
}

int conf_get_daemon_internal(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "Daemon");
	if (tmp) {
		if (tmp->type == cJSON_True) {
			return 1;
		} else {
			return 0;
		}
	}
	return DEFAULT_DAEMON;
}

int conf_get_workers(int n)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "Workers");
	if (tmp) {
		if (tmp->type == cJSON_Number) {
			if (tmp->valueint==0) {
				return n;
			} else {
				return tmp->valueint;
			}
		} else {
			return n;
		}
	}
	return n;
}

int conf_get_monitor_port(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "MonitorPort");
	if (tmp) {
		if (tmp->type!=cJSON_Number) {
			return -1;
		}
		return tmp->valueint;
	}
	return DEFAULT_MONITOR_PORT;
}

char *conf_get_working_dir(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "WorkingDir");
	if (tmp) {
		if (tmp->type!=cJSON_String) {
			return NULL;
		}
		return tmp->valuestring;
	}
	return DEFAULT_WORK_DIR;
}

char *conf_get_id_config_dir(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "IDConfigDir");
	if (tmp) {
		if (tmp->type!=cJSON_String) {
			return NULL;
		}
		return tmp->valuestring;
	}
	return "./";
}

static int conf_check_legal(cJSON *conf)
{
	int LogLevel;
	char *id_dir;
	cJSON *Modules;

	LogLevel = conf_get_log_level_internal(conf);
	if (LogLevel < 0)
		return -1;

	return 0;
}

