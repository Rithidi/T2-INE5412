#include "../include/gui.h"
#include "../include/button.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace GRAPHIC_INTERFACE { 

    void runGUI(INE5412_FS &fs) {
        sf::RenderWindow window(sf::VideoMode(800, 800), "SimpleFS Interface");

        sf::Font font;
        if (!font.loadFromFile("resources/arial.ttf")) {
            cerr << "Failed to load font!" << endl;
            return;
        }

        // Botões
        Button formatButton("Format Disk", font, {40, 50});
        Button mountButton("Mount Disk", font, {40, 100});
        Button debugButton("Debug", font, {40, 150});
        Button createButton("Create File", font, {40, 200});
        Button deleteButton("Delete File", font, {40, 250});
        Button copyinButton("Copy In", font, {40, 300});
        Button copyoutButton("Copy Out", font, {40, 350});
        Button catButton("Cat", font, {40, 400});
        Button helpButton("Help", font, {40, 450});
        Button quitButton("Quit", font, {40, 500});
        Button exitButton("Exit", font , {40, 550});
        

        // Campo de entrada
        sf::Text inputLabel("Input:", font, 20);
        inputLabel.setPosition(300, 50);
        inputLabel.setFillColor(sf::Color::Black);

        sf::RectangleShape inputBox({400, 30});
        inputBox.setPosition(300, 80);
        inputBox.setFillColor(sf::Color::White);
        inputBox.setOutlineThickness(1);
        inputBox.setOutlineColor(sf::Color::Black);

        sf::Text inputText("", font, 20);
        inputText.setPosition(310, 85);
        inputText.setFillColor(sf::Color::Black);
        string inputBuffer;

        // Área de saída
        sf::Text output("", font, 20);
        output.setPosition(300, 150);
        output.setFillColor(sf::Color::Black);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();

                // Entrada de texto
                if (event.type == sf::Event::TextEntered) {
                    if (event.text.unicode == '\b') { // Backspace
                        if (!inputBuffer.empty()) inputBuffer.pop_back();
                    } else if (event.text.unicode == '\r') { // Enter (ignorado)
                    } else if (event.text.unicode < 128) { // Apenas caracteres ASCII
                        inputBuffer += static_cast<char>(event.text.unicode);
                    }
                    inputText.setString(inputBuffer);
                }

                // Detecção de cliques nos botões
                if (event.type == sf::Event::MouseButtonPressed) {
                    auto mousePosition = sf::Mouse::getPosition(window);

                    if (formatButton.isClicked(mousePosition)) {
                        if (fs.fs_format()) {
                            output.setString("Disk formatted successfully!");
                        } else {
                            output.setString("Failed to format disk.");
                        }
                    } else if (mountButton.isClicked(mousePosition)) {
                        if (fs.fs_mount()) {
                            output.setString("Disk mounted successfully!");
                        } else {
                            output.setString(" mount disk.");
                        }
                    } else if (debugButton.isClicked(mousePosition)) {
                        fs.fs_debug();  // Presume-se que fs_debug imprime no console
                        output.setString("Debug output generated in terminal.");
                    } else if (createButton.isClicked(mousePosition)) {
                        int inumber = fs.fs_create();
                        if (inumber >= 0) {
                            output.setString("File created with inode " + to_string(inumber));
                        } else {
                            output.setString("Failed to create file.");
                        }
                    } else if (deleteButton.isClicked(mousePosition)) {
                        try {
                            int inumber = stoi(inputBuffer);
                            if (fs.fs_delete(inumber)) {
                                output.setString("File deleted successfully.");
                            } else {
                                output.setString("Failed to delete file.");
                            }
                        } catch (...) {
                            output.setString("Invalid input for inode.");
                        }
                    } else if (helpButton.isClicked(mousePosition)) {
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
                    } else if (copyinButton.isClicked(mousePosition)) {
                        size_t separator = inputBuffer.find(' ');
                        if (separator != string::npos) {
                            string filename = inputBuffer.substr(0, separator);
                            int inumber = stoi(inputBuffer.substr(separator + 1));
                            ifstream input(filename, ios::binary);
                            if (input) {
                                input.seekg(0, ios::end);
                                int filesize = input.tellg();
                                input.seekg(0, ios::beg);

                                char *buffer = new char[filesize];
                                input.read(buffer, filesize);
                                input.close();

                                int written = fs.fs_write(inumber, buffer, filesize, 0);
                                delete[] buffer;

                                if (written == filesize) {
                                    output.setString("File copied to inode " + to_string(inumber));
                                } else {
                                    output.setString("Failed to copy file.");
                                }
                            } else {
                                output.setString("Invalid file: " + filename);
                            }
                        } else {
                            output.setString("Usage: <file> <inode>");
                        }
                    } else if (copyoutButton.isClicked(mousePosition)) {
                        size_t separator = inputBuffer.find(' ');
                        if (separator != string::npos) {
                            int inumber = stoi(inputBuffer.substr(0, separator));
                            string filename = inputBuffer.substr(separator + 1);

                            int filesize = fs.fs_getsize(inumber);
                            if (filesize >= 0) {
                                char *buffer = new char[filesize];
                                int read = fs.fs_read(inumber, buffer, filesize, 0);

                                if (read == filesize) {
                                    ofstream outputFile(filename, ios::binary);
                                    outputFile.write(buffer, filesize);
                                    outputFile.close();
                                    output.setString("File saved as " + filename);
                                } else {
                                    output.setString("Failed to read inode.");
                                }
                                delete[] buffer;
                            } else {
                                output.setString("Invalid inode.");
                            }
                        } else {
                            output.setString("Usage: <inode> <file>");
                        }
                    } else if (catButton.isClicked(mousePosition)) {
                        try {
                            int inumber = stoi(inputBuffer);
                            char buffer[Disk::DISK_BLOCK_SIZE];
                            int offset = 0;
                            ostringstream fileContent;

                            while (int bytes_read = fs.fs_read(inumber, buffer, sizeof(buffer), offset)) {
                                if (bytes_read < 0) {
                                    output.setString("Error reading inode.");
                                    return;
                                }
                                fileContent.write(buffer, bytes_read);
                                offset += bytes_read;
                            }
                            output.setString(fileContent.str());
                        } catch (...) {
                            output.setString("Invalid input for inode.");
                        }
                    } else if (quitButton.isClicked(mousePosition) || exitButton.isClicked(mousePosition)) {
                        window.close();  // Fecha a janela
                    }
                }
            }

            // Desenha a interface
            window.clear(sf::Color::White);
            formatButton.draw(window);
            mountButton.draw(window);
            debugButton.draw(window);
            createButton.draw(window);
            deleteButton.draw(window);
            copyinButton.draw(window);
            copyoutButton.draw(window);
            catButton.draw(window);
            helpButton.draw(window);
            quitButton.draw(window);
            exitButton.draw(window);
            window.draw(inputLabel);
            window.draw(inputBox);
            window.draw(inputText);
            window.draw(output);
            window.display();
        }
    }
}
