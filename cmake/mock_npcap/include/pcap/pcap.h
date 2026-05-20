/*
 * Mock npcap SDK headers for CI builds.
 * This file provides stub declarations for libpcap functions.
 * For real builds, use the npcap SDK downloaded via scripts/download_npcap_sdk.ps1.
 */
#ifndef PCAP_H
#define PCAP_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mock types */
typedef struct pcap pcap_t;
typedef struct pcap_dumper pcap_dumper_t;
typedef struct pcap_if pcap_if_t;
typedef struct pcap_addr pcap_addr_t;
typedef struct pcap_send_queue pcap_send_queue_t;
typedef int bpf_u_int32;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

#define PCAP_ERRBUF_SIZE 256
#define DLT_EN10MB 1

/* Guard against redefinition — MinGW/Windows SDK already provide this */
#ifndef HAVE_STRUCT_TIMEVAL
struct timeval {
    long tv_sec;
    long tv_usec;
};
#define HAVE_STRUCT_TIMEVAL
#endif

struct pcap_pkthdr {
    struct timeval ts;
    bpf_u_int32 caplen;
    bpf_u_int32 len;
};

struct pcap_stat {
    u_int ps_recv;
    u_int ps_drop;
    u_int ps_ifdrop;
};

struct pcap_rmtauth {
    int type;
    char *username;
    char *password;
};

struct pcap_send_queue {
    u_int len;
    u_int maxlen;
    u_char *buffer;
};

struct bpf_stat {
    u_int bs_recv;
    u_int bs_drop;
};

typedef void (*pcap_handler)(u_char *user, const struct pcap_pkthdr *h, const u_char *sp);

enum pcap_direction {
    PCAP_D_INOUT = 0,
    PCAP_D_IN = 1,
    PCAP_D_OUT = 2,
    PCAP_D_INOUT_NOTED = 3
};

enum mode {
    MODE_CAPTURE = 0,
    MODE_STATISTICS = 1,
    MODE_MONITOR = 2
};

struct bpf_program {
    int bf_len;
    void *bf_insns;
};

struct bpf_insn {
    unsigned short code;
    unsigned char  jt;
    unsigned char  jf;
    int            k;
};

/* Mock functions — stub implementations in mock_pcap.c */
const char *pcap_lookupdev(char *errbuf);
int pcap_lookupnet(const char *device, bpf_u_int32 *netp, bpf_u_int32 *maskp, char *errbuf);
pcap_t *pcap_open(const char *device, int snaplen, int flags, int read_timeout, struct pcap_rmtauth *auth, char *errbuf);
pcap_t *pcap_open_dead(int linktype, int snaplen);
pcap_t *pcap_open_dead_with_tstamp_precision(int linktype, int snaplen, unsigned int tstamp_precision);
pcap_t *pcap_open_offline_with_tstamp_precision(const char *fname, char *errbuf, unsigned int tstamp_precision);
pcap_t *pcap_create(const char *device, char *errbuf);
int pcap_activate(pcap_t *p);
int pcap_close(pcap_t *p);
int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
int pcap_dispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
int pcap_next(pcap_t *p, struct pcap_pkthdr *h);
int pcap_sendpacket(pcap_t *p, const u_char *buf, int size);
int pcap_stats(pcap_t *p, struct pcap_stat *stat);
int pcap_stats_ex(pcap_t *p, struct pcap_stat *stat);
void pcap_breakloop(pcap_t *p);
int pcap_setfilter(pcap_t *p, struct bpf_program *fp);
int pcap_setdirection(pcap_t *p, enum pcap_direction d);
int pcap_geterr(pcap_t *p);
void pcap_perror(pcap_t *p, const char *prefix);
char *pcap_strerror(int error);
const char *pcap_version(void);
int pcap_compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize, bpf_u_int32 netmask);
int pcap_compile_nopcap(int snaplen_arg, int linktype_arg, struct bpf_program *fp, const char *str, int optimize, bpf_u_int32 mask);
void pcap_freecode(struct bpf_program *fp);
int pcap_offline_filter(const struct bpf_program *fp, const struct pcap_pkthdr *header, const u_char *dp);
int pcap_datalink(const pcap_t *p);
int pcap_datalink_ext(pcap_t *p);
int pcap_list_datalinks(pcap_t *p, int **dlt_buf);
int pcap_set_datalink(pcap_t *p, int dlt);
void pcap_free_datalinks(int *dlt_list);
int pcap_datalink_val_to_name(int dlt);
const char *pcap_datalink_val_to_description(int dlt);
int pcap_createsrcstr(char *str, int size, const char *source, const char *port, const char *name, int type, char *errbuf);
int pcap_parsesrcstr(const char *str, int *type, char *device, char *subnet, char *netmask, char *port, char *errbuf);
pcap_t *pcap_findalldevs_ex(const char *source, struct pcap_rmtauth *auth, pcap_if_t **alldevs, char *errbuf);
int pcap_findalldevs(pcap_if_t **alldevs, char *errbuf);
void pcap_freealldevs(pcap_if_t *alldevs);

/* Dump functions */
pcap_dumper_t *pcap_dump_open(pcap_t *p, const char *file);
pcap_dumper_t *pcap_dump_open_append(pcap_t *p, const char *file);
void pcap_dump(u_char *user, const struct pcap_pkthdr *h, const u_char *sp);
int pcap_dump_flush(pcap_dumper_t *p);
int pcap_dump_close(pcap_dumper_t *p);

/* Snapshot and buffer functions */
int pcap_set_snaplen(pcap_t *p, int snaplen);
int pcap_set_promisc(pcap_t *p, int promisc);
int pcap_set_timeout(pcap_t *p, int timeout);
int pcap_set_buffer_size(pcap_t *p, int buffer_size);
int pcap_set_immediate_mode(pcap_t *p, int mode);

/* Timestamp functions */
int pcap_list_tstamp_types(pcap_t *p, int **tstamp_types);
void pcap_free_tstamp_types(int *tstamp_types);
int pcap_set_tstamp_type(pcap_t *p, int tstamp_type);
int pcap_get_tstamp_precision(pcap_t *p);
int pcap_set_tstamp_precision(pcap_t *p, unsigned int tstamp_precision);
int pcap_set_tstamp_type_name(pcap_t *p, const char *tstamp_type_name);

/* Send queue */
int pcap_sendqueue_alloc(pcap_t *p, u_int memsize);
void pcap_sendqueue_destroy(pcap_send_queue_t *queue);
int pcap_sendqueue_queue(pcap_send_queue_t *queue, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);
int pcap_sendqueue_transmit(pcap_t *p, pcap_send_queue_t *queue, int sync);

/* Mode */
int pcap_setmode(pcap_t *p, enum mode mode);

/* Stats */
int pcap_get_selectable_fd(const pcap_t *p);

/* BPF dump */
void pcap_dump_file(pcap_dumper_t *p, FILE *file);
FILE *pcap_file(pcap_t *p);
int pcap_get_nonblock(pcap_t *p, int *nonblock);
int pcap_set_nonblock(pcap_t *p, int nonblock, char *errbuf);

/* Deprecated */
#define pcap_inject(p, buf, size) pcap_sendpacket(p, buf, size)

#ifdef __cplusplus
}
#endif

#endif /* PCAP_H */
