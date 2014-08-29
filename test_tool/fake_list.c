#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "id_msg_header.h"
#include "id_msg_body.pb-c.h"

#define	ADDR	"127.0.0.1"
#define PORT	19990

static char *idname;

void send_req(int sd)
{
	struct id_msg_header_st *sndbuf;
	MsgIDListReq req = MSG_IDLIST_REQ__INIT;
	size_t len, buf_size, ret;

	req.id_name = "";
	len = msg_idlist_req__get_packed_size(&req);
	buf_size = sizeof(struct id_msg_header_st) + len;
	sndbuf = alloca(buf_size);
	sndbuf->version = 1;
	sndbuf->command = CMD_REQ_ID_LIST;
	sndbuf->body_length = htonl(len);
	msg_idlist_req__pack(&req, sndbuf->body);

	ret = send(sd, sndbuf, buf_size, 0);
	printf("send() => %zd(%zd=%jd+%zd)\n", ret, buf_size, (intmax_t)sizeof(struct id_msg_header_st), len);
}

static void show_list_rsp(MsgIDListRsp *rsp)
{
	int i;

	for (i=0;i<rsp->n_list;++i) {
		printf("Item[%d]: {id_name=%s, value=%llu}\n", i, (rsp->list)[i]->id_name, (rsp->list)[i]->current);
	}
}

void recv_show_rsp(int sd)
{
	MsgIDListRsp *rsp;
	struct id_msg_header_st hdr;
	ssize_t len, pblen;
	uint8_t *pbbuf;
	int i;

	len = recv(sd, &hdr, sizeof(hdr), 0);
	if (len<0) {
		perror("recv()");
		return;
	} else if (len==0) {
		printf("EOF\n");
		return;
	} else if (len<sizeof(hdr)) {
		printf("Fragment!\n");
		return;
	} else {
		if (hdr.command != CMD_RSP_ID_LIST) {
			fprintf(stderr, "msg has a wrong command (%d)!\n", hdr.command);
			return;
		}
		pblen = ntohl(hdr.body_length);
		if (pblen>4096) {
			fprintf(stderr, "msg is too large!\n");
			return;
		}
		pbbuf = alloca(4096);
		len = recv(sd, pbbuf, pblen, 0);
		printf("recv() => %zu/%zu\n", len, pblen);
		rsp = msg_idlist_rsp__unpack(NULL, pblen, pbbuf);
		if (rsp==NULL) {
			fprintf(stderr, "msg_idlist_rsp__unpack() failed.\n");
			return;
		}
		show_list_rsp(rsp);
		msg_idlist_rsp__free_unpacked(rsp, NULL);
	}
}

int
main(int argc, char **argv)
{
	int sd;
	struct sockaddr_in peer_addr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd<0) {
		perror("socket()");
		exit(1);
	}

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_addr.s_addr = inet_addr(ADDR);
	peer_addr.sin_port = htons(PORT);
	if (connect(sd, (void*)&peer_addr, sizeof(peer_addr))<0) {
		perror("connect()");
		close(sd);
		return 0;
	}

	printf("Connected.\n");

	send_req(sd);

	recv_show_rsp(sd);

	close(sd);
	printf("Disconnected.\n");

	exit (0);
}

