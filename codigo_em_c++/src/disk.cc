#include "disk.h"
#include <stdexcept>
#include <unistd.h>

// Construtor da classe Disk
// Inicializa o arquivo de disco e configura o número de blocos.
Disk::Disk(const char *filename, int nblocks) : nblocks(nblocks) {
    // Abre o arquivo de disco no modo "append + binário"
    diskfile = fopen(filename, "a+b");
    if (!diskfile) throw std::runtime_error("Falhou para abrir o disk file"); // Erro se o arquivo não for aberto

    // Fecha temporariamente o arquivo
    fclose(diskfile);

    // Ajusta o tamanho do arquivo para o tamanho total necessário (número de blocos * tamanho de um bloco)
    if (truncate(filename, DISK_BLOCK_SIZE * nblocks) != 0) {
        throw std::runtime_error("Falhou para set file size"); // Erro se o ajuste falhar
    }

    // Reabre o arquivo no modo "leitura + escrita binária"
    diskfile = fopen(filename, "r+b");
    if (!diskfile) throw std::runtime_error("falhou para reabrir disk file"); // Erro se o arquivo não puder ser reaberto
}

// Retorna o número de blocos do disco
int Disk::size() { return nblocks; }

// Faz uma verificação de sanidade nos parâmetros fornecidos para leitura/escrita
void Disk::sanity_check(int blocknum, const void *data) {
    // Verifica se o ponteiro de dados não é nulo e se o número do bloco está dentro dos limites válidos
    if (!data || blocknum < 0 || blocknum >= nblocks) {
        throw std::runtime_error("Disk sanity check falhou"); // Lança erro se as condições não forem atendidas
    }
}

// Lê dados de um bloco do disco
void Disk::read(int blocknum, char *data) {
    sanity_check(blocknum, data); // Verifica os parâmetros
    fseek(diskfile, blocknum * DISK_BLOCK_SIZE, SEEK_SET); // Move o ponteiro do arquivo para o bloco correspondente
    fread(data, DISK_BLOCK_SIZE, 1, diskfile); // Lê um bloco de dados do arquivo
}

// Escreve dados em um bloco do disco
void Disk::write(int blocknum, const char *data) {
    sanity_check(blocknum, data); // Verifica os parâmetros
    fseek(diskfile, blocknum * DISK_BLOCK_SIZE, SEEK_SET); // Move o ponteiro do arquivo para o bloco correspondente
    fwrite(data, DISK_BLOCK_SIZE, 1, diskfile); // Escreve um bloco de dados no arquivo
}

// Fecha o arquivo de disco
void Disk::close() {
    if (diskfile) fclose(diskfile); // Fecha o arquivo, se estiver aberto
}