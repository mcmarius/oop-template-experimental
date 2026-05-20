/*
 * Mock npcap SDK headers for CI builds.
 * This file provides stub declarations for Packet32 functions.
 * For real builds, use the npcap SDK downloaded via scripts/download_npcap_sdk.ps1.
 */
#ifndef PACKET32_H
#define PACKET32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic Windows types (not available from Windows SDK in mock builds) */
typedef unsigned long  ULONG;
typedef unsigned short UINT;
typedef unsigned char  BOOL;
typedef uint16_t       WORD;
typedef WORD          *PWORD;
#define TRUE   1
#define FALSE  0

/* bpf_program and bpf_stat stubs (real ones are from pcap/pcap.h) */
struct bpf_program {
    int bf_len;
    /* bf_insns is a pointer in real pcap — avoid defining the struct content */
};
struct bpf_stat {
    int bs_recv;
    int bs_drop;
};

/* Mock types */
typedef struct _ADAPTER{} ADAPTER, *LPADAPTER;
typedef struct _PACKET{} PACKET, *LPPACKET;
typedef struct _PACKET_OID_DATA{} PACKET_OID_DATA, *LPPACKET_OID_DATA;

/* Mock functions */
LPADAPTER PacketOpenAdapter(char *AdapterName);
BOOL PacketGetVersion(LPADAPTER AdapterObject, PWORD VersionNumber);
BOOL PacketSendPacket(LPADAPTER AdapterObject, LPPACKET Packet, BOOL Sync);
BOOL PacketSetReadTimeout(LPADAPTER AdapterObject, int Timeout);
void PacketFreePacket(LPPACKET Packet);
BOOL PacketReceivePacket(LPADAPTER AdapterObject, LPPACKET Packet, BOOL Sync);
BOOL PacketSetBpf(LPADAPTER AdapterObject, struct bpf_program *fp, int install);
BOOL PacketSetLoopbackBehavior(LPADAPTER AdapterObject, UINT LoopbackBehavior);
void PacketSetHwFilter(LPADAPTER AdapterObject, ULONG Filter);
UINT PacketGetNumStats(LPADAPTER AdapterObject);
UINT PacketStats(LPADAPTER AdapterObject, struct bpf_stat *s);
ULONG PacketRequest(LPADAPTER AdapterObject, BOOL Set, PACKET_OID_DATA *OidData);

#ifdef __cplusplus
}
#endif

#endif /* PACKET32_H */
