#ifndef BUTTON_H
#define BUTTON_H

#include <SFML/Graphics.hpp>

class Button {
public:
    Button(const std::string &label, const sf::Font &font, sf::Vector2f position);
    void draw(sf::RenderWindow &window);
    bool isClicked(sf::Vector2i mousePosition);

private:
    sf::RectangleShape shape;
    sf::Text text;
};

#endif
