
#include <stdio.h>
#include <string.h>

#include "storage.h"
#include "pages.h"

int
read_directory(const char* path, void* buf, fuse_fill_dir_t filler)
{
    return read_used_inodes(path, buf, filler);
}

int
get_stat(const char* path, struct stat* st)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }

    memset((void*)st, 0, sizeof(struct stat));
    st->st_uid  = getuid();
    st->st_mode = node->mode;
    st->st_size = node->size;
    st->st_ino = node->inode_num;
    st->st_nlink = 1;
    return 0;
}

int
create_file(const char *path, mode_t mode)
{
    inode* node = get_node_at_path(path);
    if (node != NULL) {
        return node->inode_num;
    }
    int node_num = find_empty_inode();
    node = pages_get_node(node_num);
    node->mode = mode;
    node->inode_num = node_num;
    if (mode != 040777) {
        node->mode = 0100666;
    }
    strcpy(node->path, path);
    if (strcmp(path, "/") != 0) {
        add_file_to_dir("/", path);
    }
    return node_num;
}

int
create_directory(const char* path, mode_t mode)
{
    int node_num = create_file(path, 040777);
    inode* node = pages_get_node(node_num);
    if (node->num_blocks == 0) {
        int ents = 0;
        write_data(path, &ents, sizeof(int), 0);
    }

    return node_num;
}


int
storage_access(const char* path)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }

    return 0;
}

int
get_data(const char *path, void *buf, size_t size, off_t offset)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }

    if (size == 0 || node->num_blocks == 0) {
        return 0;
    }

    int num_to_read = size;
    void* block = pages_get_page(node->direct[0]);
    if (offset > 0) {
        block += offset;
    }

    memcpy(buf, block, num_to_read);
    return num_to_read;
}

int
write_data(const char *path, const void *buf, size_t size, off_t offset)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }

    if (size == 0) {
        return 0;
    }
    if (node->num_blocks == 0) {
        give_inode_page(node);
    }

    int num_to_write = size;
    void* block = pages_get_page(node->direct[0]);


    if (offset > 0) {
        block += offset;
    }

    memcpy(block, buf, num_to_write);
    if (size + offset > node->size) {
        node->size = size + offset;
    }
    return num_to_write;
}

void
storage_init(const char* path)
{
    pages_init(path);
    create_directory("/", 040777);
}

int
rename_file(const char *from, const char *to)
{
    inode* node = get_node_at_path(from);
    memset(node->path, 0, 64);
    strcpy(node->path, to);
    return 0;
}

int
remove_file(const char* path)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }
    free_inode(node);
    inode* dir = get_node_at_path("/");
    remove_inode_from_directory(dir, node->inode_num);
    return 0;
}

int
create_link(const char* target, const char* link_name)
{
    int new_node = find_empty_inode();
    inode* link = pages_get_node(new_node);
    inode* target_node = get_node_at_path(target);

    link->inode_num = new_node;
    link->mode = target_node->mode;
    strcpy(link->path, link_name);
    link->num_blocks = target_node->num_blocks;
    for (int i = 0; i < link->num_blocks; i++) {
        link->direct[i] = target_node->direct[i];
    }
    target_node->refs++;
    link->refs++;
    print_node(link);
    print_node(target_node);
    return 0;
}

int
update_permissions(const char* path, mode_t mode)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }
    node->mode = mode;
    return 0;
}

int
truncate_file(const char* path, off_t size)
{
    inode* node = get_node_at_path(path);
    if (node == NULL) {
        return -1;
    }
    node->size = size;
    return 0;
}
