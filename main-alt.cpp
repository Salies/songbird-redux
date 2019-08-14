#include <iostream>
#include <fmt/core.h> //necessary?
#include <fmt/format.h>
//STRING
#include <string>
//SFML
#include <SFML/Graphics.hpp>
//BASS
#include "bass.h"
#include "bassflac.h"
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
bool holding = FALSE;
const char* filePath = "01 - Rising.mp3";
sf::Clock timer;

float resizeBar;
float resizeVolumeBar;
float volume = 1;
QWORD k;

QWORD currentPosition;
std::string currentSongTime;

sf::RenderWindow window(sf::VideoMode(840, 480), "Songbird");

sf::RectangleShape volumeBar(sf::Vector2f(150, 20));
sf::RectangleShape volumeSlider(sf::Vector2f(150, 20));

sf::RectangleShape trackBar(sf::Vector2f(250, 20));
sf::RectangleShape progressBar(sf::Vector2f(0, 20));

sf::String artist;
sf::String title;
sf::String album;
sf::String displayName;

sf::Text songTitle;
sf::Text songTime;
sf::Text text;

std::string toHourFormat(int seconds) {
	int minutes = seconds / 60; //int works! (rounds to less) / equivalent of Math.floor() method
	int remainingSeconds = seconds - minutes * 60;
	std::string fmtSeconds = fmt::format("{:02d}", remainingSeconds);
	std::string formatedTime = std::to_string(minutes) + ":" + fmtSeconds;
	return formatedTime;
}

void updateTime(QWORD current, int songLengthS, sf::Text& text) {
	std::string ctime = toHourFormat(BASS_ChannelBytes2Seconds(s, current));
	text.setString(ctime + "/" + toHourFormat(songLengthS)); //TODO CHANGE THIS
}

void updateVolume(HSTREAM channel) {
	resizeVolumeBar = sf::Mouse::getPosition(window).x - volumeBar.getPosition().x;
	volume = resizeVolumeBar / 150;
	BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, volume);
	volumeSlider.setSize(sf::Vector2f(resizeVolumeBar, 20));
}

void setInfo(TagLib::FileRef file) {
	artist = file.tag()->artist().toWString();
	title = file.tag()->title().toWString();
	album = file.tag()->album().toWString();
	displayName = artist + " - " + title + " [from " + album + "]";
	window.setTitle(artist + " - " + title);
	songTitle.setString(displayName);
}

int getChannelLengthInSeconds(HSTREAM channel) {
	return BASS_ChannelBytes2Seconds(channel, BASS_ChannelGetLength(channel, BASS_POS_BYTE));
}

void updateTracking(HSTREAM channel) {
	currentPosition = BASS_ChannelGetPosition(channel, BASS_POS_BYTE);
	updateTime(currentPosition, getChannelLengthInSeconds(channel), songTime);
	timer.restart();
}

void playback(int c, HSTREAM channel) {
	updateTracking(channel);

	if (c == 0) {
		isPlaying = TRUE;
		text.setString("Playing");
		BASS_ChannelPlay(s, FALSE);
	}
	else {
		isPlaying = FALSE; //substituir por C? 0 = true, 1 = false
		text.setString("Paused");
		BASS_ChannelPause(s);
	}
}

void resizePlaybackBar() {
	resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
	k = BASS_ChannelGetLength(s, BASS_POS_BYTE) * resizeBar / 250;
	BASS_ChannelSetPosition(s, k, BASS_POS_BYTE);
	progressBar.setSize(sf::Vector2f(resizeBar, 20));
}

HSTREAM str1, str2, strout;

/*DWORD CALLBACK MyStreamProc(HSTREAM handle, void* buf, DWORD len, void* user)
{
	DWORD r;
	if (BASS_ChannelIsActive(str1)) { // stream1 has data
		r = BASS_ChannelGetData(s, buf, len);
	}
	else if (BASS_ChannelIsActive(str2)) { // stream2 has data
		r = BASS_ChannelGetData(str2, buf, len);
		if (!BASS_ChannelIsActive(str2))
			r |= BASS_STREAMPROC_END; // stream2 has ended, so we're done
	}
	else r = BASS_STREAMPROC_END;
	return r;
}*/

