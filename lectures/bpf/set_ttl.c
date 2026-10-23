#include <stdint.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

// https://notes.xeome.dev/notes/XDP-Tutorial
static inline void
csum_replace2(uint16_t *sum, uint16_t old, uint16_t new)
{
  uint16_t csum = ~*sum;
 
  csum += ~old;
  csum += csum < (uint16_t)~old;
 
  csum += new;
  csum += csum < (uint16_t)new;
 
  *sum = ~csum;
}

SEC("prog")
int
xdp_modify_ttl(struct xdp_md *ctx)
{
  void *data_end = (void *)(long)ctx->data_end;
  void *data = (void *)(long)ctx->data;

  struct ethhdr *eth = data;
  if ((void *)(eth + 1) > data_end)
    return XDP_PASS;

  if (bpf_ntohs(eth->h_proto) != ETH_P_IP)
    return XDP_PASS;

  struct iphdr *ip = data + sizeof(*eth);
  // What happens if we remove this check?
  if ((void *)(ip + 1) > data_end)
    return XDP_PASS;

  int new_ttl = 99;
  csum_replace2(&ip->check, bpf_htons(ip->ttl << 8), bpf_htons(new_ttl << 8));
  ip->ttl = new_ttl;

  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
