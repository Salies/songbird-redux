#include <iostream>

#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include "bass.h"

HSTREAM activeChannel, primary, secondary, out;

bool isPlaying = FALSE, dragging[2] = { FALSE, FALSE }, inSearchBar = FALSE;

int songLengthInSeconds;

size_t searchSize;

sf::RenderWindow window(sf::VideoMode(654, 244), "Songbird", sf::Style::None);

sf::Event event;

sf::Vector2i mousePos;

sf::RectangleShape cover(sf::Vector2f(200, 200)), rects[5], border[2], controls[3];

sf::Text searchText, songTime, trackInfo[4];

sf::String searchInput;

sf::Font fonts[2];

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
	{0, 15, 579, 207, 185, 185, 185} //volume bar (itself)
};

rV borderValues[2] = {
	{652, 242, 1, 1, 30, 30, 30},
	{200, 200, 22, 22, 26, 26, 26}
};

/*void setupRect(sf::RectangleShape rect) {

}*/

int main() {
	/* ===== FRONT END - LOADING STUFF ===== */
	controlTileset.loadFromFile("assets/controls.png");
	fonts[0].loadFromFile("assets/fonts/OpenSans-Regular.ttf");
	fonts[1].loadFromFile("assets/fonts/OpenSans-Bold.ttf");

	/* ===== FRONT END - SETTING UP RECTS =====*/
	cover.setFillColor(sf::Color::Black);
	cover.setPosition(22, 22);

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

	for (unsigned int i = 0u; i < 2; ++i) { //TODO - tirar sizeof (os valores são definidos)
		border[i].setSize(sf::Vector2f(borderValues[i].sizeX, borderValues[i].sizeY));
		border[i].setPosition(borderValues[i].posX, borderValues[i].posY);
		border[i].setFillColor(sf::Color::Transparent);
		border[i].setOutlineColor(sf::Color(borderValues[i].red, borderValues[i].green, borderValues[i].blue));
		border[i].setOutlineThickness(1);
	}

	/* ===== FRONT END - SETTING UP TEXT =====*/
	searchText.setFont(fonts[0]);
	searchText.setFillColor(sf::Color::White);
	searchText.setCharacterSize(11);
	//searchText.setString("bola");
	searchText.setPosition(235, 178);


	while (window.isOpen())
	{
		/* ===== EVENTS ===== */
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left) {
					mousePos = sf::Mouse::getPosition(window);
					if(controls[0].getGlobalBounds().contains(mousePos.x, mousePos.y)) { //is there a better way to do this other than this ridiculous if? if so, please push a commit!
						std::cout << "play";
					}
					else if (controls[1].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						std::cout << "back";
					}
					else if (controls[2].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						std::cout << "forward";
					}
					else if (rects[1].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						std::cout << "trackbar";
					}
					else if (rects[3].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						std::cout << "volume";
					}
					
					if (rects[0].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						inSearchBar = TRUE;
						rects[0].setFillColor(sf::Color(185, 185, 185));
						searchText.setFillColor(sf::Color::Black);
					}
					else {
						if (inSearchBar) {
							inSearchBar = FALSE;
							rects[0].setFillColor(sf::Color(50, 50, 50));
							searchText.setFillColor(sf::Color::White);
						}
					}
				}
				break;

			case sf::Event::TextEntered:
				if (inSearchBar) {
					searchSize = searchInput.getSize();
					if (event.text.unicode == '\b') {
						if (searchSize) {
							searchInput.erase(searchSize - 1, 1); //VS pointing a possible overflow here (???)
							if (searchSize <= 27) {
								searchText.setString(searchInput);
							}
						}
					}
					else {
						searchInput += static_cast<char>(event.text.unicode);
						if (searchSize <= 27) {
							searchText.setString(searchInput);
						}
						else {
							searchText.setString(searchInput.substring(0, 25) + "...");
						}
					}
				}
				break;

			case sf::Event::KeyPressed:
				if (event.key.control && event.key.code == sf::Keyboard::V && inSearchBar) {
					searchInput = sf::Clipboard::getString();
					if (searchInput.getSize() <= 27) {
						searchText.setString(searchInput);
					}
					else {
						searchText.setString(searchInput.substring(0, 25) + "...");
					}
				}
				break;

			default:
				break;
			}
		};

		window.clear(sf::Color::Black);

		window.draw(cover);


		for (unsigned int i = 0u; i < 3; ++i) {
			window.draw(controls[i]);
		}

		for (unsigned int i = 0u; i < 2; ++i) {
			window.draw(border[i]);
		}

		for (unsigned int i = 0u; i < 5; ++i) {
			window.draw(rects[i]);
		}

		window.draw(searchText);

		window.display();
	}

	return 0;
}