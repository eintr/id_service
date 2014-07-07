#ifndef MODULE_CONFIG_H
#define MODULE_CONFIG_H

#define	CONFIG_FILE_PATTERN	"*.id"

extern struct id_module_config_st {
	struct addrinfo *id_api_addr;
	int	snd_api_timeout_ms;
	int	rcv_api_timeout_ms;

	char *id_config_dir;
} id_module_config;

#endif

