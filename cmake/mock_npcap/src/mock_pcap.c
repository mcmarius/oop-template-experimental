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

/* --- Fake pcap_t struct for mock mode ---
 * PcapPlusPlus (and libpcap) use opaque pointers that are actually allocated
 * structs. We must allocate real memory for them so that calls like
 * pcap_geterr(p) don't dereference garbage addresses (which causes segfaults
 * on CI Windows with MSVC). */
struct pcap {
    int error_code;
    char errbuf[PCAP_ERRBUF_SIZE];
};

/* Track allocated fake descriptors for pcap_close / pcap_freealldevs */
static pcap_t *g_fake_descriptors[64];
static int g_num_fakes = 0;
static int g_next_fake_id = 1;

/* Helper: allocate a fake pcap_t with an initial error code */
static pcap_t *alloc_fake_pcap(int error_code) {
    if (g_num_fakes >= (int)(sizeof(g_fake_descriptors) / sizeof(g_fake_descriptors[0])))
        return NULL;
    pcap_t *p = (pcap_t *)calloc(1, sizeof(pcap_t));
    if (!p) return NULL;
    p->error_code = error_code;
    p->errbuf[0] = '\0';
    g_fake_descriptors[g_num_fakes++] = p;
    return p;
}

/* Helper: find index of a fake descriptor, -1 if not found */
static int find_fake_idx(pcap_t *p) {
    for (int i = 0; i < g_num_fakes; i++)
        if (g_fake_descriptors[i] == p) return i;
    return -1;
}

/* Helper: free a fake descriptor from the tracked list */
static void free_fake_pcap(pcap_t *p) {
    int idx = find_fake_idx(p);
    if (idx >= 0) {
        free(p);
        /* Shift remaining entries */
        for (int i = idx; i < g_num_fakes - 1; i++)
            g_fake_descriptors[i] = g_fake_descriptors[i + 1];
        g_num_fakes--;
    }
}

/* --- Stub implementations --- */

char *pcap_lookupdev(char *errbuf) {
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
    return alloc_fake_pcap(0);
}

pcap_t *pcap_open_dead_with_tstamp_precision(int linktype, int snaplen,
                                              u_int tstamp_precision) {
    (void)tstamp_precision;
    return alloc_fake_pcap(0);
}

pcap_t *pcap_open_offline_with_tstamp_precision(const char *fname, u_int tstamp_precision,
                                                 char *errbuf) {
    (void)tstamp_precision;
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: offline open not implemented");
    return NULL;
}

pcap_t *pcap_create(const char *device, char *errbuf) {
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: pcap_create not implemented");
    return NULL;
}

int pcap_activate(pcap_t *p) { return 0; }

void pcap_close(pcap_t *p) {
    free_fake_pcap(p);
}

int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user) { return 0; }

int pcap_dispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user) { return 0; }

const u_char *pcap_next(pcap_t *p, struct pcap_pkthdr *h) {
    (void)p;
    (void)h;
    return NULL;
}

int pcap_next_ex(pcap_t *p, struct pcap_pkthdr **h, const u_char **sp) {
    (void)p;
    (void)h;
    (void)sp;
    return -1;
}

int pcap_sendpacket(pcap_t *p, const u_char *buf, int size) {
    (void)p;
    (void)buf;
    (void)size;
    return 0;
}

int pcap_stats(pcap_t *p, struct pcap_stat *stat) {
    (void)p;
    if (stat) { memset(stat, 0, sizeof(*stat)); return -1; }
    return -1;
}

int pcap_stats_ex(pcap_t *p, struct pcap_stat *stat) {
    (void)p;
    (void)stat;
    return -1;
}

void pcap_breakloop(pcap_t *p) { (void)p; }

int pcap_setfilter(pcap_t *p, struct bpf_program *fp) {
    (void)p;
    (void)fp;
    return -1;
}

int pcap_setdirection(pcap_t *p, pcap_direction_t d) {
    (void)p;
    (void)d;
    return 0;
}

char *pcap_geterr(pcap_t *p) {
    if (!p) return "Invalid pcap descriptor";
    if (p->errbuf[0] == '\0') {
        snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "Mock pcap error %d", p->error_code);
    }
    return p->errbuf;
}

void pcap_perror(pcap_t *p, const char *prefix) {
    fprintf(stderr, "%s: %s\n", prefix ? prefix : "pcap",
            p ? pcap_geterr(p) : "Invalid pcap descriptor");
}

const char *pcap_strerror(int error) {
    (void)error;
    return "Mock pcap error";
}

const char *pcap_lib_version(void) {
    return "libpcap mock version (stub)";
}

int pcap_compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize,
                 bpf_u_int32 netmask) {
    (void)p;
    (void)fp;
    (void)str;
    (void)optimize;
    (void)netmask;
    return -1;
}

int pcap_compile_nopcap(int snaplen_arg, int linktype_arg, struct bpf_program *fp,
                        const char *str, int optimize, bpf_u_int32 mask) {
    (void)snaplen_arg;
    (void)linktype_arg;
    (void)fp;
    (void)str;
    (void)optimize;
    (void)mask;
    return -1;
}

void pcap_freecode(struct bpf_program *fp) {
    (void)fp;
}

