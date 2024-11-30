#include "../include/fs.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

using namespace std;

// Função de depuração que exibe o estado do sistema de arquivos
void INE5412_FS::fs_debug() {
    fs_block block;
    disk->read(0, block.data); // Lê o superbloco

    cout << "superblock:" << endl;
    if (block.super.magic == FS_MAGIC) {
        cout << "    magic number is valid" << endl;
    } else {
        cout << "    invalid magic number" << endl;
        return;
    }

    // Informações do superbloco
    cout << "    " << block.super.nblocks << " blocks on disk" << endl;
    cout << "    " << block.super.ninodeblocks << " blocks for inodes" << endl;
    cout << "    " << block.super.ninodes << " inodes total" << endl;

    // Itera pelos blocos de inodes
    for (int i = 0; i < block.super.ninodeblocks; ++i) {
        disk->read(i + 1, block.data);
        for (int j = 0; j < INODES_PER_BLOCK; ++j) {
            fs_inode &inode = block.inode[j];
            if (inode.isvalid) {
                cout << "inode " << (i * INODES_PER_BLOCK + j) << ":" << endl;
                cout << "    size: " << inode.size << " bytes" << endl;
                cout << "    direct blocks:";
                for (int k = 0; k < POINTERS_PER_INODE; ++k) {
                    if (inode.direct[k]) cout << " " << inode.direct[k];
                }
                cout << endl;
                if (inode.indirect) {
                    cout << "    indirect block: " << inode.indirect << endl;
                    fs_block indirect_block;
                    disk->read(inode.indirect, indirect_block.data);
                    cout << "    indirect data blocks:";
                    for (int k = 0; k < POINTERS_PER_BLOCK; ++k) {
                        if (indirect_block.pointers[k]) cout << " " << indirect_block.pointers[k];
                    }
                    cout << endl;
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

    disk->write(0, block.data); // Escreve o superbloco no bloco 0

    // Inicializa blocos de inodes
    for (int i = 1; i <= block.super.ninodeblocks; ++i) {
        memset(block.data, 0, Disk::DISK_BLOCK_SIZE); // Zera o conteúdo do bloco
        disk->write(i, block.data);
    }

    // mounted = 0;

    return 1;

}

int INE5412_FS::fs_mount() {
    fs_block block;
    disk->read(0, block.data); // Lê o superbloco

    if (block.super.magic != FS_MAGIC) return 0;

    // Inicializa o bitmap para rastrear os blocos usados
    block_bitmap = new bool[block.super.nblocks];
    std::fill(block_bitmap, block_bitmap + block.super.nblocks, false);

    // Marca os blocos reservados (superbloco e inodes) como usados
    for (int i = 0; i <= block.super.ninodeblocks; ++i) {
        block_bitmap[i] = true;
    }

    // Verifica os inodes e marca os blocos associados como usados
    for (int i = 0; i < block.super.ninodeblocks; ++i) {
        disk->read(i + 1, block.data);
        for (int j = 0; j < INODES_PER_BLOCK; ++j) {
            fs_inode &inode = block.inode[j];
            if (inode.isvalid) {
                // Marca blocos diretos como usados
                for (int k = 0; k < POINTERS_PER_INODE; ++k) {
                    if (inode.direct[k]) {
                        block_bitmap[inode.direct[k]] = true;
                    }
                }
                // Marca blocos indiretos como usados
                if (inode.indirect) {
                    block_bitmap[inode.indirect] = true;

                    fs_block indirect_block;
                    disk->read(inode.indirect, indirect_block.data);
                    for (int k = 0; k < POINTERS_PER_BLOCK; ++k) {
                        if (indirect_block.pointers[k]) {
                            block_bitmap[indirect_block.pointers[k]] = true;
                        }
                    }
                }
            }
        }
    }

    mounted = true; // Marca o sistema como montado
    return 1;
}


// Cria um novo inode
int INE5412_FS::fs_create() {
    if (!mounted) return -1;

    // Ler o superbloco
    fs_block block;
    disk->read(0, block.data);
    int ninodeblocks = block.super.ninodeblocks;  // Pega a quantidade de blocos para inodes

    for (int i = 0; i < ninodeblocks; ++i) {
        disk->read(i + 1, block.data); // Le a partir do bloco 1
        for (int j = 0; j < INODES_PER_BLOCK; ++j) {
            if (!block.inode[j].isvalid) {
                block.inode[j].isvalid = 1; // Marca o inode como válido
                block.inode[j].size = 0; // Inicializa o tamanho como 0
                memset(block.inode[j].direct, 0, sizeof(block.inode[j].direct)); // Zera os ponteiros diretos
                block.inode[j].indirect = 0; // Zera o ponteiro indireto

                disk->write(i + 1, block.data); // Atualiza o bloco de inodes no disco
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
    inode_load(inumber, &inode); // Carrega o inode indicado
    if (!inode.isvalid) return 0;

    // Itera sobre os ponteiros diretos
    for (int i = 0; i < POINTERS_PER_INODE; ++i) { 
        if (inode.direct[i]) free_block(inode.direct[i]); // se estiveer alocado, marca o bloco como livre
    }

    if (inode.indirect) {
        fs_block indirect_block;
        disk->read(inode.indirect, indirect_block.data); // Lê o bloco indireto
        for (int i = 0; i < POINTERS_PER_BLOCK; ++i) {
            if (indirect_block.pointers[i]) free_block(indirect_block.pointers[i]); // Marca como livre todos os blocos apontados
        }
        free_block(inode.indirect); // Marca como livre o próprio bloco indireto
    }

    inode.isvalid = 0;
    inode_save(inumber, &inode);
    return 1;
}

// Retorna o tamanho do arquivo associado a um inode
int INE5412_FS::fs_getsize(int inumber) {

    fs_inode inode;
    inode_load(inumber, &inode);
    if (!inode.isvalid) return -1;

    return inode.size;
}

int INE5412_FS::fs_read(int inumber, char *data, int length, int offset) {
    if (!mounted) return 0; // Sistema de arquivos precisa estar montado

    fs_inode inode;
    inode_load(inumber, &inode); // Carrega o inode especificado
    if (!inode.isvalid) return 0; // Verifica se o inode é válido

    int bytes_read = 0; // Total de bytes lidos
    while (length > 0) { // Enquanto houver bytes para ler
        if (offset >= inode.size) {
            break; // Offset excede o tamanho do arquivo
        }

        int block_offset = offset / Disk::DISK_BLOCK_SIZE; // Qual bloco acessar
        int block_pos = offset % Disk::DISK_BLOCK_SIZE;    // Posição inicial no bloco
        int to_read = std::min(length, Disk::DISK_BLOCK_SIZE - block_pos); // Quantidade de bytes a ler

        fs_block block = {}; // Inicializa o bloco para evitar resíduos

        // Verifica se o bloco está nos limites dos ponteiros diretos
        if (block_offset < POINTERS_PER_INODE) {
            if (inode.direct[block_offset] == 0 || inode.direct[block_offset] >= disk->size()) {
                std::cerr << "Error: Direct block " << block_offset 
                          << " is not allocated or out of bounds." << std::endl;
                break;
            }
            disk->read(inode.direct[block_offset], block.data);

        // Caso contrário, verifica os blocos indiretos
        } else if (inode.indirect) {
            if (inode.indirect == 0 || inode.indirect >= disk->size()) {
                std::cerr << "Error: Indirect block " << inode.indirect 
                          << " is not allocated or out of bounds." << std::endl;
                break;
            }

            fs_block indirect_block;
            disk->read(inode.indirect, indirect_block.data);

            int indirect_index = block_offset - POINTERS_PER_INODE;

            // Verifica se o índice está dentro dos limites do bloco indireto
            if (indirect_index < 0 || indirect_index >= POINTERS_PER_BLOCK) {
                std::cerr << "Error: Indirect index " << indirect_index 
                          << " is out of bounds." << std::endl;
                break;
            }

            // Verifica se o ponteiro no bloco indireto é válido
            if (indirect_block.pointers[indirect_index] == 0 || indirect_block.pointers[indirect_index] >= disk->size()) {
                std::cerr << "Error: Indirect data block " << indirect_block.pointers[indirect_index]
                          << " is not allocated or out of bounds." << std::endl;
                break;
            }

            disk->read(indirect_block.pointers[indirect_index], block.data);
        } else {
            break; // Nenhum bloco válido encontrado
        }

        // Copia os dados lidos para o buffer de saída
        memcpy(data + bytes_read, block.data + block_pos, to_read);

        // Atualiza os contadores
        bytes_read += to_read;
        offset += to_read;
        length -= to_read;
    }

    return bytes_read; // Retorna o total de bytes lidos
}


// Escreve dados em um inode
int INE5412_FS::fs_write(int inumber, const char *data, int length, int offset) {
    // Verifica se o sistema de arquivos está montado
    if (!mounted) return 0;

    // Carrega o inode especificado
    fs_inode inode;
    inode_load(inumber, &inode);

    // Verifica se o inode é válido
    if (!inode.isvalid) return 0;

    // Inicializa variáveis para rastrear progresso da escrita
    int total_bytes_written = 0;   // Total de bytes escritos
    int current_offset = offset;  // Posição atual no arquivo
    int remaining_length = length; // Dados restantes para escrever

    // Escreve os dados enquanto houver bytes a serem processados
    while (remaining_length > 0) {
        // Calcula o índice do bloco e a posição dentro do bloco
        int block_index = current_offset / Disk::DISK_BLOCK_SIZE;
        int block_offset = current_offset % Disk::DISK_BLOCK_SIZE;

        int block_num = 0; // Número do bloco onde escreveremos os dados

        // Lida com blocos diretos
        if (block_index < POINTERS_PER_INODE) {
            // Se o bloco direto ainda não foi alocado, aloque-o
            if (inode.direct[block_index] == 0) {
                inode.direct[block_index] = allocate_block();
                if (inode.direct[block_index] == 0) return total_bytes_written; // Falha ao alocar bloco
            }
            block_num = inode.direct[block_index];
        } else { 
            // Caso os blocos diretos acabem, usa blocos indiretos
            if (inode.indirect == 0) { // Se o bloco indireto não existe, aloque-o
                inode.indirect = allocate_block();
                if (inode.indirect == 0) return total_bytes_written; // Falha ao alocar bloco indireto

                // Inicializa o bloco indireto com 0
                fs_block indirect_block = {};
                disk->write(inode.indirect, indirect_block.data);
            }

            // Lê o bloco indireto do disco
            fs_block indirect_block;
            disk->read(inode.indirect, indirect_block.data);

            // Calcula o índice dentro do bloco indireto
            int indirect_index = block_index - POINTERS_PER_INODE;

            // Aloca um novo bloco de dados, se necessário
            if (indirect_block.pointers[indirect_index] == 0) {
                indirect_block.pointers[indirect_index] = allocate_block();
                if (indirect_block.pointers[indirect_index] == 0) return total_bytes_written; // Falha ao alocar

                // Atualiza o bloco indireto no disco
                disk->write(inode.indirect, indirect_block.data);
            }
            block_num = indirect_block.pointers[indirect_index];
        }

        // Lê o bloco onde os dados serão escritos
        fs_block block;
        disk->read(block_num, block.data);

        // Calcula o número de bytes a serem escritos no bloco atual
        int bytes_to_write = min(remaining_length, Disk::DISK_BLOCK_SIZE - block_offset);

        // Escreve os dados no bloco
        memcpy(block.data + block_offset, data + total_bytes_written, bytes_to_write);

        // Salva o bloco atualizado no disco
        disk->write(block_num, block.data);

        // Atualiza os contadores de progresso
        total_bytes_written += bytes_to_write;
        current_offset += bytes_to_write;
        remaining_length -= bytes_to_write;
    }

    // Atualiza o tamanho do arquivo no inode, se necessário
    inode.size = max(inode.size, offset + length);
    inode_save(inumber, &inode); // Salva o inode atualizado no disco

    // Retorna o total de bytes escritos
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
        if (!block_bitmap[i]) { // Verifica se o bloco está livre
            block_bitmap[i] = true; // Marca o bloco como usado
            return i; // Retorna o número do bloco alocado
        }
    }
    std::cerr << "No free blocks available!" << std::endl;
    return 0; // Sem blocos livres disponíveis
}


void INE5412_FS::free_block(int blocknum) {
    block_bitmap[blocknum] = false;
}
