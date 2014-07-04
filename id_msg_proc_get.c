#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <room_service.h>
#include <util_misc.h>
#include <util_atomic.h>

#include "mod_config.h"
#include "id_msg_body.pb-c.h"
#include "id_msg_header.h"
#include "id_msg_proc.h"

#define	NAMESIZE	1024

struct mem_st {
	int state;
	conn_tcp_t *conn;
	struct id_msg_buf_st *rcvbuf;

	MsgIDGetReq *req;

	MsgIDGetRsp rsp;

	struct id_msg_buf_st sndbuf;
};

enum {
	ST_PARSE_MSG=1,
	ST_PREPARE_RSP,
	ST_SEND_RSP,
	ST_Ex,
	ST_TERM
};

static void *imp_new(imp_t *imp)
{
	struct mem_st *mem = imp->memory;

	mem->state = ST_PARSE_MSG;
	mem->sndbuf.buf = NULL;
	msg_idget_rsp__init(&mem->rsp);
	return NULL;
}

static int imp_delete(imp_t *imp)
{
	struct mem_st *mem = imp->memory;
	if (mem->sndbuf.buf) {
		id_msg_buf_free(&mem->sndbuf);
	}
	if (mem->conn) {
		conn_tcp_close_nb(mem->conn);
		mem->conn = NULL;
	}
}

static enum enum_driver_retcode imp_driver_parse_msg(imp_t *imp)
{
	struct mem_st *mem = imp->memory;

	mem->req = msg_idget_req__unpack(NULL, mem->rcvbuf->pb_len, mem->rcvbuf->pb_start);
	if (mem->req==NULL) {
		mylog(L_INFO, "imp[%d]: %s: msg_idget_req__unpack() failed.", imp->id, __FUNCTION__);
		mem->state = ST_Ex;
		return TO_RUN;
	}
	mem->state = ST_PREPARE_RSP;
	return TO_RUN;
}

static enum enum_driver_retcode imp_driver_prepare_rsp(imp_t *imp)
{
	struct mem_st *mem = imp->memory;
	int ret;
	int pb_len;

	ret = id_get(mem->req->id_name, &mem->rsp.id, mem->req->num);
	if (ret!=0) {
		mem->rsp.id = 0;
		mem->rsp.reason = "Get id failed";
	} else {
		mem->rsp.reason = "OK";
	}
	pb_len = msg_idget_rsp__get_packed_size(&mem->rsp);
	mem->sndbuf.buf_size = sizeof(struct id_msg_header_st) + pb_len;
	mem->sndbuf.buf = malloc(mem->sndbuf.buf_size);
	if (mem->sndbuf.buf==NULL) {
		mem->state = ST_Ex;
		return TO_RUN;
	}
	mem->sndbuf.hdr = (void*)mem->sndbuf.buf;
	mem->sndbuf.hdr->version = 1;
	mem->sndbuf.hdr->command = CMD_RSP_ID_GET;
	mem->sndbuf.hdr->padding = 0;
	mem->sndbuf.hdr->body_length = htonl(pb_len);
	msg_idget_rsp__pack(&mem->rsp, mem->sndbuf.hdr->body);
	mem->sndbuf.len = mem->sndbuf.buf_size;
	mem->sndbuf.pos = 0;

	msg_idget_req__free_unpacked(mem->req, NULL);
	mem->req = NULL;

	mem->state = ST_SEND_RSP;
	return TO_RUN;
}

static enum enum_driver_retcode imp_driver_send_rsp(imp_t *imp)
{
	int ret;
	struct mem_st *mem = imp->memory;

	if (imp->event_mask & EV_MASK_TIMEOUT  || imp->event_mask & EV_MASK_IOERR) {
		mylog(L_INFO, "imp[%d]: ST_SEND_RSP timed out or error.", imp->id);
		mem->state = ST_Ex;
		return TO_RUN;
	}
	ret = conn_tcp_send_nb(mem->conn, mem->sndbuf.buf + mem->sndbuf.pos, mem->sndbuf.len);
	if (ret<=0) {
		if (errno==EAGAIN) {
			mylog(L_INFO, "ST_SEND_RSP[+%ds] sleep->", delta_t());
			imp_set_ioev(imp, mem->conn->sd, EPOLLOUT|EPOLLRDHUP);
			imp_set_timer(imp, id_module_config.snd_api_timeout_ms);
			return TO_WAIT_IO;
		} else {
			mylog(L_INFO, "ST_SEND_RSP[+%ds] error: %m->", delta_t());
			mem->state = ST_Ex;
			return TO_RUN;
		}
	} else {
		mem->sndbuf.pos += ret;
		mem->sndbuf.len -= ret;
		if (mem->sndbuf.len <= 0) {
			mylog(L_INFO, "ST_SEND_RSP[+%ds] %d bytes sent, OK->", delta_t(), ret);
			mem->state = ST_TERM;
			return TO_RUN;
		}
		mylog(L_INFO, "ST_SEND_RSP[+%ds] %d bytes sent, again->", delta_t(), ret);
		return TO_RUN;
	}
}

static enum enum_driver_retcode imp_driver_ex(imp_t *imp)
{
	struct mem_st *mem = imp->memory;

	mylog(L_INFO, "ST_Ex[+%ds] Exception occured.", delta_t());
	mem->state = ST_TERM;
	return TO_RUN;
}

static enum enum_driver_retcode driver(imp_t *imp)
{
	struct mem_st *mem = imp->memory;

	switch (mem->state) {
		case ST_PARSE_MSG:
			return imp_driver_parse_msg(imp);
			break;
		case ST_PREPARE_RSP:
			return imp_driver_prepare_rsp(imp);
			break;
		case ST_SEND_RSP:
			return imp_driver_send_rsp(imp);
			break;
		case ST_Ex:
			return imp_driver_ex(imp);
			break;
		case ST_TERM:
			return TO_TERM;
			break;
		default:
			break;
	}
}

static void *imp_serialize(imp_t *imp)
{
	struct mem_st *mem = imp->memory;
}

static imp_soul_t soul = {
	.fsm_new = imp_new,
	.fsm_delete = imp_delete,
	.fsm_driver = driver,
	.fsm_serialize = imp_serialize,
};

imp_t *msg_id_get_summon(conn_tcp_t *conn, struct id_msg_buf_st *buf)
{
	struct mem_st *mem;

	mem = malloc(sizeof(struct mem_st));
	if (mem==NULL) {
		return NULL;
	}

	mem->conn = conn;
	mem->rcvbuf = buf;

	return imp_summon(mem, &soul);
}

