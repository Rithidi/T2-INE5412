#include "../include/fs.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

// Função de depuração que exibe o estado do sistema de arquivos
void INE5412_FS::fs_debug() {
    fs_block block;
    disk->read(0, block.data); // Lê o superbloco

    std::cout << "superblock:" << std::endl;
    if (block.super.magic == FS_MAGIC) {
        std::cout << "    magic number is valid" << std::endl;
    } else {
        std::cout << "    invalid magic number" << std::endl;
        return;
    }

    // Informações do superbloco
    std::cout << "    " << block.super.nblocks << " blocks on disk" << std::endl;
    std::cout << "    " << block.super.ninodeblocks << " blocks for inodes" << std::endl;
    std::cout << "    " << block.super.ninodes << " inodes total" << std::endl;

    // Itera pelos blocos de inodes
    for (int i = 0; i < block.super.ninodeblocks; ++i) {
        disk->read(i + 1, block.data);
        for (int j = 0; j < INODES_PER_BLOCK; ++j) {
            fs_inode &inode = block.inode[j];
            if (inode.isvalid) {
                std::cout << "inode " << (i * INODES_PER_BLOCK + j) << ":" << std::endl;
                std::cout << "    size: " << inode.size << " bytes" << std::endl;
                std::cout << "    direct blocks:";
                for (int k = 0; k < POINTERS_PER_INODE; ++k) {
                    if (inode.direct[k]) std::cout << " " << inode.direct[k];
                }
                std::cout << std::endl;
                if (inode.indirect) {
                    std::cout << "    indirect block: " << inode.indirect << std::endl;
                    fs_block indirect_block;
                    disk->read(inode.indirect, indirect_block.data);
                    std::cout << "    indirect data blocks:";
                    for (int k = 0; k < POINTERS_PER_BLOCK; ++k) {
                        if (indirect_block.pointers[k]) std::cout << " " << indirect_block.pointers[k];
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
}

// Formata o disco inicializando o sistema de arquivos
int INE5412_FS::fs_format() {
    fs_block block = {};
    block.super.magic = FS_MAGIC;
    block.super.nblocks = disk->size();
    block.super.ninodeblocks = (block.super.nblocks / 10) + 1;
    block.super.ninodes = block.super.ninodeblocks * INODES_PER_BLOCK;

    disk->write(0, block.data); // Escreve o superbloco

    // Inicializa blocos de inodes
    for (int i = 1; i <= block.super.ninodeblocks; ++i) {
        memset(block.data, 0, Disk::DISK_BLOCK_SIZE);
        disk->write(i, block.data);
    }
    return 1;
}

// Monta o sistema de arquivos
int INE5412_FS::fs_mount() {
    fs_block block;
    disk->read(0, block.data);

    if (block.super.magic != FS_MAGIC) return 0;

    block_bitmap = new bool[block.super.nblocks];
    std::fill(block_bitmap, block_bitmap + block.super.nblocks, false);

    // Marca os blocos reservados como usados
    for (int i = 0; i <= block.super.ninodeblocks; ++i) {
        block_bitmap[i] = true;
    }
    mounted = true;

    return 1;
}

// Cria um novo inode
int INE5412_FS::fs_create() {
    if (!mounted) return -1;

    fs_block block;
    disk->read(0, block.data);
    int ninodeblocks = block.super.ninodeblocks;

    for (int i = 0; i < ninodeblocks; ++i) {
        disk->read(i + 1, block.data);
        for (int j = 0; j < INODES_PER_BLOCK; ++j) {
            if (!block.inode[j].isvalid) {
                block.inode[j].isvalid = 1;
                block.inode[j].size = 0;
                memset(block.inode[j].direct, 0, sizeof(block.inode[j].direct));
                block.inode[j].indirect = 0;

                disk->write(i + 1, block.data);
                return i * INODES_PER_BLOCK + j;
            }
        }
    }

    return -1;
}

// Deleta um inode e libera seus blocos
int INE5412_FS::fs_delete(int inumber) {
    if (!mounted) return 0;

    fs_inode inode;
    inode_load(inumber, &inode);
    if (!inode.isvalid) return 0;

    for (int i = 0; i < POINTERS_PER_INODE; ++i) {
        if (inode.direct[i]) free_block(inode.direct[i]);
    }

    if (inode.indirect) {
        fs_block indirect_block;
        disk->read(inode.indirect, indirect_block.data);
        for (int i = 0; i < POINTERS_PER_BLOCK; ++i) {
            if (indirect_block.pointers[i]) free_block(indirect_block.pointers[i]);
        }
        free_block(inode.indirect);
    }

    inode.isvalid = 0;
    inode_save(inumber, &inode);
    return 1;
}

// Retorna o tamanho do arquivo associado a um inode
int INE5412_FS::fs_getsize(int inumber) {
    if (!mounted) return -1;

    fs_inode inode;
    inode_load(inumber, &inode);
    if (!inode.isvalid) return -1;

    return inode.size;
}

// Lê dados de um inode
int INE5412_FS::fs_read(int inumber, char *data, int length, int offset) {
    if (!mounted) return 0;

    fs_inode inode;
    inode_load(inumber, &inode);
    if (!inode.isvalid) return 0;

    int bytes_read = 0;
    while (length > 0) {
        int block_offset = offset / Disk::DISK_BLOCK_SIZE;
        int block_pos = offset % Disk::DISK_BLOCK_SIZE;
        int to_read = std::min(length, Disk::DISK_BLOCK_SIZE - block_pos);

        fs_block block;
        if (block_offset < POINTERS_PER_INODE && inode.direct[block_offset]) {
            disk->read(inode.direct[block_offset], block.data);
        } else if (inode.indirect) {
            fs_block indirect_block;
            disk->read(inode.indirect, indirect_block.data);
            if (indirect_block.pointers[block_offset - POINTERS_PER_INODE]) {
                disk->read(indirect_block.pointers[block_offset - POINTERS_PER_INODE], block.data);
            }
        } else {
            break;
        }

        memcpy(data + bytes_read, block.data + block_pos, to_read);
        bytes_read += to_read;
        offset += to_read;
        length -= to_read;
    }

    return bytes_read;
}

// Escreve dados em um inode
int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset) {
    if (!mounted) return 0;

    fs_inode inode;
    inode_load(inumber, &inode);

    if (!inode.isvalid) return 0;

    int total_bytes_written = 0;
    int current_offset = offset;
    int remaining_length = length;

    while (remaining_length > 0) {
        int block_index = current_offset / Disk::DISK_BLOCK_SIZE;
        int block_offset = current_offset % Disk::DISK_BLOCK_SIZE;

        int block_num = 0;
        if (block_index < POINTERS_PER_INODE) {
            if (inode.direct[block_index] == 0) {
                inode.direct[block_index] = allocate_block();
                if (inode.direct[block_index] == 0) return total_bytes_written;
            }
            block_num = inode.direct[block_index];
        } else {
            if (inode.indirect == 0) {
                inode.indirect = allocate_block();
                if (inode.indirect == 0) return total_bytes_written;

                fs_block indirect_block = {};
                disk->write(inode.indirect, indirect_block.data);
            }
            fs_block indirect_block;
            disk->read(inode.indirect, indirect_block.data);

            int indirect_index = block_index - POINTERS_PER_INODE;
            if (indirect_block.pointers[indirect_index] == 0) {
                indirect_block.pointers[indirect_index] = allocate_block();
                if (indirect_block.pointers[indirect_index] == 0) return total_bytes_written;

                disk->write(inode.indirect, indirect_block.data);
            }
            block_num = indirect_block.pointers[indirect_index];
        }

        fs_block block;
        disk->read(block_num, block.data);

        int bytes_to_write = std::min(remaining_length, Disk::DISK_BLOCK_SIZE - block_offset);
        memcpy(block.data + block_offset, data + total_bytes_written, bytes_to_write);

        disk->write(block_num, block.data);

        total_bytes_written += bytes_to_write;
        current_offset += bytes_to_write;
        remaining_length -= bytes_to_write;
    }

    inode.size = std::max(inode.size, offset + length);
    inode_save(inumber, &inode);

    return total_bytes_written;
}

// Funções auxiliares
void INE5412_FS::inode_load(int inumber, fs_inode *inode) {
    fs_block block;
    int block_index = 1 + (inumber / INODES_PER_BLOCK);
    int inode_index = inumber % INODES_PER_BLOCK;
    disk->read(block_index, block.data);
    *inode = block.inode[inode_index];
}

void INE5412_FS::inode_save(int inumber, fs_inode *inode) {
    fs_block block;
    int block_index = 1 + (inumber / INODES_PER_BLOCK);
    int inode_index = inumber % INODES_PER_BLOCK;
    disk->read(block_index, block.data);
    block.inode[inode_index] = *inode;
    disk->write(block_index, block.data);
}

int INE5412_FS::allocate_block() {
    for (int i = 0; i < disk->size(); ++i) {
        if (!block_bitmap[i]) {
            block_bitmap[i] = true;
            return i;
        }
    }
    return 0;
}

void INE5412_FS::free_block(int blocknum) {
    block_bitmap[blocknum] = false;
}
