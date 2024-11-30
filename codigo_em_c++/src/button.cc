#include "../include/button.h"

using namespace std;


Button::Button(const string &label, const sf::Font &font, sf::Vector2f position) {
    shape.setSize({200, 50});
    shape.setPosition(position);
    shape.setFillColor(sf::Color::Blue);

    text.setFont(font);
    text.setString(label);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::White);
    text.setPosition(position.x + 10, position.y + 10);
}

void Button::draw(sf::RenderWindow &window) {
    window.draw(shape);
    window.draw(text);
}

bool Button::isClicked(sf::Vector2i mousePosition) {
    return shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition));
}
