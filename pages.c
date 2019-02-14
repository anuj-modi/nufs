
#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "pages.h"


const int NUFS_SIZE  = 1024 * 1024; // 1MB
const int PAGE_COUNT = 200;
const int BLOCK_SIZE = 4096;

static int   pages_fd   = -1;
static void* pages_base =  0;

static char* blockmap;
static void* blocks;
static int num_nodes = 75;
static char* nodemap;
static inode* nodes;
static int next_node = 2;

void
pages_init(const char* path)
{
    pages_fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(pages_fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv == 0);

    pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
    assert(pages_base != MAP_FAILED);

    void* current_location = pages_base;

    // set up node bitmap
    nodemap = current_location;
    current_location += num_nodes * sizeof(char);

    // set up data bitmap
    blockmap = current_location;
    current_location += PAGE_COUNT * sizeof(char);

    // set up node array
    nodes = current_location;

    for (int i = 0; i < num_nodes; i++) {
        current_location += sizeof(inode);
    }

    blocks = pages_base + (5 * BLOCK_SIZE);

}

void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
pages_get_page(int pnum)
{
    return blocks + 4096 * pnum;
}

inode*
pages_get_node(int node_id)
{
    return &(nodes[node_id]);
}

int
pages_find_empty_block()
{
    int pnum = -1;
    for (int ii = 0; ii < PAGE_COUNT; ++ii) {
        if (blockmap[ii] == 0) { // if page is empty
            pnum = ii;
            blockmap[ii] = 1;
            break;
        }
    }
    return pnum;
}

int
find_empty_inode()
{
    int inum = -1;
    for (int ii = 2; ii < PAGE_COUNT; ++ii) {
        if (nodemap[ii] == 0) { // if page is empty
            inum = ii;
            nodemap[ii] = 1;
            break;
        }
    }
    return inum;
}

inode*
get_node_at_path(const char *path)
{
    for (int i = 0; i < num_nodes; i++) {
        if (nodemap[i] == 1 && strcmp(nodes[i].path, path) == 0) {
            return &(nodes[i]);
        }
    }
    return NULL;
}

void
print_node(inode* node)
{
    if (node) {
        printf("node{id: %d, refs: %d, mode: %04o, size: %d, path: %s}\n",
               node->inode_num, node->refs, node->mode, node->size, node->path);
    }
    else {
        printf("node{null}\n");
    }
}

int
same_path(const char* path1, const char* path2)
{
    return 0;
}

int
read_used_inodes(const char* path, void* buf, fuse_fill_dir_t filler)
{
    inode* node = get_node_at_path(path);
    int* data = pages_get_page(node->direct[0]);
    int num_ents = data[0];

    for (int i = 0; i < num_ents; i++) {
        int curr_node_num = data[i + 1];
        struct stat st;
        memset(&st, 0, sizeof(struct stat));
        st.st_uid  = getuid();
        st.st_mode = nodes[curr_node_num].mode;
        st.st_size = nodes[curr_node_num].size;
        st.st_ino = curr_node_num;
        filler(buf, ((void*)&(nodes[curr_node_num].path)) + 1, &st, 0);

    }
    // struct stat* st = malloc(sizeof(struct stat));
    // get_stat(nodes[i].path, &st);

    // print_node(&nodes[i]);
    // printf("filling: %s\n", nodes[i].path);
    // free(st);

    return 0;
}

int
give_inode_page(inode* node)
{
    int pnum = pages_find_empty_block();
    node->direct[node->num_blocks] = pnum;
    node->num_blocks++;
    return pnum;
}

void
add_file_to_dir(const char* dir, const char* file)
{
    inode* dir_node = get_node_at_path(dir);
    inode* file_node = get_node_at_path(file);
    int* data = pages_get_page(dir_node->direct[0]);
    int dir_size = data[0];
    data[dir_size + 1] = file_node->inode_num;
    data[0]++;
}

void
free_inode(inode* node)
{
    nodemap[node->inode_num] = 0;
    if (node->refs > 0) {
        for (int i = 0; i < node->num_blocks; i++) {
            blockmap[node->direct[i]] = 0;
        }
    }
}

void
remove_inode_from_directory(inode* dir, int node_num)
{
    int* dir_ents = pages_get_page(dir->direct[0]);
    int dir_size = dir_ents[0];
    if (dir_ents[dir_size] == node_num) {
        dir_ents[0]--;
        return;
    }

    for (int i = 1; i < dir_size; i++) {
        if (dir_ents[i] == node_num) {
            dir_ents[i] = dir_ents[dir_size];
            dir_ents[0]--;
            return;
        }
    }
}
