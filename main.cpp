#include <iostream>

#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include "bass.h"

HSTREAM activeChannel, primary, secondary, out;

bool isPlaying = FALSE, dragging[2] = { FALSE, FALSE };

int songLengthInSeconds;

sf::RenderWindow window(sf::VideoMode(654, 244), "Songbird", sf::Style::None);

sf::RectangleShape cover(sf::Vector2f(200, 200)), rects[5], border[2], controls[3];

sf::Texture coverArt, controlTileset;

struct rV{ //WORKS!
	float sizeX;
	float sizeY;
	float posX;
	float posY;
	sf::Uint8 red;
	sf::Uint8 green;
	sf::Uint8 blue;
};

rV rectValues[5] = {
	{200, 15, 233, 177, 50, 50, 50}, //search bar --- 1 + 20 + 1 + 200 + 1 + 10 (horizontal) / 178 vertical
	{200, 15, 309, 207, 50, 50, 50}, //trackbar (background) --- 1 + 20 + 1 + 200 + 1 + 10 + 11 + 15 + 14 + 15 + 11 + 10 / 208 - 1 cause yes (vertical)
	{0, 15, 309, 207, 185, 185, 185}, //trackbar (itself)
	{54, 15, 579, 207, 50, 50, 50}, //volume bar (background)
	{54, 15, 579, 207, 185, 185, 185} //volume bar itself
};

rV borderValues[2] = {
	{652, 242, 1, 1, 30, 30, 30},
	{200, 200, 22, 22, 26, 26, 26}
};

/*void setupRect(sf::RectangleShape rect) {

}*/

int main() {
	/* ===== FRONT END - SETTING UP RECTS =====*/
	controlTileset.loadFromFile("assets/controls.png");

	for (unsigned int i = 0u; i < 3; ++i) {
		controls[i].setTexture(&controlTileset);
	}

	controls[0].setSize(sf::Vector2f(14, 13));
	controls[0].setTextureRect(sf::IntRect(0, 0, 14, 13));
	controls[0].setPosition(259, 208);

	for (unsigned int i = 1; i < 3; ++i) {
		controls[i].setSize(sf::Vector2f(11, 11));
		controls[i].setTextureRect(sf::IntRect(38, 0, 11, 11));
	}

	controls[1].setPosition(233, 209);
	controls[2].setPosition(299, 209); //+11

	controls[2].setScale(-1, 1);

	for (unsigned int i = 0u; i < 5; ++i) { //TODO - u?
		rects[i].setSize(sf::Vector2f(rectValues[i].sizeX, rectValues[i].sizeY));
		rects[i].setPosition(rectValues[i].posX, rectValues[i].posY);
		rects[i].setFillColor(sf::Color(rectValues[i].red, rectValues[i].green, rectValues[i].blue));
	}

	for (unsigned int i = 0u; i < sizeof(borderValues) / sizeof(*borderValues); ++i) { //TODO - tirar sizeof (os valores são definidos)
		border[i].setSize(sf::Vector2f(borderValues[i].sizeX, borderValues[i].sizeY));
		border[i].setPosition(borderValues[i].posX, borderValues[i].posY);
		border[i].setFillColor(sf::Color::Transparent);
		border[i].setOutlineColor(sf::Color(borderValues[i].red, borderValues[i].green, borderValues[i].blue));
		border[i].setOutlineThickness(1);
	}


	while (window.isOpen())
	{
		window.clear(sf::Color::Black);

		for (unsigned int i = 0u; i < 3; ++i) {
			window.draw(controls[i]);
		}

		for (unsigned int i = 0u; i < sizeof(borderValues) / sizeof(*borderValues); ++i) {
			window.draw(border[i]);
		}

		for (unsigned int i = 0u; i < 5; ++i) {
			window.draw(rects[i]);
		}

		window.display();
	}

	return 0;
}