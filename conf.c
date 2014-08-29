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
	cJSON_AddItemToObject(conf, "Daemon", cJSON_CreateFalse());
	cJSON_AddNumberToObject(conf, "MonitorPort", DEFAULT_MONITOR_PORT);
	cJSON_AddNumberToObject(conf, "ConcurrentMax", DEFAULT_CONCURRENT_MAX);
	cJSON_AddStringToObject(conf, "WorkingDir", INSTALL_PREFIX);
	cJSON_AddStringToObject(conf, "IDConfigDir", DEFAULT_IDCONFIG_PATH);
	cJSON_AddNumberToObject(conf, "RestartForward_mil", DEFAULT_FORWARD_mil);
	cJSON_AddNumberToObject(conf, "Recv_TimeOut_ms", 1000);
	cJSON_AddNumberToObject(conf, "Send_TimeOut_ms", 1000);

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
		if (conf_check_legal(conf)==0) {
			global_conf = conf;
			ret = 0;
		} else {
			ret = -1;
		}
	} else {
		global_conf = conf_create_default_config();
		ret = 0;
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
		return tmp->valueint;
	}
	return DEFAULT_CONCURRENT_MAX;
}

int conf_get_log_level(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "LogLevel");
	if (tmp) {
		return tmp->valueint;
	}
	return 6;
}

int conf_get_daemon(void)
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

char *conf_get_working_dir(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "WorkingDir");
	if (tmp) {
		return tmp->valuestring;
	}
	return DEFAULT_WORK_DIR;
}

char *conf_get_id_config_dir(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "IDConfigDir");
	if (tmp) {
		return tmp->valuestring;
	}
	return DEFAULT_IDCONFIG_PATH;
}

char *conf_get_local_addr(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "LocalAddr");
	if (tmp) {
		return tmp->valuestring;
	}
	return "0.0.0.0";
}

int conf_get_local_port(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "LocalPort");
	if (tmp) {
		return tmp->valueint;
	}
	return 19999;
}

int conf_get_restart_forward(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "RestartForward_mil");
	if (tmp) {
		return tmp->valueint;
	}
	return 5;
}

int conf_get_r_timeout(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "Recv_TimeOut_ms");
	if (tmp) {
		return tmp->valueint;
	}
	return 5;
}

int conf_get_s_timeout(void)
{
	cJSON *tmp;
	tmp = cJSON_GetObjectItem(global_conf, "Send_TimeOut_ms");
	if (tmp) {
		return tmp->valueint;
	}
	return 5;
}

static int conf_check_legal(cJSON *conf)
{
	int LogLevel;
	char *id_dir;
	cJSON *Modules;

	LogLevel = conf_get_log_level();
	if (LogLevel < 0)
		return -1;

	return 0;
}