int main()
{
	//oi();
	BASS_PluginLoad("bassflac.dll", 0);

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

	//progress bar
	trackBar.setFillColor(sf::Color(209, 209, 209));
	trackBar.setPosition(325.f, 130.f);

	progressBar.setFillColor(sf::Color(245, 66, 147));
	progressBar.setPosition(325.f, 130.f);

	//volume bar
	volumeBar.setFillColor(sf::Color(209, 209, 209));
	volumeBar.setPosition(325.f, 175.f);

	volumeSlider.setFillColor(sf::Color(64, 204, 61));
	volumeSlider.setPosition(325.f, 175.f);

	//ref text
	sf::Font OpenSansBold;
	OpenSansBold.loadFromFile("OpenSans-Bold.ttf");

	text.setFont(OpenSansBold);
	text.setString("Paused");
	text.setCharacterSize(24);
	text.setFillColor(sf::Color::White);
	text.setStyle(sf::Text::Bold);

	//song title
	songTitle.setFont(OpenSansBold);
	songTitle.setCharacterSize(14);
	songTitle.setFillColor(sf::Color::White);
	songTitle.setStyle(sf::Text::Bold);
	songTitle.setPosition(0, 100.f);

	//song time
	songTime.setFont(OpenSansBold);
	songTime.setCharacterSize(14);
	songTime.setFillColor(sf::Color::White);
	songTime.setStyle(sf::Text::Bold);
	songTime.setPosition(325.f, 150.f);

	//initializing player
	BASS_Init(-1, 44100, 0, 0, NULL);
	s = BASS_StreamCreateFile(FALSE, filePath, 0, 0, BASS_STREAM_AUTOFREE); //change the string for whatever filename
	TagLib::FileRef f(filePath);
	setInfo(f);

	sf::Texture testTexture;
	getCover(f, testTexture);

	sf::RectangleShape coverArt(sf::Vector2f(300, 300));
	coverArt.setTexture(&testTexture);
	coverArt.setPosition(10.f, 130.f);

	int songLenInSeconds = getChannelLengthInSeconds(s);

	songTime.setString("0:00/" + toHourFormat(songLenInSeconds));

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
						playback(0, s);
					}

					if (pauseButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) && isPlaying == TRUE) {
						playback(1, s);
					}

					if (trackBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) ) {
						resizePlaybackBar();

						updateTracking(s);
					}

					//same thing, but for volume (change that later with slider functions)
					if (volumeBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
						updateVolume(s);
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

		window.draw(trackBar);

		window.draw(progressBar);

		window.draw(volumeBar);

		window.draw(volumeSlider);

		window.draw(songTime);

		window.display();

		float x;
		if (timer.getElapsedTime() >= sf::seconds(1.0f)) {
			//though I only update the bar each second (to not overload the processor with too many calls) I always use bytes for precision
			currentPosition = BASS_ChannelGetPosition(s, BASS_POS_BYTE);
			//UPDATE BAR
			x = 250 * currentPosition / BASS_ChannelGetLength(s, BASS_POS_BYTE);
			progressBar.setSize(sf::Vector2f(x, 20));

			//UPDATE TIME (0:00)
			updateTime(currentPosition, songLenInSeconds, songTime);

			timer.restart();
		}

		/*if (trackBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) { //mudar essas var
				holding = TRUE;
				BASS_ChannelPause(s);

				resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
				k = BASS_ChannelGetLength(s, BASS_POS_BYTE) * resizeBar / 250;
				BASS_ChannelSetPosition(s, k, BASS_POS_BYTE);
				progressBar.setSize(sf::Vector2f(resizeBar, 20));

				currentPosition = BASS_ChannelGetPosition(s, BASS_POS_BYTE);
				updateTime(currentPosition, getChannelLengthInSeconds(s), songTime);
				timer.restart();

				std::cout << "POSIÇÃO DO CANAL:";
				std::cout << BASS_ChannelGetPosition(s, BASS_POS_BYTE);
				std::cout << " ";

				std::cout << "VALOR DA BARRA:";
				std::cout <<  k;
				std::cout << "\n";

				//resizePlaybackBar();

				//updateTracking(s);
			}
			else {
				holding = FALSE;
				if (isPlaying && holding == FALSE) {
					BASS_ChannelPlay(s, FALSE);
				}
			}
		}*/

		if (trackBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
			resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
			k = BASS_ChannelGetLength(s, BASS_POS_BYTE) * resizeBar / 250;
			BASS_ChannelSetPosition(s, k, BASS_POS_BYTE);
			progressBar.setSize(sf::Vector2f(resizeBar, 20));
			currentPosition = BASS_ChannelGetPosition(s, BASS_POS_BYTE);
			updateTime(currentPosition, getChannelLengthInSeconds(s), songTime);
			timer.restart();
		}

		if (volumeBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				updateVolume(s);
			}
		}
	}

	return 0;
}

//TODO - SUBSTITUIR BASS_ChannelGetLength(s, BASS_POS_BYTE)

/*NOTA => POSSO USAR O ACTIVE CHANNEL (testei parada n deu certo mas acho que rola)*/

//should updateTracking and resizeTrackBar be together?