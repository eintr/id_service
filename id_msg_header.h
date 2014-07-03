#ifndef PUSH_INTERFACE_H
#define PUSH_INTERFACE_H

/** \file push_service.h
	源自：
http://cr.admin.weibo.com/w/projects/titans/%E6%8E%A5%E5%8F%A3%E6%96%87%E6%A1%A3/#push
*/

/** cond 0 */
#include <stdint.h>
/** endcond */

#define	PUSH_MSG_VERSION		1

#define CMD_REQ_EST		0x01  // Mobile->PUSH	创建连接
#define CMD_REQ_HB		0x02  //
#define CMD_REQ_TOKEN	0x03  // 客户端上传Token

#define CMD_CAST_NOTIFY	0x91  // PUSH->Mobile	Notify通知

#define CMD_REQ_PUSH	0x11  // SYNC->PUSH	PUSH请求
#define CMD_RSP_PUSH	0xa1  // PUSH->SYNC	PUSH回应

#define CMD_REQ_SESSCHK	0x12  // PUSH->SYNC	session_id 查询请求
#define CMD_RSP_SESSCHK	0xa2  // SYNC->PUSH	session_id 查询回应

/** Generic push message */ 
struct push_msg_header_st {
	uint32_t version:8;		/**< Message version */
	uint32_t command:8;		/**< Message command */
	uint32_t padding:16;	/**< Padding */
	uint32_t body_length;	/**< Length of body part */
	uint8_t body[0];		/**< Body, protobuf format, see push_interface.proto for more details */
}__attribute__((packed));

/* =========================================== */

#include <util_conn_tcp.h>

struct push_msg_buf_st {
	uint8_t *buf;
	size_t buf_size;
	size_t len, pos;
	uint8_t *pb_start;
	size_t pb_len;
};

enum {
	RCV_OVER=1,
	RCV_CONTINUE,
	RCV_WAIT,
	RCV_ERROR
};

int push_msg_recv_generic(conn_tcp_t *conn, struct push_msg_buf_st *b, int version, int command);
void push_msg_buf_free(struct push_msg_buf_st *b);

#endif

