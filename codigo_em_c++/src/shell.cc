#include "../include/fs.h"  // Inclui o cabeçalho do sistema de arquivos
#include <iostream>         // Biblioteca para entrada e saída
#include <sstream>          // Biblioteca para manipulação de strings e fluxos
#include <fstream>          // Biblioteca para manipulação de arquivos
#include <cstring>          // Biblioteca para operações com strings

class File_Ops {
public:
    static int do_copyin(const char *filename, int inumber, INE5412_FS *fs);
    static int do_copyout(int inumber, const char *filename, INE5412_FS *fs);
};

using namespace std; // Usa o namespace padrão

// Função cat: Lê e exibe o conteúdo de um inode no terminal
void cat(INE5412_FS &fs, int inumber) {
    // Verifica se o inode é válido
    if (fs.fs_getsize(inumber) < 0) {
        cerr << "Error: Invalid inode " << inumber << endl;
        return;
    }

    char buffer[Disk::DISK_BLOCK_SIZE]; // Buffer para armazenar os dados lidos
    int offset = 0;                     // Deslocamento inicial
    int bytes_read;

    // Lê os dados do inode em blocos
    while ((bytes_read = fs.fs_read(inumber, buffer, sizeof(buffer), offset)) > 0) {
        cout.write(buffer, bytes_read); // Escreve os dados no terminal
        offset += bytes_read;          // Incrementa o deslocamento
    }

    if (bytes_read < 0) {
        cerr << "Error reading inode " << inumber << endl; // Erro ao ler o inode
    } else {
        cout << endl; // Adiciona uma nova linha após exibir os dados
    }
}

// Função copyin: Copia um arquivo do sistema de arquivos host para um inode
int File_Ops::do_copyin(const char *filename, int inumber, INE5412_FS *fs) {
    // Abre o arquivo local para leitura em modo binário
    ifstream input(filename, ios::binary);
    if (!input) {
        cerr << "Error: Could not open file " << filename << endl;
        return 0; // Falha ao abrir o arquivo
    }

    // Verifica se o inode é válido
    if (fs->fs_getsize(inumber) < 0) {
        cerr << "Error: Invalid inode " << inumber << endl;
        input.close();
        return 0;
    }

    char buffer[Disk::DISK_BLOCK_SIZE]; // Buffer para os dados
    int offset = 0;                     // Deslocamento inicial

    // Lê o arquivo em blocos e escreve no inode
    while (input.read(buffer, sizeof(buffer))) {
        int bytes_written = fs->fs_write(inumber, buffer, sizeof(buffer), offset);
        if (bytes_written <= 0) {
            cerr << "Error writing to inode " << inumber << endl;
            input.close();
            return 0;
        }
        offset += bytes_written; // Incrementa o deslocamento
    }

    // Escreve quaisquer bytes restantes no final do arquivo
    if (input.gcount() > 0) {
        int bytes_written = fs->fs_write(inumber, buffer, input.gcount(), offset);
        if (bytes_written <= 0) {
            cerr << "Error writing to inode " << inumber << endl;
            input.close();
            return 0;
        }
    }

    input.close(); // Fecha o arquivo local
    cout << "File " << filename << " successfully copied to inode " << inumber << endl;
    return 1; // Sucesso
}

// Função copyout: Copia os dados de um inode para um arquivo local
int File_Ops::do_copyout(int inumber, const char *filename, INE5412_FS *fs) {
    // Verifica se o inode é válido
    if (fs->fs_getsize(inumber) < 0) {
        cerr << "Error: Invalid inode " << inumber << endl;
        return 0;
    }

    // Abre o arquivo local para escrita em modo binário
    ofstream output(filename, ios::binary);
    if (!output) {
        cerr << "Error: Could not open file " << filename << " for writing" << endl;
        return 0;
    }

    char buffer[Disk::DISK_BLOCK_SIZE]; // Buffer para os dados
    int offset = 0;                     // Deslocamento inicial
    int bytes_read;

    // Lê os dados do inode em blocos e escreve no arquivo
    while ((bytes_read = fs->fs_read(inumber, buffer, sizeof(buffer), offset)) > 0) {
        output.write(buffer, bytes_read); // Escreve os dados no arquivo local
        offset += bytes_read;            // Incrementa o deslocamento
    }

    if (bytes_read < 0) {
        cerr << "Error reading inode " << inumber << endl; // Erro ao ler o inode
    } else {
        cout << "Inode " << inumber << " successfully copied to file " << filename << endl;
    }

    output.close(); // Fecha o arquivo local
    return 1;       // Sucesso
}

// Função shell: Interface de linha de comando do SimpleFS
void shell(Disk *disk) {
    INE5412_FS fs(disk); // Inicializa o sistema de arquivos

    string line, command; // Variáveis para a linha de entrada e o comando
    while (true) {
        cout << "simplefs> "; // Exibe o prompt
        getline(cin, line);   // Lê a linha de entrada do usuário
        istringstream iss(line);
        iss >> command; // Extrai o comando

        if (command == "format") {
            cout << (fs.fs_format() ? "Disk formatted successfully!" : "Failed to format disk!") << endl;
        } else if (command == "mount") {
            cout << (fs.fs_mount() ? "Disk mounted successfully!" : "Failed to mount disk!") << endl;
        } else if (command == "debug") {
            fs.fs_debug(); // Mostra o estado do sistema de arquivos
        } else if (command == "create") {
            int inumber = fs.fs_create();
            cout << (inumber >= 0 ? "File created with inode number " + to_string(inumber) : "Failed to create file!") << endl;
        } else if (command == "delete") {
            int inumber;
            iss >> inumber;
            cout << (fs.fs_delete(inumber) ? "File deleted successfully!" : "Failed to delete file!") << endl;
        } else if (command == "cat") {
            int inumber;
            iss >> inumber;
            cat(fs, inumber); // Exibe o conteúdo do inode
        } else if (command == "copyin") {
            string filename;
            int inumber;
            iss >> filename >> inumber;
            File_Ops::do_copyin(filename.c_str(), inumber, &fs);
        } else if (command == "copyout") {
            int inumber;
            string filename;
            iss >> inumber >> filename;
            File_Ops::do_copyout(inumber, filename.c_str(), &fs);
        } else if (command == "help") {
            cout << "Commands are:\n"
                 << "  format\n"
                 << "  mount\n"
                 << "  debug\n"
                 << "  create\n"
                 << "  delete <inode>\n"
                 << "  cat <inode>\n"
                 << "  copyin <file> <inode>\n"
                 << "  copyout <inode> <file>\n"
                 << "  help\n"
                 << "  quit\n"
                 << "  exit\n";
        } else if (command == "quit" || command == "exit") {
            break;
        } else {
            cerr << "Invalid command!" << endl;
        }
    }
}
