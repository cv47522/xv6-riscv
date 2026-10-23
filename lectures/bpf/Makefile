all: seccomp.bpf drop_icmp.o set_ttl.o

%.o: %.c
	clang -O2 -Wall --target=bpf -c $< -o $@

genseccomp: genseccomp.c
	gcc genseccomp.c -o genseccomp -lseccomp

seccomp.bpf: genseccomp
	./genseccomp > $@
