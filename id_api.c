/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
/** \endcond */

/** \file id_api.c
	This is a test module, it reflects every byte of the socket.
*/

//#include <room_service.h>
#include "mod_config.h"
#include "id_msg_header.h"
#include "id_msg_body.pb-c.h"
#include "id_msg_proc.h"

#define	BUFSIZE	4096
#define	LOGSIZE	1024

struct id_api_memory_st {
	int sd;
	int state;
	struct id_msg_buf_st *msgbuf;

	char logbuf[LOGSIZE];
};

enum state_en {
	ST_RCV_MSG=1,
	ST_DEMUX_MSG,
	ST_FORK,
	ST_Ex,
	ST_TERM
};

static void *id_api_new(struct id_api_memory_st *m)
{
	m->state = ST_RCV_MSG;
	m->msgbuf = malloc(sizeof(struct id_msg_buf_st));
	m->msgbuf->buf = NULL;
	return NULL;
}

static int id_api_delete(struct id_api_memory_st *memory)
{
	if (memory->sd>=0) {
		close(memory->sd);
		memory->sd = -1;
	}
	if (memory->msgbuf) {
		if (memory->msgbuf->buf) {
			id_msg_buf_free(memory->msgbuf);
		}
		free(memory->msgbuf);
		memory->msgbuf = NULL;
	}
	free(memory);
	return 0;
}

static int id_api_driver_rcv_msg(struct id_api_memory_st *mem)
{
    struct id_msg_header_st *hdr;
    ssize_t len;
    int ret;

	ret = id_msg_recv_generic(mem->sd, mem->msgbuf, 1, -1);
	switch (ret) {
		case 0:
			//fprintf(stderr, "%s: Got msg.\n", __FUNCTION__);
            mem->state = ST_DEMUX_MSG;
            return 0;
            break;
        case -1:
            mem->state = ST_Ex;
            return -1;
            break;
        default:
            //fprintf(stderr, "%s: This must be a bug! Core dump!\n", __FUNCTION__);
            abort();
            break;
    }
}

static int id_api_driver_demux_msg(struct id_api_memory_st *mem)
{
	switch (mem->msgbuf->hdr->command) {
		case CMD_REQ_ID_CREATE:
			//fprintf(stderr, "%s: Got ID_CREATE msg.\n", __FUNCTION__);
			msg_id_create_enter(mem->sd, mem->msgbuf);
			break;
		case CMD_REQ_ID_LIST:
			msg_id_list_enter(mem->sd, mem->msgbuf);
			break;
		case CMD_REQ_ID_GET:
			//fprintf(stderr, "%s: Got ID_GET msg.\n", __FUNCTION__);
			msg_id_get_enter(mem->sd, mem->msgbuf);
			break;
		default:
			syslog(LOG_INFO, "%s: msg command==%d, which I don't know how to deal with. Drop it!\n", __FUNCTION__, mem->msgbuf->hdr->command);
			break;
	}

	mem->state = ST_TERM;
	return 0;
}

static int id_api_driver_ex(struct id_api_memory_st *mem)
{
	syslog(LOG_INFO, "Conn: Exception occured.");
	mem->state = ST_TERM;
	return 0;
}

static int id_api_driver(struct id_api_memory_st *mem)
{
	switch (mem->state) {
		case ST_RCV_MSG:
			return id_api_driver_rcv_msg(mem);
			break;
		case ST_DEMUX_MSG:
			return id_api_driver_demux_msg(mem);
			break;
		case ST_Ex:
			return id_api_driver_ex(mem);
			break;
		case ST_TERM:
			return 0;
			break;
		default:
			break;
	}
	return 0;
}

static void *id_api_serialize(struct id_api_memory_st *mem)
{
	//fprintf(stderr, "%s is running.\n", __FUNCTION__);
	return NULL;
}

int id_api_enter(int client_sd)
{
	struct id_api_memory_st *mem;

	mem = calloc(sizeof(*mem), 1);
	mem->sd = client_sd;
	id_api_new(mem);
	while (mem->state != ST_TERM) {
		id_api_driver(mem);
	}
	id_api_delete(mem);
	return 0;
}

