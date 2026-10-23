PROGS	:= run_interp run_fast_interp run_jit snippets.o
CFLAGS	:= -g -O2

all: $(PROGS)

run_%: main.o %.o util.o
	$(CC) $^ -o $@

%.o: %.c bpf.h

clean:
	rm -f *.o $(PROGS)
