OBJ = slock.o rwlock.o rcu.o

CC=gcc

CFLAGS += -I. -MMD -MP
#CFLAGS += -g
#CFLAGS += -O2
CFLAGS += -g -O2 -fno-omit-frame-pointer
DEPS = $(wildcard *.d)
-include $(DEPS)

clist: clist.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
