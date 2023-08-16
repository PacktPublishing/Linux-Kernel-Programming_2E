# Makefile
ALL := primegen

CC=${CROSS_COMPILE}gcc
CFLAGS=-Wall
CFLAGS_DBG=-g -ggdb -gdwarf-4 -O0 -Wall -Wextra
CFLAGS_DBG_ASAN=${CFLAGS_DBG} -fsanitize=address
CFLAGS_DBG_MSAN=-fsanitize=memory -fPIE -pie
CFLAGS_DBG_UB=${CFLAGS_DBG} -fsanitize=undefined

all: ${ALL}
CB_FILES := *.[ch]

src = $(wildcard *.c)
obj = $(src:.c=.o)

#LDFLAGS = -lGL -lglut -lpng -lz -lm

primegen: $(obj)
	$(CC) ${CFLAGS} -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) ${ALL}
