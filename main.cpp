//SFML
#include <SFML/Graphics.hpp>
//BASS
#include "bass.h"
#include "bassflac.h"
//STRING
#include <string>
//HEADERS
#include "covers.h"
#include <taglib/fileref.h>
#include <taglib/apefile.h>
#include <taglib/apetag.h>
#include <taglib/asffile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/commentsframe.h>
#include <taglib/flacfile.h>
#include <taglib/id3v1genres.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpcfile.h>
#include <taglib/mpegfile.h>
#include <taglib/mp4file.h>
#include <taglib/tag.h>
#include <taglib/taglib.h>
#include <taglib/textidentificationframe.h>
#include <taglib/tstring.h>
#include <taglib/vorbisfile.h>

HSTREAM s;
bool isPlaying = FALSE;
const char* filePath = "SONG.SONGFILE";

int main()
{
	//oi();
	BASS_PluginLoad("bassflac.dll", 0);

	sf::RenderWindow window(sf::VideoMode(840, 480), "Songbird");
	sf::Texture buttonsTile;
	buttonsTile.loadFromFile("buttons.png");

	//play button
	sf::RectangleShape playButton(sf::Vector2f(50, 50));
	playButton.setTexture(&buttonsTile);
	playButton.setTextureRect(sf::IntRect(0, 0, 97, 112));
	playButton.setPosition(10.f, 30.f); //placeholder position

	//pause button
	sf::RectangleShape pauseButton(sf::Vector2f(50, 50));
	pauseButton.setTexture(&buttonsTile);
	pauseButton.setTextureRect(sf::IntRect(130, 0, 75, 112));
	pauseButton.setPosition(100.f, 30.f);

	//ref text
	sf::Font OpenSansBold;
	OpenSansBold.loadFromFile("OpenSans-Bold.ttf");
	sf::Text text;
	text.setFont(OpenSansBold);
	text.setString("Paused");
	text.setCharacterSize(24);
	text.setFillColor(sf::Color::White);
	text.setStyle(sf::Text::Bold);

	//song title
	sf::Text songTitle;
	songTitle.setFont(OpenSansBold);
	songTitle.setCharacterSize(14);
	songTitle.setFillColor(sf::Color::White);
	songTitle.setStyle(sf::Text::Bold);
	songTitle.setPosition(0, 100.f);

	//initializing player
	BASS_Init(-1, 44100, 0, 0, NULL);
	s = BASS_StreamCreateFile(FALSE, filePath, 0, 0, BASS_STREAM_AUTOFREE); //change the string for whatever filename
	TagLib::FileRef f(filePath);
	sf::String artist = f.tag()->artist().toWString();
	sf::String title = f.tag()->title().toWString();
	sf::String album = f.tag()->album().toWString();
	sf::String displayName = artist + " - " + title + " [from " + album + "]";
	window.setTitle(artist + " - " + title);
	songTitle.setString(displayName);

	sf::Texture testTexture;
	getCover(f, testTexture);

	sf::RectangleShape coverArt(sf::Vector2f(300, 300));
	coverArt.setTexture(&testTexture);
	coverArt.setPosition(10.f, 130.f);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
				// window closed
			case sf::Event::Closed:
				window.close();
				break;

			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left) {
					if (playButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) && isPlaying == FALSE) {
							isPlaying = TRUE;
							text.setString("Playing");
							BASS_ChannelPlay(s, FALSE);
					}

					if (pauseButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) && isPlaying == TRUE) {
							isPlaying = FALSE;
							text.setString("Paused");
							BASS_ChannelPause(s);
					}
				}
				break;

				// we don't process other types of events
			default:
				break;
			}
		}

		window.clear();
		//window.draw(sprite);
		window.draw(playButton);

		//drawing pause button
		window.draw(pauseButton);

		//draw reference text
		window.draw(text);

		//draw song title
		window.draw(songTitle);

		window.draw(coverArt);

		//window.draw(coverArt);

		window.display();
		//BASS_ChannelPlay(s, FALSE);
	}

	return 0;
}