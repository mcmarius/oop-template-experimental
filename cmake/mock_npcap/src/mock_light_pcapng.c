/*
 * Mock light_pcapng stub implementation for CI builds.
 * This provides stub implementations of all light_pcapng functions
 * required by the pre-built PcapPlusPlus libraries.
 * For real builds, use the npcap SDK downloaded via scripts/download_npcap_sdk.ps1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stub types — complete structs with at least one member (C requires this) */
typedef struct light_file { int _opaque; } light_file_t;
typedef struct light_block { int _opaque; } light_block_t;
typedef struct light_option { int _opaque; } light_option_t;
typedef struct light_compression_context { int _opaque; } light_compression_context_t;
typedef struct light_file_info { int _opaque; } light_file_info_t;

/* --- Stub implementations --- */

/* File operations */
light_file_t *light_open(const char *filename, const char *mode) {
    return (light_file_t *)fopen(filename, mode);
}

light_file_t *light_open_compression(const char *filename, const char *mode) {
    return light_open(filename, mode);
}

int light_close(light_file_t *f) {
    if (f) fclose((FILE *)f);
    return 0;
}

int light_close_compressed(light_file_t *f) {
    return light_close(f);
}

int light_eof(light_file_t *f) {
    if (!f) return 1;
    return feof((FILE *)f);
}

int light_size(light_file_t *f) {
    if (!f) return 0;
    fseek((FILE *)f, 0, SEEK_END);
    return ftell((FILE *)f);
}

int light_get_pos(light_file_t *f) {
    if (!f) return 0;
    return ftell((FILE *)f);
}

int light_set_pos(light_file_t *f, int pos) {
    if (!f) return -1;
    return fseek((FILE *)f, pos, SEEK_SET);
}

/* Block operations */
light_block_t *light_alloc_block(void) {
    return (light_block_t *)calloc(1, sizeof(light_block_t));
}

void light_free_block(light_block_t *block) {
    free(block);
}

int light_add_block(light_block_t *block, int type, const void *data, int len) {
    return 0;
}

int light_get_block_count(light_file_t *f) {
    if (!f) return 0;
    return 0;
}

light_block_t *light_get_block_info(light_file_t *f, int index) {
    (void)f;
    (void)index;
    return NULL;
}

/* Option operations */
light_option_t *light_alloc_option(void) {
    return (light_option_t *)calloc(1, sizeof(light_option_t));
}

void light_free_option(light_option_t *opt) {
    free(opt);
}

int light_add_option(light_option_t *opt, int code, const void *data, int len) {
    return 0;
}

int light_create_option(light_option_t **opt, int code, const void *data, int len) {
    if (opt) *opt = light_alloc_option();
    return 0;
}

light_option_t *light_get_option(light_block_t *block, int code) {
    (void)block;
    (void)code;
    return NULL;
}

void *light_get_option_data(light_option_t *opt, int *len) {
    if (len) *len = 0;
    return NULL;
}

int light_get_option_length(light_option_t *opt) {
    return 0;
}

/* File info operations */
light_file_info_t *light_create_file_info(void) {
    return (light_file_info_t *)calloc(1, sizeof(light_file_info_t));
}

light_file_info_t *light_create_default_file_info(void) {
    return light_create_file_info();
}

void light_free_file_info(light_file_info_t *info) {
    free(info);
}

int light_pcang_get_file_info(light_file_t *f, light_file_info_t *info) {
    (void)f;
    (void)info;
    return 0;
}

/* Packet operations */
int light_read(light_file_t *f, void *buf, int len) {
    if (!f || !buf) return -1;
    return fread(buf, 1, len, (FILE *)f);
}

int light_read_compressed(light_file_t *f, void *buf, int len) {
    return light_read(f, buf, len);
}

int light_read_from_memory(light_file_t *f, void *buf, int len) {
    return light_read(f, buf, len);
}

int light_read_record(light_file_t *f, void *buf, int len) {
    return light_read(f, buf, len);
}

int light_write(light_file_t *f, const void *buf, int len) {
    if (!f || !buf) return -1;
    return fwrite(buf, 1, len, (FILE *)f);
}

int light_write_compressed(light_file_t *f, const void *buf, int len) {
    return light_write(f, buf, len);
}

int light_write_packet(light_file_t *f, const void *pkt, int len) {
    return light_write(f, pkt, len);
}

/* Flush operations */
int light_flush(light_file_t *f) {
    if (!f) return -1;
    return fflush((FILE *)f);
}

int light_pcapng_flush(light_file_t *f) {
    return light_flush(f);
}

/* pcapng operations */
light_file_t *light_pcapng_open_read(const char *filename) {
    return light_open(filename, "rb");
}

light_file_t *light_pcapng_open_write(const char *filename) {
    return light_open(filename, "wb");
}

light_file_t *light_pcapng_open_append(const char *filename) {
    return light_open(filename, "ab");
}

int light_pcapng_close(light_file_t *f) {
    return light_close(f);
}

void light_pcapng_release(light_file_t *f) {
    light_close(f);
}

int light_pcapng_to_file_stream(light_file_t *f, FILE *stream) {
    if (!f || !stream) return -1;
    return 0;
}

/* Compression context */
light_compression_context_t *light_get_compression_context(void) {
    return (light_compression_context_t *)calloc(1, sizeof(light_compression_context_t));
}

light_compression_context_t *light_get_decompression_context(void) {
    return light_get_compression_context();
}

int light_is_compressed_file(light_file_t *f) {
    (void)f;
    return 0;
}
