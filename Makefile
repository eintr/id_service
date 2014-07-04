
MOD_NAME=id_service

sources=mod_skel.c id_msg_body.pb-c.c id_msg_header.c id_hasht.c id_api.c id_file.c id_api_listener.c id_msg_proc_get.c id_msg_proc_create.c id_pool.c

CFLAGS+=-D_GNU_SOURCE

LDFLAGS+=`pkg-config --libs 'libprotobuf-c >= 1.0.0'`

include ../module.inc

prepare: id_msg_body.pb-c.c

id_msg_body.pb-c.c: id_msg_body.proto
	protoc-c --c_out=. $^

test: ../../cJSON.o ../../util_syscall.o ../../util_log.o ../../ds_hasht.o id_hasht.c id_file.c testmain.c
	gcc -DTEST $(CFLAGS) -o $@ $^ $(LDFALGS) -pthread -lm

