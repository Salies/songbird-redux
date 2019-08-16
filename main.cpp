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
HSTREAM activeChannel;

HSTREAM str1, str2, strout;

BASS_CHANNELINFO info;

bool isPlaying = FALSE;
bool holding = FALSE;
const char* filePath = "01 - Rising.mp3";
sf::Clock timer;

float resizeBar;
float resizeVolumeBar;
float volume = 1;
QWORD k;

int songLenInSeconds;

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

sf::Texture testTexture;
sf::RectangleShape coverArt(sf::Vector2f(300, 300));

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
		//BASS_ChannelPlay(s, FALSE);
		BASS_ChannelPlay(activeChannel, FALSE);
	}
	else {
		isPlaying = FALSE; //substituir por C? 0 = true, 1 = false
		text.setString("Paused");
		//BASS_ChannelPause(s);
		BASS_ChannelPause(activeChannel);
	}
}

void resizePlaybackBar() {
	resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
	k = BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE) * resizeBar / 250;
	BASS_ChannelSetPosition(activeChannel, k, BASS_POS_BYTE);
	progressBar.setSize(sf::Vector2f(resizeBar, 20));
}

void setInfo(TagLib::FileRef file) {
	artist = file.tag()->artist().toWString();
	title = file.tag()->title().toWString();
	album = file.tag()->album().toWString();
	displayName = artist + " - " + title + " [from " + album + "]";
	window.setTitle(artist + " - " + title);
	songTitle.setString(displayName);
	songLenInSeconds = getChannelLengthInSeconds(activeChannel);
	songTime.setString("0:00/" + toHourFormat(songLenInSeconds));
}

void setActiveChannel(HSTREAM channel, const char *filename) {
	activeChannel = channel;
	BASS_ChannelGetInfo(channel, &info);
	TagLib::FileRef file(filename);
	setInfo(file);
	getCover(file, testTexture);
	coverArt.setTexture(&testTexture);
}

DWORD CALLBACK MyStreamProc(HSTREAM handle, void* buf, DWORD len, void* user)
{
	DWORD r;
	if (BASS_ChannelIsActive(str1)) { // stream1 has data
		if (activeChannel != str1) {
			setActiveChannel(str1, "01. Deus Le Volt!.flac");
		}
		r = BASS_ChannelGetData(str1, buf, len);
		//std::cout << "oi";
	}
	else if (BASS_ChannelIsActive(str2)) { // stream2 has data
		if (activeChannel != str2) {
			setActiveChannel(str2, "02. Spread Your Fire.flac");
		}
		r = BASS_ChannelGetData(str2, buf, len);
		//setActiveChannel(str1, "02. Spread Your Fire.flac");
		if (!BASS_ChannelIsActive(str2))
			r |= BASS_STREAMPROC_END; // stream2 has ended, so we're done
	}
	else r = BASS_STREAMPROC_END;
	return r;
}

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

	getCover(f, testTexture);

	coverArt.setPosition(10.f, 130.f);

	//TEST STUFF
	str1 = BASS_StreamCreateFile(FALSE, "01. Deus Le Volt!.flac", 0, 0, BASS_STREAM_DECODE);
	str2 = BASS_StreamCreateFile(FALSE, "02. Spread Your Fire.flac", 0, 0, BASS_STREAM_DECODE);
	BASS_ChannelGetInfo(str1, &info);
	setActiveChannel(str1, "01. Deus Le Volt!.flac");
	strout = BASS_StreamCreate(info.freq, info.chans, 0, MyStreamProc, 0); // create the output stream

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
						//playback(0, s);
						//playback(0, strout);
						isPlaying = TRUE;
						BASS_ChannelPlay(strout, FALSE);
						text.setString("Playing");
					}

					if (pauseButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) && isPlaying == TRUE) {
						//playback(1, s);
						//playback(1, strout);
						isPlaying = FALSE;
						BASS_ChannelPause(strout);
						text.setString("Paused");
					}

					if (trackBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) ) {
						//resizePlaybackBar();

						//updateTracking(s);

						resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
						k = BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE) * resizeBar / 250;
						BASS_ChannelSetPosition(activeChannel, k, BASS_POS_BYTE);
						BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //resets the buffer - it lags anyway, but this is not buggy/it's more elegant/manageable
						progressBar.setSize(sf::Vector2f(resizeBar, 20));

						updateTracking(activeChannel);
					}

					//same thing, but for volume (change that later with slider functions)
					if (volumeBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
						//updateVolume(s);
						//updateVolume(strout);
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
			//currentPosition = BASS_ChannelGetPosition(s, BASS_POS_BYTE);
			currentPosition = BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE);
			//UPDATE BAR
			//x = 250 * currentPosition / BASS_ChannelGetLength(s, BASS_POS_BYTE);
			x = 250 * currentPosition / BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE);
			progressBar.setSize(sf::Vector2f(x, 20));

			//UPDATE TIME (0:00)
			updateTime(currentPosition, songLenInSeconds, songTime);

			timer.restart();
		}

		/*if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
			std::cout << holding;
			if (trackBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
				holding = TRUE;
			}

			if (holding == TRUE) {
				resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
				std::cout << resizeBar;
				std::cout << "\n";
				if (resizeBar <= 0) {
					resizeBar = 0;
				}
				else if (resizeBar >= 250) {
					resizeBar = 250;
				}

				k = BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE) * resizeBar / 250;
				BASS_ChannelSetPosition(activeChannel, k, BASS_POS_BYTE);
				progressBar.setSize(sf::Vector2f(resizeBar, 20));
				currentPosition = BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE);
				updateTime(currentPosition, getChannelLengthInSeconds(activeChannel), songTime);
				BASS_ChannelPause(strout);
				updateTracking(activeChannel);
				timer.restart();
			}
		}
		else {
			if (holding && isPlaying && BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE) != BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE)) { //TODO
				holding = FALSE;
				//BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //resets the buffer - it lags anyway, but this is not buggy/it's more elegant/manageable // change this one's position?
				BASS_ChannelPlay(strout, FALSE);
			} //BUGADA DE NOVOOOOOOOOOO
		}*/

		if (volumeBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				//updateVolume(s);
				updateVolume(strout);
			}
		}
	}

	return 0;
}

//TODO - SUBSTITUIR BASS_ChannelGetLength(s, BASS_POS_BYTE)

/*NOTA => POSSO USAR O ACTIVE CHANNEL (testei parada n deu certo mas acho que rola)*/

//should updateTracking and resizeTrackBar be together?

//TODO - ISPLAYING = FALSE QUANDO TERMINA, DENTRE OUTRAS SITUAÇÕES

/*TRABALHAR COM PROPORCIONALDIADE NO SETACTIVECHANNEL (IF QUEUE > 1?)*/

//STOURT SIMPLESMENTE RECEBE DADOS DOS OUTROS - TENHO QUE ATUALIZAR A FUNÇÃO