int pcap_offline_filter(const struct bpf_program *fp, const struct pcap_pkthdr *header,
                        const u_char *dp) {
    return 0;
}

int pcap_datalink(pcap_t *p) { (void)p; return DLT_EN10MB; }

int pcap_datalink_ext(pcap_t *p) { return DLT_EN10MB; }

int pcap_list_datalinks(pcap_t *p, int **dlt_buf) { return -1; }

int pcap_set_datalink(pcap_t *p, int dlt) { return -1; }

void pcap_free_datalinks(int *dlt_list) {}

const char *pcap_datalink_val_to_name(int dlt) {
    (void)dlt;
    return "en10mb";
}

const char *pcap_datalink_val_to_description(int dlt) {
    (void)dlt;
    return "Ethernet";
}

int pcap_createsrcstr(char *source, int type, const char *host,
                      const char *port, const char *name, char *errbuf) {
    (void)source;
    (void)type;
    (void)host;
    (void)port;
    (void)name;
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: srcstr creation not implemented");
    return -1;
}

int pcap_parsesrcstr(const char *source, int *type, char *host,
                     char *port, char *name, char *errbuf) {
    (void)source;
    (void)type;
    (void)host;
    (void)port;
    (void)name;
    (void)errbuf;
    return -1;
}

int pcap_findalldevs_ex(const char *source, struct pcap_rmtauth *auth,
                        pcap_if_t **alldevs, char *errbuf) {
    /* Return success with an empty (NULL) device list so the app can
       gracefully handle "no devices available" instead of crashing. */
    fprintf(stderr, "[MOCK] pcap_findalldevs_ex(source=%s) -> returning empty device list\n",
            source ? source : "NULL");
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: no devices available");
    if (alldevs) *alldevs = NULL;
    return 0;
}

int pcap_findalldevs(pcap_if_t **alldevs, char *errbuf) {
    /* Return success with an empty (NULL) device list so the app can
       gracefully handle "no devices available" instead of crashing. */
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: no devices available");
    if (alldevs) *alldevs = NULL;
    return 0;
}

void pcap_freealldevs(pcap_if_t *alldevs) {
    /* The real pcap_freealldevs frees each node and its name/description
       strings. Our mock doesn't allocate any, so no-op is fine. */
    (void)alldevs;
}

pcap_dumper_t *pcap_dump_open(pcap_t *p, const char *file) {
    (void)p;
    (void)file;
    return NULL;
}

pcap_dumper_t *pcap_dump_open_append(pcap_t *p, const char *file) {
    return NULL;
}

void pcap_dump(u_char *user, const struct pcap_pkthdr *h, const u_char *sp) {}

int pcap_dump_flush(pcap_dumper_t *p) { return 0; }

void pcap_dump_close(pcap_dumper_t *p) { (void)p; }

int pcap_set_snaplen(pcap_t *p, int snaplen) { return 0; }
int pcap_set_promisc(pcap_t *p, int promisc) { return 0; }
int pcap_set_timeout(pcap_t *p, int timeout) { return 0; }
int pcap_set_buffer_size(pcap_t *p, int buffer_size) { return 0; }
int pcap_set_immediate_mode(pcap_t *p, int mode) { return 0; }

int pcap_list_tstamp_types(pcap_t *p, int **tstamp_types) { return -1; }
void pcap_free_tstamp_types(int *tstamp_types) {}
int pcap_set_tstamp_type(pcap_t *p, int tstamp_type) { return 0; }
int pcap_get_tstamp_precision(pcap_t *p) { return 0; }
int pcap_set_tstamp_precision(pcap_t *p, int tstamp_precision) {
    (void)tstamp_precision;
    return 0;
}

struct pcap_send_queue *pcap_sendqueue_alloc(u_int memsize) {
    (void)memsize;
    return NULL;
}
void pcap_sendqueue_destroy(struct pcap_send_queue *queue) { (void)queue; }
int pcap_sendqueue_queue(struct pcap_send_queue *queue, const struct pcap_pkthdr *pkt_header,
                         const u_char *pkt_data) {
    (void)queue;
    (void)pkt_header;
    (void)pkt_data;
    return 0;
}
u_int pcap_sendqueue_transmit(pcap_t *p, struct pcap_send_queue *queue, int sync) {
    (void)p;
    (void)queue;
    (void)sync;
    return 0;
}

int pcap_setmode(pcap_t *p, enum mode mode) { return 0; }

int pcap_get_selectable_fd(pcap_t *p) { (void)p; return -1; }

int pcap_snapshot(pcap_t *p) { (void)p; return 0; }

int pcap_get_nonblock(pcap_t *p, int *nonblock) { return 0; }
int pcap_set_nonblock(pcap_t *p, int nonblock, char *errbuf) { return 0; }

/* --- Additional stubs required by PcapPlusPlus on Windows --- */

pcap_t *pcap_open_offline(const char *fname, char *errbuf) {
    fprintf(stderr, "[MOCK] pcap_open_offline(%s) -> returning NULL (not implemented)\n",
            fname ? fname : "NULL");
    if (errbuf) snprintf(errbuf, PCAP_ERRBUF_SIZE, "Mock: pcap_open_offline not implemented");
    return NULL;
}

int pcap_setmintocopy(pcap_t *p, int size) {
    /* WinPcap-specific; no-op on mock */
    return size;
}