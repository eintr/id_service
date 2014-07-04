/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/** \endcond */

/** \file id_api.c
	This is a test module, it reflects every byte of the socket.
*/

#include <room_service.h>
#include "mod_config.h"
#include "id_msg_header.h"
#include "id_msg_body.pb-c.h"
#include "id_msg_proc.h"

#define	BUFSIZE	4096
#define	LOGSIZE	1024

struct id_api_memory_st {
	conn_tcp_t	*conn_api;
	int state;
	struct id_msg_buf_st *msgbuf;

	char logbuf[LOGSIZE];
};

static imp_soul_t id_api_soul;

enum state_en {
	ST_RCV_MSG=1,
	ST_DEMUX_MSG,
	ST_FORK,
	ST_Ex,
	ST_TERM
};

static void *id_api_new(imp_t *imp)
{
	struct id_api_memory_st *m=imp->memory;

	m->state = ST_RCV_MSG;
	m->msgbuf = malloc(sizeof(struct id_msg_buf_st));
	m->msgbuf->buf = NULL;
	return NULL;
}

static int id_api_delete(imp_t *imp)
{
	struct id_api_memory_st *memory = imp->memory;

	if (memory->conn_api) {
		conn_tcp_close_nb(memory->conn_api);
	}
	if (memory->msgbuf) {
		if (memory->msgbuf->buf) {
			id_msg_buf_free(memory->msgbuf);
		}
		free(memory->msgbuf);
		memory->msgbuf = NULL;
	}
	free(memory);
	imp->memory = NULL;
	return 0;
}

static enum enum_driver_retcode id_api_driver_rcv_msg(imp_t *imp)
{
	struct id_api_memory_st *mem = imp->memory;
    struct id_msg_header_st *hdr;
    ssize_t len;
    int ret;

	if (imp->event_mask & EV_MASK_TIMEOUT) {
		fprintf(stderr, "%s: Recive CMD_REQ_PUSH timed out.\n", __FUNCTION__);
		mem->state = ST_Ex;
		return TO_RUN;
	}
	ret = id_msg_recv_generic(mem->conn_api, mem->msgbuf, 1, -1);
	switch (ret) {
		case RCV_OVER:
			fprintf(stderr, "%s: Got msg.\n", __FUNCTION__);
            mem->state = ST_DEMUX_MSG;
            return TO_RUN;
            break;
        case RCV_WAIT:
            imp_set_ioev(imp, mem->conn_api->sd, EPOLLIN|EPOLLRDHUP);
			imp_set_timer(imp, id_module_config.rcv_api_timeout_ms);
            return TO_WAIT_IO;
            break;
        case RCV_CONTINUE:
            return TO_RUN;
            break;
        case RCV_ERROR:
            mem->state = ST_Ex;
            return TO_RUN;
            break;
        default:
            fprintf(stderr, "%s: This must be a bug! Core dump!\n", __FUNCTION__);
            abort();
            break;
    }
    return TO_RUN;
}

static enum enum_driver_retcode id_api_driver_demux_msg(imp_t *imp)
{
	struct id_api_memory_st *mem = imp->memory;

	switch (mem->msgbuf->hdr->command) {
		case CMD_REQ_ID_CREATE:
			fprintf(stderr, "%s: Got ID_CREATE msg.\n", __FUNCTION__);
			msg_id_create_summon(mem->conn_api, mem->msgbuf);
			mem->conn_api = NULL;
			mem->msgbuf = NULL;
			break;
		case CMD_REQ_ID_LIST:
			//msg_id_list_summon(mem->conn_api, mem->msgbuf);
			mem->conn_api = NULL;
			mem->msgbuf = NULL;
			break;
		case CMD_REQ_ID_GET:
			fprintf(stderr, "%s: Got ID_GET msg.\n", __FUNCTION__);
			msg_id_get_summon(mem->conn_api, mem->msgbuf);
			mem->conn_api = NULL;
			mem->msgbuf = NULL;
			break;
		default:
			mylog(L_INFO, "%s: msg command==%d, which I don't know how to deal with. Drop it!\n", __FUNCTION__, mem->msgbuf->hdr->command);
			break;
	}

	mem->state = ST_TERM;
	return TO_RUN;
}

static enum enum_driver_retcode id_api_driver_ex(imp_t *imp)
{
	struct id_api_memory_st *mem = imp->memory;

	mylog(L_INFO, "imp[%d]: Exception occured.", imp->id);
	mem->state = ST_TERM;
	return TO_RUN;
}

static enum enum_driver_retcode id_api_driver(imp_t *imp)
{
	struct id_api_memory_st *mem = imp->memory;
	struct epoll_event ev;
	ssize_t ret;

	mem = imp->memory;
	switch (mem->state) {
		case ST_RCV_MSG:
			return id_api_driver_rcv_msg(imp);
			break;
		case ST_DEMUX_MSG:
			return id_api_driver_demux_msg(imp);
			break;
		case ST_Ex:
			return id_api_driver_ex(imp);
			break;
		case ST_TERM:
			return TO_TERM;
			break;
		default:
			break;
	}
	return TO_RUN;
}

static void *id_api_serialize(imp_t *unused)
{
	fprintf(stderr, "%s is running.\n", __FUNCTION__);
	return NULL;
}

imp_t *id_api_summon(conn_tcp_t *conn)
{
	imp_t *server;
	struct id_api_memory_st *mem;

	mem = calloc(sizeof(*mem), 1);
	mem->conn_api = conn;
	server = imp_summon(mem, &id_api_soul);
	if (server==NULL) {
		mylog(L_ERR, "Failed to summon a new push_service.");
		free(mem);
		return NULL;
	}
	return server;
}

static imp_soul_t id_api_soul = {
	.fsm_new = id_api_new,
	.fsm_delete = id_api_delete,
	.fsm_driver = id_api_driver,
	.fsm_serialize = id_api_serialize,
};

