#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <errno.h>

#include <util_log.h>

#include "id_msg_header.h"

#define	BUF_SIZE_INIT	1024
#define	BUF_SIZE_EXPAND	4096

int id_msg_recv_generic(conn_tcp_t *conn, struct id_msg_buf_st *b, int version, int command)
{
    struct id_msg_header_st *hdr;
	void *newbuf;
    struct epoll_event ev;
    ssize_t len;

	if (b->buf==NULL) {
		b->buf_size = conn_tcp_suggest_bufsize(conn)*2;
		if (b->buf_size < BUF_SIZE_INIT) {
			b->buf_size = BUF_SIZE_INIT;
		}
		b->buf = calloc(b->buf_size, 1);
		if (b->buf == NULL) {
			return RCV_ERROR;
		}
		b->len = 0;
		b->pos = 0;
	}

	if (b->buf_size - b->len <1) {
		/* expand the buffer if needed. */
		newbuf = realloc(b->buf, b->buf_size + BUF_SIZE_EXPAND);
		if (newbuf==NULL) {
			mylog(L_ERR, "Memory allocation failed!");
			return RCV_ERROR;
		}
		mylog(L_DEBUG, "Buffer size (%zd) is too small, expaned to %zd.", b->buf_size, b->buf_size+BUF_SIZE_EXPAND);
		b->buf_size += BUF_SIZE_EXPAND;
		b->buf = newbuf;
	}


	len = conn_tcp_recv_nb(conn, b->buf + b->pos, b->buf_size - b->len);
	if (len<0) {
		if (errno==EAGAIN) {
			//fprintf(stderr, "%s: errno==EAGAIN\n", __FUNCTION__);
			return RCV_WAIT;
		} else {
			//fprintf(stderr, "%s: errno==%d\n", __FUNCTION__, errno);
			return RCV_ERROR;
		}
	} else if (len==0) {
		//fprintf(stderr, "%s: peer closed.\n", __FUNCTION__);
		return RCV_ERROR;
	} else {
		b->len += len;
		if (b->len<sizeof(struct id_msg_header_st)) {
			//fprintf(stderr, "%s: msg header is not completed, again.\n", __FUNCTION__);
			return RCV_CONTINUE;
		}
		hdr = (void*)(b->buf);
		if (b->len < sizeof(struct id_msg_header_st)+ ntohl(hdr->body_length)) {
			//fprintf(stderr, "%s: msg body is not completed, again.\n", __FUNCTION__);
			return RCV_CONTINUE;
		}
		fprintf(stderr, "%s: msg recv completed.\n", __FUNCTION__);
		if (hdr->version != version) {
			//fprintf(stderr, "%s: msg version==%d, Drop it!\n", __FUNCTION__, hdr->version);
			return RCV_ERROR;
		}
		if (command>0 && hdr->command != command) {
			//fprintf(stderr, "%s: msg command==%d, which I don't know how to deal with. Drop it!\n", __FUNCTION__, hdr->command);
			return RCV_ERROR;
		}
		b->hdr = (void*)b->buf;
		b->pb_start = b->buf + sizeof(struct id_msg_header_st);
		b->pb_len = ntohl(hdr->body_length);
		//fprintf(stderr, "%s: msg ok.\n", __FUNCTION__);
		return RCV_OVER;
	}
}

void id_msg_buf_free(struct id_msg_buf_st *b)
{
	if (b) {
		free(b->buf);
		b->buf = NULL;
		b->buf_size = 0;
		b->len = 0;
		b->pos = 0;
	}
}

