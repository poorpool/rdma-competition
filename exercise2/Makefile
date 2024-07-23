# Makefile

CC = gcc
CFLAGS = -libverbs -O3 -g
TARGET = server

all: $(TARGET)
	ln -sf $(TARGET) client

$(TARGET): bw_template.c
	$(CC) -o $(TARGET) bw_template.c $(CFLAGS)

clean:
	rm -f $(TARGET) client
