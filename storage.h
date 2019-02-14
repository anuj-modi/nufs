#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>

void storage_init(const char* path);
int  get_stat(const char* path, struct stat* st);
int  create_file(const char *path, mode_t mode);
int  create_directory(const char *path, mode_t mode);
int  read_directory(const char* path, void* buf, fuse_fill_dir_t filler);
int  storage_access(const char* path);
int  get_data(const char *path, void *buf, size_t size, off_t offset);
int  write_data(const char *path, const void *buf, size_t size, off_t offset);
int  rename_file(const char *from, const char *to);
int  remove_file(const char* path);
int  create_link(const char* target, const char* link_name);
int  update_permissions(const char* path, mode_t mode);
int  truncate_file(const char* path, off_t size);


#endif
