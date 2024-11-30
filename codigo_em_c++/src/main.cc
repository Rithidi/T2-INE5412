#include "gui.h" // Inclui o cabeçalho da interface gráfica
#include "shell.h" // Inclui o cabeçalho do modo terminal
#include "fs.h" // Inclui o cabeçalho do sistema de arquivos
#include <iostream> // Biblioteca para entrada e saída
#include <string> // Biblioteca para manipulação de strings
#include <limits> // Necessário para limpar o buffer de entrada

// Função principal do programa
int main(int argc, char **argv) {
    // Verifica se o número de argumentos é válido (espera 3 argumentos)
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <diskfile> <nblocks>" << std::endl;
        return 1; // Retorna erro se os argumentos forem inválidos
    }

    // Converte o número de blocos fornecido como argumento para inteiro
    int nblocks = std::stoi(argv[2]);

    // Inicializa o disco com o arquivo e o número de blocos especificados
    Disk disk(argv[1], nblocks);

    // Inicializa o sistema de arquivos associado ao disco
    INE5412_FS fs(&disk);

    // Solicita ao usuário que selecione o modo de operação
    std::cout << "Select mode of operation:\n";
    std::cout << "1. Command Line Interface (CLI)\n";
    std::cout << "2. Graphical User Interface (GUI)\n";
    std::cout << "Enter your choice (1 or 2): ";

    int choice; // Armazena a escolha do usuário
    std::cin >> choice; // Lê a escolha do usuário

    // Limpa o buffer de entrada para evitar problemas com entradas pendentes
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Verifica a escolha do usuário
    if (choice == 1) {
        // Modo CLI (terminal)
        std::cout << "Starting in CLI mode...\n";
        shell(&disk); // Chama a função do modo terminal (shell)
    } else if (choice == 2) {
        // Modo GUI (interface gráfica)
        std::cout << "Starting in GUI mode...\n";
        runGUI(fs); // Chama a função da interface gráfica
    } else {
        // Entrada inválida
        std::cerr << "Invalid choice. Exiting program.\n";
        disk.close(); // Fecha o disco antes de sair
        return 1; // Retorna erro
    }

    // Fecha o disco ao final da execução
    disk.close();
    return 0; // Retorna sucesso
}
