#ifndef FS_H
#define FS_H

#include "disk.h"

class INE5412_FS {
public:
    static const unsigned int FS_MAGIC = 0xf0f03410;
    static const unsigned short int INODES_PER_BLOCK = 128;
    static const unsigned short int POINTERS_PER_INODE = 5;
    static const unsigned short int POINTERS_PER_BLOCK = 1024;

    class fs_superblock {
    public:
        unsigned int magic;
        int nblocks;
        int ninodeblocks;
        int ninodes;
    };

    class fs_inode {
    public:
        int isvalid;
        int size;
        int direct[POINTERS_PER_INODE];
        int indirect;
    };

    union fs_block {
    public:
        fs_superblock super;
        fs_inode inode[INODES_PER_BLOCK];
        int pointers[POINTERS_PER_BLOCK];
        char data[Disk::DISK_BLOCK_SIZE];
    };

    INE5412_FS(Disk *d) : disk(d), mounted(false), block_bitmap(nullptr) {}

    ~INE5412_FS() { if (block_bitmap) delete[] block_bitmap; }

    void fs_debug();
    int fs_format();
    int fs_mount();
    int fs_create();
    int fs_delete(int inumber);
    int fs_getsize(int inumber);
    int fs_read(int inumber, char *data, int length, int offset);
    int fs_write(int inumber, const char *data, int length, int offset);

private:
    Disk *disk;
    bool mounted;
    bool *block_bitmap;

    void inode_load(int inumber, fs_inode *inode);
    void inode_save(int inumber, fs_inode *inode);
    int allocate_block();
    void free_block(int blocknum);
};

#endif