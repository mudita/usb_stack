# Copyright  Onplick <info@onplick.com> - All Rights Reserved
# Unauthorized copying of this file, via any medium is strictly prohibited
# Proprietary and confidential

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

CFLAGS = -Wall -fPIC -MMD -DNDEBUG

.PHONY: all lib test clean

all: lib test

lib: libmtp.a

test: lib
	make -C tests

libmtp.a: $(OBJS)
	$(AR) csr $@ $^

clean:
	make -C tests clean
	rm -rf $(TESTDIR)
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f libmtp.a

include $(wildcard *.d)
