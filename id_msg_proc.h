#ifndef ID_MSG_PROC_H
#define ID_MSG_PROC_H

#include <room_service.h>
#include <util_conn_tcp.h>
#include "id_msg_header.h"

imp_t *msg_id_create_summon(conn_tcp_t*, struct id_msg_buf_st*);
imp_t *msg_id_list_summon(conn_tcp_t*, struct id_msg_buf_st*);
imp_t *msg_id_get_summon(conn_tcp_t*, struct id_msg_buf_st*);

#endif

