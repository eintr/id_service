CFLAGS+=-I.. -I../../..

LDFLAGS+=`pkg-config --libs 'libprotobuf-c >= 1.0.0'`

%: %.o
	$(CC) -o $@ $^ ../id_msg_body.pb-c.o  $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

