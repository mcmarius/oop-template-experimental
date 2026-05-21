/*
 * Mock npcap SDK headers for CI builds.
 * This file provides stub declarations for libpcap functions.
 * For real builds, use the npcap SDK downloaded via scripts/download_npcap_sdk.ps1.
 */
#ifndef PCAP_H
#define PCAP_H

#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PCAP_API macro — matches real npcap SDK funcattrs.h.
   In the real SDK this is __declspec(dllimport) when PCAP_DLL is defined.
   Our mock is always static, so it's empty. */
#ifndef PCAP_API
#define PCAP_API
#endif

/* Mock types */
typedef struct pcap pcap_t;
typedef struct pcap_dumper pcap_dumper_t;
typedef struct pcap_if pcap_if_t;
typedef struct pcap_addr pcap_addr_t;
typedef struct pcap_send_queue pcap_send_queue_t;
typedef struct pcap_send_queue pcap_send_queue;
typedef struct pcap_stat_ex pcap_stat_ex_t;
typedef int bpf_u_int32;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef int pcap_direction_t;

#define PCAP_ERRBUF_SIZE 256
#define DLT_EN10MB 1

/* struct timeval definition.
   Only define on non-Linux systems — on Linux <sys/types.h> provides it. */
#ifndef __linux__
struct timeval {
    long tv_sec;
    long tv_usec;
};
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

/* Mock functions — stub implementations in mock_pcap.c.
   All signatures match the real npcap SDK pcap.h (SDK 1.16). */

PCAP_API char    *pcap_lookupdev(char *errbuf);
PCAP_API int     pcap_lookupnet(const char *device, bpf_u_int32 *netp, bpf_u_int32 *maskp, char *errbuf);
PCAP_API pcap_t *pcap_open(const char *device, int snaplen, int flags, int read_timeout, struct pcap_rmtauth *auth, char *errbuf);
PCAP_API pcap_t *pcap_open_dead(int linktype, int snaplen);
PCAP_API pcap_t *pcap_open_dead_with_tstamp_precision(int linktype, int snaplen, u_int tstamp_precision);
PCAP_API pcap_t *pcap_open_offline_with_tstamp_precision(const char *fname, u_int tstamp_precision, char *errbuf);
PCAP_API pcap_t *pcap_open_offline(const char *fname, char *errbuf);
PCAP_API pcap_t *pcap_create(const char *device, char *errbuf);
PCAP_API int     pcap_activate(pcap_t *p);
PCAP_API void    pcap_close(pcap_t *p);
PCAP_API int     pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
PCAP_API int     pcap_dispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
PCAP_API const u_char *pcap_next(pcap_t *p, struct pcap_pkthdr *h);
PCAP_API int     pcap_next_ex(pcap_t *p, struct pcap_pkthdr **h, const u_char **sp);
PCAP_API int     pcap_sendpacket(pcap_t *p, const u_char *buf, int size);
PCAP_API int     pcap_stats(pcap_t *p, struct pcap_stat *stat);
PCAP_API int     pcap_stats_ex(pcap_t *p, struct pcap_stat *stat);
PCAP_API void    pcap_breakloop(pcap_t *p);
PCAP_API int     pcap_setfilter(pcap_t *p, struct bpf_program *fp);
PCAP_API int     pcap_setdirection(pcap_t *p, pcap_direction_t d);
PCAP_API char    *pcap_geterr(pcap_t *p);
PCAP_API void    pcap_perror(pcap_t *p, const char *prefix);
PCAP_API const char *pcap_strerror(int error);
PCAP_API const char *pcap_lib_version(void);
PCAP_API int     pcap_compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize, bpf_u_int32 netmask);
PCAP_API int     pcap_compile_nopcap(int snaplen_arg, int linktype_arg, struct bpf_program *fp, const char *str, int optimize, bpf_u_int32 mask);
PCAP_API void    pcap_freecode(struct bpf_program *fp);
PCAP_API int     pcap_offline_filter(const struct bpf_program *fp, const struct pcap_pkthdr *header, const u_char *dp);
PCAP_API int     pcap_datalink(pcap_t *p);
PCAP_API int     pcap_datalink_ext(pcap_t *p);
PCAP_API int     pcap_list_datalinks(pcap_t *p, int **dlt_buf);
PCAP_API int     pcap_set_datalink(pcap_t *p, int dlt);
PCAP_API void    pcap_free_datalinks(int *dlt_list);
PCAP_API const char *pcap_datalink_val_to_name(int dlt);
PCAP_API const char *pcap_datalink_val_to_description(int dlt);
PCAP_API int     pcap_createsrcstr(char *source, int type, const char *host, const char *port, const char *name, char *errbuf);
PCAP_API int     pcap_parsesrcstr(const char *source, int *type, char *host, char *port, char *name, char *errbuf);
PCAP_API int     pcap_findalldevs_ex(const char *source, struct pcap_rmtauth *auth, pcap_if_t **alldevs, char *errbuf);
PCAP_API int     pcap_findalldevs(pcap_if_t **alldevs, char *errbuf);
PCAP_API void    pcap_freealldevs(pcap_if_t *alldevs);

