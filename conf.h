#ifndef AA_CONF_H
#define AA_CONF_H

/**
	\file conf.h
		\brief Configure processing. And some global defines, limitations, declarations ...
*/
/** \cond 0 */
#include "cJSON.h"
/** \endcond */

/**
	\def APPVERSION
		\brief Application version.
*/
#define APPVERSION					"1.0"

#define DEFAULT_DAEMON				1
#define	DEFAULT_CONCURRENT_MAX		20000
#define DEFAULT_MONITOR_PORT		19990
#define DEFAULT_DEBUG_VALUE 		1
#define	DEFAULT_WORK_DIR			INSTALL_PREFIX
#define DEFAULT_CONFPATH			CONFDIR"/"APPNAME".conf"
#define DEFAULT_IDCONFIG_PATH		DEFAULT_CONFPATH
#define DEFAULT_FORWARD_mil			5

#define PORT_MIN 1025
#define PORT_MAX 65534

#define BACKLOG_NUM 500

/**
	\fn int conf_new(const char *filename)
		\brief Init the internal global configure struct and load the configure file.
		\param filename ASCIIZ string of configure file name.
		\warning Not thread safe!
*/
int conf_new(const char *filename);

/**
	\fn	int conf_delete(void)
		\brief Destroy the internal global configure struct.
*/
int conf_delete();

/**
	\fn int conf_reload(const char *filename)
		\brief Reload the configure file and flush the internal global configure struct.
		\param filename ASCIIZ string of configure file name.
*/
int conf_reload(const char *filename);

/**
	\fn int conf_load_json(cJSON *conf)
		\brief Parse a cJSON object and flush the internal global configure struct.
		\param conf A pointer points to a parsed configure file.
*/
int conf_load_json(cJSON *conf);

//int conf_get_monitor_port();
int conf_get_concurrent_max();
int conf_get_log_level();
int conf_get_daemon();
char *conf_get_working_dir(void);
char *conf_get_id_config_dir(void);

char *conf_get_local_addr(void);
int conf_get_local_port(void);
int conf_get_restart_forward(void);
int conf_get_r_timeout(void);
int conf_get_s_timeout(void);

#endif

