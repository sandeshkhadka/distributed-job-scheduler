#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

#define MAX_SYSCALL_ID 512

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} target_cgroup SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, MAX_SYSCALL_ID);
    __type(key, __u32);
    __type(value, __u64);
} syscall_count SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 2);
    __type(key, __u32);
    __type(value, __u64);
} io_bytes SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 2);
    __type(key, __u32);
    __type(value, __u64);
} net_bytes SEC(".maps");

#define __NR_read 0
#define __NR_write 1
#define __NR_openat 257
#define __NR_sendto 44
#define __NR_recvfrom 45

SEC("tp/raw_syscalls/sys_enter")
int handle_sys_enter(struct trace_event_raw_sys_enter* ctx) {
    __u32 key = 0;
    __u64* cgroup_id = bpf_map_lookup_elem(&target_cgroup, &key);
    if (!cgroup_id)
        return 0;

    if (bpf_get_current_cgroup_id() != *cgroup_id)
        return 0;

    __u32 syscall_nr = (__u32)ctx->id;

    if (syscall_nr < MAX_SYSCALL_ID) {
        __u64* cnt = bpf_map_lookup_elem(&syscall_count, &syscall_nr);
        if (cnt) {
            __sync_fetch_and_add(cnt, 1);
        }
    }

    if (syscall_nr == __NR_read) {
        __u32 rkey = 0;
        __u64* bytes = bpf_map_lookup_elem(&io_bytes, &rkey);
        if (bytes) {
            __sync_fetch_and_add(bytes, ctx->args[2]);
        }
    }

    if (syscall_nr == __NR_write) {
        __u32 wkey = 1;
        __u64* bytes = bpf_map_lookup_elem(&io_bytes, &wkey);
        if (bytes) {
            __sync_fetch_and_add(bytes, ctx->args[2]);
        }
    }

    if (syscall_nr == __NR_sendto) {
        __u32 txkey = 0;
        __u64* bytes = bpf_map_lookup_elem(&net_bytes, &txkey);
        if (bytes) {
            __sync_fetch_and_add(bytes, ctx->args[2]);
        }
    }

    if (syscall_nr == __NR_recvfrom) {
        __u32 rxkey = 1;
        __u64* bytes = bpf_map_lookup_elem(&net_bytes, &rxkey);
        if (bytes) {
            __sync_fetch_and_add(bytes, ctx->args[2]);
        }
    }

    return 0;
}
