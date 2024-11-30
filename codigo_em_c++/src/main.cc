#include "gui.h" // Inclui o cabeçalho da interface gráfica
#include "shell.h" // Inclui o cabeçalho do modo terminal
#include "fs.h" // Inclui o cabeçalho do sistema de arquivos
#include <iostream> // Biblioteca para entrada e saída
#include <string> // Biblioteca para manipulação de strings
#include <limits> // Necessário para limpar o buffer de entrada

using namespace std;

// Função principal do programa
int main(int argc, char **argv) {
    // Verifica se o número de argumentos é válido (espera 3 argumentos)
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <diskfile> <nblocks>" << endl;
        return 1; // Retorna erro se os argumentos forem inválidos
    }

    // Converte o número de blocos fornecido como argumento para inteiro
    int nblocks = stoi(argv[2]);

    // O valor de blocos tem que ser no mínimo 3 (1 para superbloco, 1 para inodes, 1 dados)
    if (nblocks < 2) {
        std::cerr << "Error: Disk must have at least 2 blocks." << std::endl;
        return 1;
    }

    // Inicializa o disco com o arquivo e o número de blocos especificados
    Disk disk(argv[1], nblocks);

    // Inicializa o sistema de arquivos associado ao disco
    INE5412_FS fs(&disk);

    // Solicita ao usuário que selecione o modo de operação
    cout << "Select mode of operation:\n";
    cout << "1. Command Line Interface (CLI)\n";
    cout << "2. Graphical User Interface (GUI)\n";
    cout << "Enter your choice (1 or 2): ";

    int choice; // Armazena a escolha do usuário
    cin >> choice; // Lê a escolha do usuário

    // Limpa o buffer de entrada para evitar problemas com entradas pendentes
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // Verifica a escolha do usuário
    if (choice == 1) {
        // Modo (terminal)
        shell(&disk); // Chama a função do modo terminal (shell)
    } else if (choice == 2) {
        // Modo GUI
        GRAPHIC_INTERFACE::runGUI(fs); // Chama a função da interface gráfica
    } else {
        // Entrada inválida
        cerr << "Invalid choice. Exiting program.\n";
        disk.close(); // Fecha o disco antes de sair
        return 1; // Retorna erro
    }

    // Fecha o disco ao final da execução
    disk.close();
    return 0; // Retorna sucesso
}

//exemplos de entrada: ./bin/simplefs resources/image.200 200
