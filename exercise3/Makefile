CC = mpicc
CFLAGS = -Wall -g
LDFLAGS = -lucp -lucs -luct

TARGET = pingpong
SRCS = pingpong.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
