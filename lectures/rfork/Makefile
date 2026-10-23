CC      = gcc
CFLAGS  = -O2 -g -Wall -Wextra

# The stub is freestanding and is linked at a fixed low address (0x20000)
# where no ordinary process has anything mapped.
STUB_CFLAGS  = $(CFLAGS) -ffreestanding -fno-builtin -fno-stack-protector \
               -fno-tree-loop-distribute-patterns -fno-pie -mgeneral-regs-only
RFORK_PORT = 7777

STUB_LDFLAGS = -static -nostdlib -nostartfiles -no-pie \
               -Wl,-Ttext-segment=0x20000 -Wl,--build-id=none

all: demo rforkd rfork-stub

# The parent is linked statically: then it is laid out just like the static
# stub, so the kernel puts the vdso at the same address in both.
demo: demo.o rfork.o rfork_ctx.o
	$(CC) $(CFLAGS) -static -o $@ $^

rforkd: rforkd.o
	$(CC) $(CFLAGS) -o $@ $^

rfork-stub: stub.o stub_start.o
	$(CC) $(STUB_LDFLAGS) -o $@ $^

stub.o: stub.c rfork_proto.h
	$(CC) $(STUB_CFLAGS) -c -o $@ $<

stub_start.o: stub_start.S
	$(CC) -c -o $@ $<

demo.o rfork.o: rfork.h rfork_proto.h
rforkd.o: rfork_proto.h

# start the "other machine" in the background, run the parent, stop the daemon
# (RFORKD_FLAGS=-v shows the memory map and every page fault, RFORK_PORT=n picks a port)
run: all
	@./rforkd $(RFORKD_FLAGS) $(RFORK_PORT) & pid=$$!; sleep 0.2; RFORK_PORT=$(RFORK_PORT) ./demo; sleep 0.2; kill $$pid

clean:
	rm -f *.o demo rforkd rfork-stub

.PHONY: all run clean
