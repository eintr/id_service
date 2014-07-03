
MOD_NAME=id_service

sources=mod_skel.c id_msg_body.pb-c.c id_msg_header.c

LDFLAGS+=`pkg-config --libs 'libprotobuf-c >= 1.0.0'`

include ../module.inc

prepare: push_msg_body.pb-c.c

push_msg_body.pb-c.c: push_msg_body.proto
	protoc-c --c_out=. $^

test: ../../util_syscall.o ../../util_log.o id_file.c
	gcc -DTEST $(CFLAGS) -o $@ $^ $(LDFALGS)

