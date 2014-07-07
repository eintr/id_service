/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/** \endcond */

/** \file mod.c
	This is a test module, it reflects every byte of the socket.
*/

#include <conf.h>
#include <room_service.h>

#include "mod_config.h"
#include "id_api.h"
#include "id_pool.h"

struct listener_memory_st {
	listen_tcp_t	*listen;
};

static imp_soul_t listener_soul;

static imp_t *id_listener;

static int get_config(cJSON *conf)
{
	int ga_err;
	struct addrinfo hint;
	cJSON *value;
	char *Host, Port[12];
	char tmp_path[1024];

	hint.ai_flags = 0;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	value = cJSON_GetObjectItem(conf, "LocalAddr");
	if (value->type != cJSON_String) {
		mylog(L_INFO, "Module is configured with illegal LocalAddr.");
		return -1;
	} else {
		Host = value->valuestring;
	}

	value = cJSON_GetObjectItem(conf, "LocalPort");
	if (value->type != cJSON_Number) {
		mylog(L_INFO, "Module is configured with illegal LocalPort.");
		return -1;
	} else {
		sprintf(Port, "%d", value->valueint);
	}

	ga_err = getaddrinfo(Host, Port, &hint, &id_module_config.id_api_addr);
	if (ga_err) {
		mylog(L_INFO, "Module configured with illegal LocalAddr and LocalPort.");
		return -1;
	}

	value = cJSON_GetObjectItem(conf, "ID_Config_Dir");
	if (value==NULL) {
		strncpy(tmp_path, conf_get_working_dir(), 1024);
		strncat(tmp_path, "/conf", 1024);
		id_module_config.id_config_dir = strdup(tmp_path);
		mylog(L_INFO, "ID_Config_Dir unconfigured, using default:%s.", id_module_config.id_config_dir);
	} else if (value->type != cJSON_String) {
		mylog(L_INFO, "Module configured an illegal or missing .");
		return -1;
	} else {
		id_module_config.id_config_dir = strdup(value->valuestring);
	}

	value = cJSON_GetObjectItem(conf, "Recv_API_TimeOut_ms");
	if (value==NULL) {
		// Use default value.
	} else if (value->type != cJSON_Number) {
		mylog(L_INFO, "Module is configured with illegal Recv_API_TimeOut_ms.");
		return -1;
	} else {
		id_module_config.rcv_api_timeout_ms = value->valueint;
	}

	value = cJSON_GetObjectItem(conf, "Send_API_TimeOut_ms");
	if (value==NULL) {
		// Use default value.
	} else if (value->type != cJSON_Number) {
		mylog(L_INFO, "Module is configured with illegal Send_API_TimeOut_ms.");
		return -1;
	} else {
		id_module_config.snd_api_timeout_ms = value->valueint;
	}

	return 0;
}

static int mod_init(cJSON *conf)
{
	struct listener_memory_st *l_mem;

	if (get_config(conf)!=0) {
		return -1;
	}

	if (id_pool_init(id_module_config.id_config_dir)!=0) {
		mylog(L_ERR, "id_pool_init() failed.");
		return -1;
	}

	l_mem = calloc(sizeof(struct listener_memory_st), 1);
	l_mem->listen = conn_tcp_listen_create(id_module_config.id_api_addr, NULL);
	if (l_mem->listen == NULL) {
		return -1;
	}

	id_listener = imp_summon(l_mem, &listener_soul);
	if (id_listener==NULL) {
		mylog(L_ERR, "imp_summon() Failed!\n");
		return -1;
	}
	mylog(L_DEBUG, MOD_NAME ": Module init OK.");
	return 0;
}

static int mod_destroy(void)
{
	free(id_module_config.id_config_dir);
	id_module_config.id_config_dir = NULL;
	if (id_listener) {
		imp_kill(id_listener);
	}
	return 0;
}

static void mod_maint(void)
{
}

static cJSON *mod_serialize(void)
{
	return NULL;
}

static void *listener_new(imp_t *p)
{
	struct listener_memory_st *lmem=p->memory;

	if (imp_set_ioev(p, lmem->listen->sd, EPOLLIN|EPOLLRDHUP)<0) {
		mylog(L_ERR, "Set listen socket epoll event FAILED: %m\n");
	}

	return NULL;
}

static int listener_delete(imp_t *p)
{
	struct listener_memory_st *mem=p->memory;

	free(mem);
	return 0;
}

static enum enum_driver_retcode listener_driver(imp_t *p)
{
	struct listener_memory_st *lmem=p->memory;
	struct server_memory_st *emem;
	conn_tcp_t *conn;
	imp_t *server;
	struct epoll_event ev;

	// TODO: Disable timer here.

	while (conn_tcp_accept_nb(&conn, lmem->listen, NULL)==0) {
		server = id_api_summon(conn);
		if (server==NULL) {
			mylog(L_ERR, "Failed to summon a new api imp.");
			conn_tcp_close_nb(conn);
			continue;
		}
	}

	return TO_WAIT_IO;
}

static void *listener_serialize(imp_t *imp)
{
	struct listener_memory_st *m=imp->memory;

	return NULL;
}

static imp_soul_t listener_soul = {
	.fsm_new = listener_new,
	.fsm_delete = listener_delete,
	.fsm_driver = listener_driver,
	.fsm_serialize = listener_serialize,
};

module_interface_t api_interface = 
{
	.mod_initializer = mod_init,
	.mod_destroier = mod_destroy,
	.mod_maintainer = mod_maint,
	.mod_serialize = mod_serialize,
};

