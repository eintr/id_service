#ifndef ID_INTERFACE_H
#define ID_INTERFACE_H

/** \file push_service.h
	源自：
http://cr.admin.weibo.com/w/projects/titans/%E6%8E%A5%E5%8F%A3%E6%96%87%E6%A1%A3/#id
*/

/** cond 0 */
#include <stdint.h>
/** endcond */

#define	PUSH_MSG_VERSION		1

#define CMD_REQ_ID_CREATE         0x21  // 创建序列请求
#define CMD_RSP_ID_CREATE         0xb1  // 创建序列回应

#define CMD_REQ_ID_LIST                 0x22  // 列举序列请求
#define CMD_RSP_ID_LIST                 0xb2  // 列举序列回应

#define CMD_REQ_ID_GET                 0x23  // 获取序列号请求
#define CMD_RSP_ID_GET                 0xb3  // 获取序列号回应

/** Generic push message */ 
struct id_msg_header_st {
	uint32_t version:8;		/**< Message version */
	uint32_t command:8;		/**< Message command */
	uint32_t padding:16;	/**< Padding */
	uint32_t body_length;	/**< Length of body part */
	uint8_t body[0];		/**< Body, protobuf format, see push_interface.proto for more details */
}__attribute__((packed));

/* =========================================== */

struct id_msg_buf_st {
	uint8_t *buf;
	struct id_msg_header_st *hdr;	// Always == buf, for convenions.
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

int id_msg_recv_generic(int sd, struct id_msg_buf_st *b, int version, int command);
void id_msg_buf_free(struct id_msg_buf_st *b);

#endif

