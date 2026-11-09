#define SBRK_ERROR ((char *)-1)

struct stat;

/*
 * System calls
 *
 * Q: Which files implement these, and why open() versus sys_open()?
 * A: These declare user stubs. user/usys.pl generates them in
 *    user/usys.S: open() loads SYS_open into a7 and executes ecall.
 *    syscalls[] in kernel/syscall.c maps that number to sys_open() in
 *    kernel/sysfile.c; no C call crosses the privilege boundary, so the
 *    names may differ. Other handlers live in kernel/sysproc.c and
 *    kernel/log.c. See docs/05-syscall-reference.md.
 */
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int *);
int pipe(int *);
int write(int, const void *, int);
int read(int, void *, int);
int close(int);
int kill(int);
int exec(const char *, char **);
int open(const char *, int);
int mknod(const char *, short, short);
int unlink(const char *);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int mkdir(const char *);
int chdir(const char *);
int dup(int);
int getpid(void);
char *sys_sbrk(int, int);
int pause(int);
int uptime(void);
int sync(void);
int interpose(int, char *); // mask, path; path not length-checked here

// ulib.c
int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int strcmp(const char *, const char *);
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);
int atoi(const char *);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
char *sbrk(int);
char *sbrklazy(int);

// printf.c
void fprintf(int, const char *, ...) __attribute__((format(printf, 2, 3)));
void printf(const char *, ...) __attribute__((format(printf, 1, 2)));

// umalloc.c
void *malloc(uint);
void free(void *);
