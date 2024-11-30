#include "../include/fs.h" // Inclui o cabeçalho do sistema de arquivos
#include <iostream> // Biblioteca para entrada e saída
#include <sstream> // Biblioteca para manipulação de strings e fluxos
#include <fstream> // Biblioteca para manipulação de arquivos
#include <cstring> // Biblioteca para operações com strings

using namespace std; // Usa o namespace padrão

// Função cat: Lê e exibe o conteúdo de um inode no terminal
void cat(INE5412_FS &fs, int inumber) {
    // Verifica se o inode é válido
    if (fs.fs_getsize(inumber) < 0) {
        std::cerr << "Error: Invalid inode " << inumber << std::endl;
        return;
    }

    char buffer[Disk::DISK_BLOCK_SIZE]; // Buffer para armazenar os dados lidos
    int offset = 0; // Deslocamento inicial
    int bytes_read;

    // Lê os dados do inode em blocos
    while ((bytes_read = fs.fs_read(inumber, buffer, sizeof(buffer), offset)) > 0) {
        std::cout.write(buffer, bytes_read); // Escreve os dados no terminal
        offset += bytes_read; // Incrementa o deslocamento
    }

    if (bytes_read < 0) {
        // Erro ao ler o inode
        std::cerr << "Error reading inode " << inumber << std::endl;
    } else {
        // Adiciona uma nova linha após exibir os dados
        std::cout << std::endl;
    }
}

// Função copyin: Copia um arquivo do sistema de arquivos host para um inode
void copyin(INE5412_FS &fs, const char *filename, int inumber) {
    // Abre o arquivo local para leitura em modo binário
    std::ifstream input(filename, std::ios::binary);
    if (!input) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    // Verifica se o inode é válido
    if (fs.fs_getsize(inumber) < 0) {
        std::cerr << "Error: Invalid inode " << inumber << std::endl;
        return;
    }

    char buffer[Disk::DISK_BLOCK_SIZE]; // Buffer para os dados
    int offset = 0; // Deslocamento inicial

    // Lê o arquivo em blocos e escreve no inode
    while (input.read(buffer, sizeof(buffer))) {
        int bytes_written = fs.fs_write(inumber, buffer, input.gcount(), offset);
        if (bytes_written <= 0) {
            std::cerr << "Error writing to inode " << inumber << std::endl;
            return;
        }
        offset += bytes_written; // Incrementa o deslocamento
    }

    // Escreve quaisquer bytes restantes no final do arquivo
    if (input.gcount() > 0) {
        int bytes_written = fs.fs_write(inumber, buffer, input.gcount(), offset);
        if (bytes_written <= 0) {
            std::cerr << "Error writing to inode " << inumber << std::endl;
            return;
        }
    }

    input.close(); // Fecha o arquivo local
    std::cout << "File " << filename << " successfully copied to inode " << inumber << std::endl;
}

// Função copyout: Copia os dados de um inode para um arquivo local
void copyout(INE5412_FS &fs, int inumber, const char *filename) {
    // Verifica se o inode é válido
    if (fs.fs_getsize(inumber) < 0) {
        std::cerr << "Error: Invalid inode " << inumber << std::endl;
        return;
    }

    // Abre o arquivo local para escrita em modo binário
    std::ofstream output(filename, std::ios::binary);
    if (!output) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return;
    }

    char buffer[Disk::DISK_BLOCK_SIZE]; // Buffer para os dados
    int offset = 0; // Deslocamento inicial
    int bytes_read;

    // Lê os dados do inode em blocos e escreve no arquivo
    while ((bytes_read = fs.fs_read(inumber, buffer, sizeof(buffer), offset)) > 0) {
        output.write(buffer, bytes_read); // Escreve os dados no arquivo local
        offset += bytes_read; // Incrementa o deslocamento
    }

    if (bytes_read < 0) {
        // Erro ao ler o inode
        std::cerr << "Error reading inode " << inumber << std::endl;
    } else {
        // Sucesso na cópia
        std::cout << "Inode " << inumber << " successfully copied to file " << filename << std::endl;
    }

    output.close(); // Fecha o arquivo local
}

// Função shell: Interface de linha de comando do SimpleFS
void shell(Disk *disk) {
    INE5412_FS fs(disk); // Inicializa o sistema de arquivos

    string line, command; // Variáveis para a linha de entrada e o comando
    while (true) {
        cout << "simplefs> "; // Exibe o prompt
        getline(cin, line); // Lê a linha de entrada do usuário
        istringstream iss(line); // Cria um stream para a linha
        iss >> command; // Extrai o comando

        // Processa os comandos
        if (command == "format") {
            if (fs.fs_format()) {
                cout << "Disk formatted successfully!" << endl;
            } else {
                cout << "Failed to format disk!" << endl;
            }
        } else if (command == "mount") {
            if (fs.fs_mount()) {
                cout << "Disk mounted successfully!" << endl;
            } else {
                cout << "Failed to mount disk!" << endl;
            }
        } else if (command == "debug") {
            fs.fs_debug(); // Mostra o estado do sistema de arquivos
        } else if (command == "create") {
            int inumber = fs.fs_create();
            if (inumber >= 0) {
                cout << "File created with inode number " << inumber << endl;
            } else {
                cout << "Failed to create file!" << endl;
            }
        } else if (command == "delete") {
            int inumber;
            iss >> inumber;
            if (fs.fs_delete(inumber)) {
                cout << "File deleted successfully!" << endl;
            } else {
                cout << "Failed to delete file!" << endl;
            }
        } else if (command == "cat") {
            int inumber;
            iss >> inumber;
            cat(fs, inumber); // Exibe o conteúdo do inode
        } else if (command == "copyin") {
            string filename;
            int inumber;
            iss >> filename >> inumber;
            copyin(fs, filename.c_str(), inumber); // Copia o arquivo local para o inode
        } else if (command == "copyout") {
            int inumber;
            string filename;
            iss >> inumber >> filename;
            copyout(fs, inumber, filename.c_str()); // Copia o inode para um arquivo local
        } else if (command == "help") {
            // Exibe a lista de comandos
            cout << "Commands are:" << endl;
            cout << "  format" << endl;
            cout << "  mount" << endl;
            cout << "  debug" << endl;
            cout << "  create" << endl;
            cout << "  delete <inode>" << endl;
            cout << "  cat <inode>" << endl;
            cout << "  copyin <file> <inode>" << endl;
            cout << "  copyout <inode> <file>" << endl;
            cout << "  help" << endl;
            cout << "  quit" << endl;
            cout << "  exit" << endl;
        } else if (command == "quit" || command == "exit") {
            break; // Sai do loop e encerra o programa
        } else {
            // Comando inválido
            cout << "Invalid command!" << endl;
        }
    }
}
