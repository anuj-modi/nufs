#ifndef PAGES_H
#define PAGES_H

#include <stdio.h>
#include <fuse.h>

#include "storage.h"

typedef struct inode {
    int refs;       // reference count
    int mode;       // permission & type
    int size;       // bytes for file
    int num_blocks; // number of blocks used for file
    int direct[10];  // array of direct block numbers
    int indirect;   // number of block that contains more blocks
    int inode_num;
    char path[64];   // name/path of the file
} inode;

void   pages_init(const char* path);
void   pages_free();
void*  pages_get_page(int pnum);
inode* pages_get_node(int node_id);
int    pages_find_empty();
void   print_node(inode* node);
int    find_empty_inode();
inode* get_node_at_path(const char *path);
int    read_used_inodes(const char* path, void* buf, fuse_fill_dir_t filler);
int    give_inode_page(inode* node);
void   add_file_to_dir(const char* dir, const char* file);
void   free_inode(inode* node);
void   remove_inode_from_directory(inode* dir, int node_num);


#endif