/* Dump functions */
PCAP_API pcap_dumper_t *pcap_dump_open(pcap_t *p, const char *file);
PCAP_API pcap_dumper_t *pcap_dump_open_append(pcap_t *p, const char *file);
PCAP_API void           pcap_dump(u_char *user, const struct pcap_pkthdr *h, const u_char *sp);
PCAP_API int            pcap_dump_flush(pcap_dumper_t *p);
PCAP_API void           pcap_dump_close(pcap_dumper_t *p);

/* Snapshot and buffer functions */
PCAP_API int pcap_set_snaplen(pcap_t *p, int snaplen);
PCAP_API int pcap_set_promisc(pcap_t *p, int promisc);
PCAP_API int pcap_set_timeout(pcap_t *p, int timeout);
PCAP_API int pcap_set_buffer_size(pcap_t *p, int buffer_size);
PCAP_API int pcap_set_immediate_mode(pcap_t *p, int mode);

/* Timestamp functions */
PCAP_API int     pcap_list_tstamp_types(pcap_t *p, int **tstamp_types);
PCAP_API void    pcap_free_tstamp_types(int *tstamp_types);
PCAP_API int     pcap_set_tstamp_type(pcap_t *p, int tstamp_type);
PCAP_API int     pcap_get_tstamp_precision(pcap_t *p);
PCAP_API int     pcap_set_tstamp_precision(pcap_t *p, int tstamp_precision);

/* Send queue */
PCAP_API pcap_send_queue *pcap_sendqueue_alloc(u_int memsize);
PCAP_API void             pcap_sendqueue_destroy(pcap_send_queue *queue);
PCAP_API int              pcap_sendqueue_queue(pcap_send_queue *queue, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);
PCAP_API u_int            pcap_sendqueue_transmit(pcap_t *p, pcap_send_queue *queue, int sync);

/* Mode */
PCAP_API int pcap_setmode(pcap_t *p, enum mode mode);

/* Stats */
PCAP_API int pcap_get_selectable_fd(pcap_t *p);

/* BPF dump */
PCAP_API void   pcap_dump_file(pcap_dumper_t *p, FILE *file);
PCAP_API FILE  *pcap_file(pcap_t *p);
PCAP_API int    pcap_snapshot(pcap_t *p);
PCAP_API int    pcap_get_nonblock(pcap_t *p, int *nonblock);
PCAP_API int    pcap_set_nonblock(pcap_t *p, int nonblock, char *errbuf);

/* Windows-specific / additional functions needed by PcapPlusPlus */
PCAP_API int pcap_setmintocopy(pcap_t *p, int size);

/* Deprecated */
#define pcap_inject(p, buf, size) pcap_sendpacket(p, buf, size)

#ifdef __cplusplus
}
#endif

#endif /* PCAP_H */
