#include <seccomp.h>
#include <stdio.h>
#include <errno.h>

int
main(int argc, char *argv[])
{
  scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);

  seccomp_rule_add(ctx, SCMP_ACT_ERRNO(ELOOP), SCMP_SYS(mkdir), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(write), 1,
    SCMP_CMP(0, SCMP_CMP_GE, 3));

  seccomp_export_bpf(ctx, 1);
  seccomp_release(ctx);
}
