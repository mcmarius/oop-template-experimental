/*
 * Mock libpcap stub implementation for CI builds.
 * These stubs provide minimal implementations to satisfy the linker.
 * For real builds, use the npcap SDK downloaded via scripts/download_npcap_sdk.ps1.
 */
#include "pcap/pcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* <time.h> intentionally omitted — MinGW's _timeval.h redefines struct timeval
   which was already defined by our mock pcap.h. No stub function needs time() */

/* --- Stub implementations --- */

const char *pcap_lookupdev(char *errbuf) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: no devices available");
    return NULL;
}

int pcap_lookupnet(const char *device, bpf_u_int32 *netp, bpf_u_int32 *maskp, char *errbuf) {
    if (netp) *netp = 0;
    if (maskp) *maskp = 0;
    return 0;
}

pcap_t *pcap_open(const char *device, int snaplen, int flags, int read_timeout,
                  struct pcap_rmtauth *auth, char *errbuf) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: pcap_open not implemented");
    return NULL;
}

pcap_t *pcap_open_dead(int linktype, int snaplen) {
    return (pcap_t *)0x1;
}

pcap_t *pcap_open_dead_with_tstamp_precision(int linktype, int snaplen,
                                              unsigned int tstamp_precision) {
    return (pcap_t *)0x2;
}

pcap_t *pcap_open_offline_with_tstamp_precision(const char *fname, char *errbuf,
                                                 unsigned int tstamp_precision) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: offline open not implemented");
    return NULL;
}

pcap_t *pcap_create(const char *device, char *errbuf) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: pcap_create not implemented");
    return NULL;
}

int pcap_activate(pcap_t *p) { return 0; }

int pcap_close(pcap_t *p) { return 0; }

int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user) { return 0; }

int pcap_dispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user) { return 0; }

int pcap_next(pcap_t *p, struct pcap_pkthdr *h) { return -1; }

int pcap_sendpacket(pcap_t *p, const u_char *buf, int size) { return 0; }

int pcap_stats(pcap_t *p, struct pcap_stat *stat) { return -1; }

int pcap_stats_ex(pcap_t *p, struct pcap_stat *stat) { return -1; }

void pcap_breakloop(pcap_t *p) {}

int pcap_setfilter(pcap_t *p, struct bpf_program *fp) { return -1; }

int pcap_setdirection(pcap_t *p, enum pcap_direction d) { return 0; }

int pcap_geterr(pcap_t *p) { return -1; }

void pcap_perror(pcap_t *p, const char *prefix) {
    fprintf(stderr, "%s: Mock pcap error\n", prefix);
}

char *pcap_strerror(int error) {
    return "Mock pcap error";
}

const char *pcap_lib_version(void) {
    return "libpcap mock version (stub)";
}

int pcap_compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize,
                 bpf_u_int32 netmask) {
    return -1;
}

int pcap_compile_nopcap(int snaplen_arg, int linktype_arg, struct bpf_program *fp,
                        const char *str, int optimize, bpf_u_int32 mask) {
    return -1;
}

void pcap_freecode(struct bpf_program *fp) {}

int pcap_offline_filter(const struct bpf_program *fp, const struct pcap_pkthdr *header,
                        const u_char *dp) {
    return 0;
}

int pcap_datalink(const pcap_t *p) { return DLT_EN10MB; }

int pcap_datalink_ext(pcap_t *p) { return DLT_EN10MB; }

int pcap_list_datalinks(pcap_t *p, int **dlt_buf) { return -1; }

int pcap_set_datalink(pcap_t *p, int dlt) { return -1; }

void pcap_free_datalinks(int *dlt_list) {}

int pcap_datalink_val_to_name(int dlt) {
    return -1;
}

const char *pcap_datalink_val_to_description(int dlt) {
    return "Mock link type";
}

int pcap_createsrcstr(char *str, int size, const char *source, const char *port,
                      const char *name, int type, char *errbuf) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: srcstr creation not implemented");
    return -1;
}

int pcap_parsesrcstr(const char *str, int *type, char *device, char *subnet,
                     char *netmask, char *port, char *errbuf) {
    return -1;
}

pcap_t *pcap_findalldevs_ex(const char *source, struct pcap_rmtauth *auth,
                            pcap_if_t **alldevs, char *errbuf) {
    /* Return success with an empty (NULL) device list so the app can
       gracefully handle "no devices available" instead of crashing
       when iterating uninitialized memory. */
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: no devices available");
    if (alldevs) *alldevs = NULL;
    return NULL;
}

int pcap_findalldevs(pcap_if_t **alldevs, char *errbuf) {
    /* Return success with an empty (NULL) device list so the app can
       gracefully handle "no devices available" instead of crashing. */
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: no devices available");
    if (alldevs) *alldevs = NULL;
    return 0;
}

void pcap_freealldevs(pcap_if_t *alldevs) {}

pcap_dumper_t *pcap_dump_open(pcap_t *p, const char *file) {
    return NULL;
}

pcap_dumper_t *pcap_dump_open_append(pcap_t *p, const char *file) {
    return NULL;
}

void pcap_dump(u_char *user, const struct pcap_pkthdr *h, const u_char *sp) {}

int pcap_dump_flush(pcap_dumper_t *p) { return 0; }

int pcap_dump_close(pcap_dumper_t *p) { return 0; }

int pcap_set_snaplen(pcap_t *p, int snaplen) { return 0; }
int pcap_set_promisc(pcap_t *p, int promisc) { return 0; }
int pcap_set_timeout(pcap_t *p, int timeout) { return 0; }
int pcap_set_buffer_size(pcap_t *p, int buffer_size) { return 0; }
int pcap_set_immediate_mode(pcap_t *p, int mode) { return 0; }

int pcap_list_tstamp_types(pcap_t *p, int **tstamp_types) { return -1; }
void pcap_free_tstamp_types(int *tstamp_types) {}
int pcap_set_tstamp_type(pcap_t *p, int tstamp_type) { return 0; }
int pcap_get_tstamp_precision(pcap_t *p) { return 0; }
int pcap_set_tstamp_precision(pcap_t *p, unsigned int tstamp_precision) { return 0; }

int pcap_sendqueue_alloc(pcap_t *p, u_int memsize) { return 0; }
void pcap_sendqueue_destroy(struct pcap_send_queue *queue) {}
int pcap_sendqueue_queue(struct pcap_send_queue *queue, const struct pcap_pkthdr *pkt_header,
                         const u_char *pkt_data) { return 0; }
int pcap_sendqueue_transmit(pcap_t *p, struct pcap_send_queue *queue, int sync) { return 0; }

int pcap_setmode(pcap_t *p, enum mode mode) { return 0; }

int pcap_get_selectable_fd(const pcap_t *p) { return -1; }

int pcap_get_nonblock(pcap_t *p, int *nonblock) { return 0; }
int pcap_set_nonblock(pcap_t *p, int nonblock, char *errbuf) { return 0; }

/* --- Additional stubs required by PcapPlusPlus on Windows --- */

pcap_t *pcap_open_offline(const char *fname, char *errbuf) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: pcap_open_offline not implemented");
    /* Return a non-NULL handle so Pcap++ can call open() without NULL deref */
    return (pcap_t *)0x3;
}

int pcap_setmintocopy(pcap_t *p, int size) {
    /* WinPcap-specific; no-op on mock */
    return size;
}