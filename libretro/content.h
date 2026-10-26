#ifndef KOF98_CONTENT_H
#define KOF98_CONTENT_H
#include <stddef.h>
/* Paths are owned by the adapter; no process cwd or compiled-in data paths. */
#define KOF98_PATH_MAX 1024
int kof98_join(char *out, size_t capacity, const char *dir, const char *name);
int kof98_file_exists(const char *dir, const char *name);
int kof98_content_directory(const char *content, char *out, size_t capacity);
int kof98_validate_content(const char *rom, const char *system, char *bios,
                          size_t capacity, char *error, size_t error_size);
#endif
