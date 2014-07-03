#ifndef MODULE_CONFIG_H
#define MODULE_CONFIG_H

extern struct id_module_config_st {
	struct addrinfo *id_api_addr;
	int	snd_api_timeout_ms;
	int	rcv_api_timeout_ms;

	char *id_filename;
} id_module_config;

#endif

