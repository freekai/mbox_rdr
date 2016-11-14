OSX_VER = 10.12
CC = gcc
CFLAGS = -g -std=c99 -I. -mmacosx-version-min=$(OSX_VER)
LD = ld
LDFLAGS = -lc -macosx_version_min $(OSX_VER)
DEPS = mbox_rdr.h
OBJ = mbox_rdr.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

mbox_rdr.dylib: $(OBJ)
	$(LD) $(LDFLAGS) -dylib -o $@ $^

.PHONY: clean test

test: mbox_rdr.dylib test.o
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OBJ) mbox_rdr.dylib test.o test
