#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

#include "util_atomic.h"

#include "mod_config.h"
#include "id_msg_body.pb-c.h"
#include "id_msg_header.h"
#include "id_msg_proc.h"
#include "id_pool.h"

#define	NAMESIZE	1024

struct mem_st {
	int state;
	int sd;
	struct id_msg_buf_st *rcvbuf;

	MsgIDListReq *req;

	MsgIDListRsp rsp;

	struct id_msg_buf_st sndbuf;
};

enum {
	ST_PARSE_MSG=1,
	ST_PREPARE_RSP,
	ST_SEND_RSP,
	ST_Ex,
	ST_TERM
};

static void *imp_new(struct mem_st *mem)
{
	mem->state = ST_PARSE_MSG;
	mem->sndbuf.buf = NULL;
	msg_idlist_rsp__init(&mem->rsp);
	return NULL;
}

static int imp_delete(struct mem_st *mem)
{
/*
	if (mem->rcvbuf) {
		if (mem->rcvbuf->buf) {
			free(mem->rcvbuf->buf);
			mem->rcvbuf->buf = NULL;
		}
		free(mem->rcvbuf);
		mem->rcvbuf = NULL;
	}
*/
	if (mem->sndbuf.buf) {
		id_msg_buf_free(&mem->sndbuf);
	}
	if (mem->sd>=0) {
		close(mem->sd);
		mem->sd = -1;
	}
	free(mem);
}

static int imp_driver_do_list(struct mem_st *mem);

static int imp_driver_parse_msg(struct mem_st *mem)
{
	mem->req = msg_idlist_req__unpack(NULL, mem->rcvbuf->pb_len, mem->rcvbuf->pb_start);
	if (mem->req==NULL) {
		syslog(LOG_INFO, "Conn: %s: msg_idlist_req__unpack() failed.", __FUNCTION__);
		mem->state = ST_Ex;
		return 0;
	}
	syslog(LOG_DEBUG, "Conn: %s: msg_idlist_req__unpack() OK.", __FUNCTION__);
	return imp_driver_do_list(mem);
}

static int imp_driver_do_list(struct mem_st *mem)
{
	int num;

	if ((mem->rsp.n_list=id_list(&mem->rsp.list))<0) {
		syslog(LOG_INFO, "Conn: %s: id_list() failed.", __FUNCTION__);
		mem->rsp.n_list = 0;
		mem->rsp.list = NULL;
	}
	mem->state = ST_PREPARE_RSP;
	return 0;
}

static int imp_driver_prepare_rsp(struct mem_st *mem)
{
	int ret;
	int pb_len;

	pb_len = msg_idlist_rsp__get_packed_size(&mem->rsp);
	mem->sndbuf.buf_size = sizeof(struct id_msg_header_st) + pb_len;
	mem->sndbuf.buf = malloc(mem->sndbuf.buf_size);
	if (mem->sndbuf.buf==NULL) {
		mem->state = ST_Ex;
		return 0;
	}
	mem->sndbuf.hdr = (void*)mem->sndbuf.buf;
	mem->sndbuf.hdr->version = 1;
	mem->sndbuf.hdr->command = CMD_RSP_ID_LIST;
	mem->sndbuf.hdr->padding = 0;
	mem->sndbuf.hdr->body_length = htonl(pb_len);
	msg_idlist_rsp__pack(&mem->rsp, mem->sndbuf.hdr->body);
	mem->sndbuf.len = mem->sndbuf.buf_size;
	mem->sndbuf.pos = 0;

	msg_idlist_req__free_unpacked(mem->req, NULL);
	free(mem->rsp.list);
	mem->rsp.list = NULL;
	mem->req = NULL;

	mem->state = ST_SEND_RSP;
	return 0;
}

static int imp_driver_send_rsp(struct mem_st *mem)
{
	int ret;

send_data:
	ret = send(mem->sd, mem->sndbuf.buf + mem->sndbuf.pos, mem->sndbuf.len, 0);
	if (ret<=0) {
		if (errno==EINTR) {
			syslog(LOG_DEBUG, "ST_SEND_RSP[+%ds] sleep->", delta_t());
			goto send_data;
		} else {
			syslog(LOG_INFO, "ST_SEND_RSP[+%ds] error: %m->", delta_t());
			mem->state = ST_Ex;
			return 0;
		}
	} else {
		mem->sndbuf.pos += ret;
		mem->sndbuf.len -= ret;
		if (mem->sndbuf.len <= 0) {
			syslog(LOG_DEBUG, "ST_SEND_RSP[+%ds] %d bytes sent, OK->", delta_t(), ret);
			mem->state = ST_TERM;
			return 0;
		}
		syslog(LOG_DEBUG, "ST_SEND_RSP[+%ds] %d bytes sent, again->", delta_t(), ret);
		return 0;
	}
}

static int imp_driver_ex(struct mem_st *mem)
{
	syslog(LOG_DEBUG, "ST_Ex[+%ds] Exception occured.", delta_t());
	mem->state = ST_TERM;
	return 0;
}

static int imp_driver(struct mem_st *mem)
{
	switch (mem->state) {
		case ST_PARSE_MSG:
			return imp_driver_parse_msg(mem);
			break;
		case ST_PREPARE_RSP:
			return imp_driver_prepare_rsp(mem);
			break;
		case ST_SEND_RSP:
			return imp_driver_send_rsp(mem);
			break;
		case ST_Ex:
			return imp_driver_ex(mem);
			break;
		case ST_TERM:
			return 0;
			break;
		default:
			break;
	}
}

static void *imp_serialize(struct mem_st *mem)
{
}

int msg_id_list_enter(int sd, struct id_msg_buf_st *buf)
{
	struct mem_st *mem;

	mem = malloc(sizeof(struct mem_st));
	if (mem==NULL) {
		return -1;
	}
	mem->sd = sd;
	mem->rcvbuf = buf;
	imp_new(mem);
	while (mem->state!=ST_TERM) {
		imp_driver(mem);
	}
	imp_delete(mem);

	return 0;
}

