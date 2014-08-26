include config.mk

INSTALL_PREFIX=/usr/local/titans/id_service

TARGET=titans_id_service

sources=main.c conf.c util_log.c mod_skel.c id_msg_body.pb-c.c id_msg_header.c id_hasht.c id_api.c id_file.c id_api_listener.c id_msg_proc_get.c id_msg_proc_create.c id_msg_proc_list.c id_pool.c

objects=$(sources:.c=.o)

CFLAGS+=-D_GNU_SOURCE

#LDFLAGS+=`pkg-config --libs 'libprotobuf-c >= 1.0.0'`
LDFLAGS+=-lprotobuf-c

include ../module.inc

all: $(TARGET)

prepare: id_msg_body.pb-c.c

id_msg_body.pb-c.c: id_msg_body.proto
	protoc-c --c_out=. $^

$(TARGET): $(objects)
	$(CC) -o $@ $^ $(LDFLAGS)

test: ../../cJSON.o ../../util_syscall.o ../../util_log.o ../../ds_hasht.o id_hasht.c id_file.c testmain.c
	gcc -DTEST $(CFLAGS) -o $@ $^ $(LDFALGS) -pthread -lm

