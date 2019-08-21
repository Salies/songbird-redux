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
BASS_CHANNELINFO info2;

bool isPlaying = FALSE;
bool draggingTrackbar = FALSE;
bool draggingVolume = FALSE;

//const char* filePath = "01 - Rising.mp3";
//sf::Clock timer;

float resizeBar;
float resizeVolumeBar;
float volume = 1;
QWORD k;

float x;

int songLenInSeconds;

QWORD currentPosition;
std::string currentSongTime;

sf::RenderWindow window(sf::VideoMode(1355, 715), "Songbird");

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

void updateTime(QWORD current, int songLengthS, sf::Text& text, HSTREAM channel) {
	std::string ctime = toHourFormat(BASS_ChannelBytes2Seconds(channel, current));
	text.setString(ctime + "/" + toHourFormat(songLengthS)); //TODO CHANGE THIS - since hourformat(songlengths) is constant
}

void updateVolume(HSTREAM channel) {
	resizeVolumeBar = sf::Mouse::getPosition(window).x - volumeBar.getPosition().x;
	if (resizeVolumeBar <= 0) {
		resizeVolumeBar = 0;
	}
	else if (resizeVolumeBar >= 150) {
		resizeVolumeBar = 150;
	}
	volume = resizeVolumeBar / 150;
	BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, volume);
	volumeSlider.setSize(sf::Vector2f(resizeVolumeBar, 20));
}

int getChannelLengthInSeconds(HSTREAM channel) {
	return BASS_ChannelBytes2Seconds(channel, BASS_ChannelGetLength(channel, BASS_POS_BYTE));
}

void updateTracking(HSTREAM channel) {
	currentPosition = BASS_ChannelGetPosition(channel, BASS_POS_BYTE);
	updateTime(currentPosition, getChannelLengthInSeconds(channel), songTime, channel);
}

void playback(int c) {
	isPlaying = c;
	if (isPlaying) {
		BASS_ChannelPlay(strout, FALSE);
		text.setString("Playing");
	}
	else {
		BASS_ChannelPause(strout);
		text.setString("Paused");
	}
}

void resizePlaybackBar() {
	resizeBar = sf::Mouse::getPosition(window).x - trackBar.getPosition().x;
	if (resizeBar <= 0) {
		resizeBar = 0;
	}
	else if (resizeBar >= 250) {
		resizeBar = 250;
	}
	k = BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE) * resizeBar / 250;
	BASS_ChannelSetPosition(activeChannel, k, BASS_POS_BYTE);
	progressBar.setSize(sf::Vector2f(resizeBar, 20));
	updateTracking(activeChannel);
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

void setActiveChannel(HSTREAM channel, const char* filename) {
	activeChannel = channel;
	BASS_ChannelGetInfo(channel, &info);
	TagLib::FileRef file(filename);
	setInfo(file);
	getCover(file, testTexture);
	coverArt.setTexture(&testTexture);
}

DWORD r;

const char* songs[] = {
	"01. Deus Le Volt!.flac",
	"02. Spread Your Fire.flac",
	"03. Angels And Demons.flac",
	"04. Waiting Silence.flac",
	"05. Wishing Well.flac",
	"06. The Temple Of Hate.flac",
	"07. The Shadow Hunter.flac",
	"08. No Pain For The Dead.flac",
	"09. Winds Of Destination.flac",
	"10. Sprouts Of Time.flac",
	"11. Morning Star.flac",
	"12. Late Redemption.flac",
	"13. Gate XIII.flac"
};

/*/const char* songs[] = {
	"01. Act I_ Scene One_ Regression.flac",
	"02. Act I_ Scene Two_ I. Overture 1928.flac",
	"03. Act I_ Scene Two_ II. Strange Déjà Vu.flac",
	"04. Act I_ Scene Three_ I. Through My Words.flac",
	"05. Act I_ Scene Three_ II. Fatal Tragedy.flac",
	"06. Act I_ Scene Four_ Beyond This Life.flac",
	"07. Act I_ Scene Five_ Through Her Eyes.flac",
	"08. Act II_ Scene Six_ Home.flac",
	"09. Act II_ Scene Seven_ I. The Dance of Eternity.flac",
	"10. Act II_ Scene Seven_ II. One Last Time.flac",
	"11. Act II_ Scene Eight_ The Spirit Carries On.flac",
	"12. Act II_ Scene Nine_ Finally Free.flac"
};*/

int sizeOfSongs = sizeof(songs) / sizeof(*songs); //obivmante mudar isso quando os queues forem alteráveis

int b = 0;

DWORD CALLBACK MyStreamProc(HSTREAM handle, void* buf, DWORD len, void* user)
{
	//BOTAR UM IF AQUI? (DAR STD OUT OI PRA VER SE PRECISA)

	//though I only update the bar each second (to not overload the processor with too many calls) I always use bytes for precision
	currentPosition = BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE);
	//UPDATE BAR
	x = 250 * currentPosition / BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE);
	progressBar.setSize(sf::Vector2f(x, 20));

	updateTime(currentPosition, songLenInSeconds, songTime, activeChannel);

	if (BASS_ChannelIsActive(activeChannel)) { // stream1 has data //VOLTAR PRA STR1, EM STR2 DECLARAR 1
		r = BASS_ChannelGetData(activeChannel, buf, len);
	}
	else if (!BASS_ChannelIsActive(activeChannel) && BASS_ChannelIsActive(str2)) { // stream2 has data
		BASS_ChannelGetInfo(str2, &info2);
		if (songs[b + 1] && !BASS_ChannelIsActive(str1)) { //verify needed because a skip could've happened
			b = b + 1;
			str1 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
		}
		else if (!songs[b + 1]) {
			b = NULL;
		}
		setActiveChannel(str2, info2.filename);
		r = BASS_ChannelGetData(str2, buf, len);
	}
	else if (!BASS_ChannelIsActive(activeChannel) && BASS_ChannelIsActive(str1)) {
		BASS_ChannelGetInfo(str1, &info2);
		if (songs[b + 1] && !BASS_ChannelIsActive(str2)) {
			b = b + 1;
			str2 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
		}
		else if (!songs[b + 1]) {
			b = NULL;
		}
		setActiveChannel(str1, info2.filename);
		r = BASS_ChannelGetData(str1, buf, len);
	}
	else {
		r = BASS_STREAMPROC_END;
		BASS_StreamFree(str1);
		BASS_StreamFree(str2);
	}
	return r;
}

//HSTREAM currentChannel;
void jumpTrack() {
	if (b) {
		if (activeChannel == str1) {
			//currentChannel = str2;
			BASS_ChannelGetInfo(str2, &info2);
			//setActiveChannel(currentChannel, info2.filename);
			setActiveChannel(str2, info2.filename);
			BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //maybe setting this later "bites" the beggining of the song? idk, it sounds smooth. maybe this will have to be a down.
			if (songs[b + 1]) {
				b = b + 1;
				str1 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
			}
			else {
				b = NULL;
			}
		}
		else if (activeChannel == str2) {
			BASS_ChannelGetInfo(str1, &info2);
			setActiveChannel(str1, info2.filename);
			BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
			if (songs[b + 1]) {
				b = b + 1;
				str2 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
			}
			else {
				b = NULL;
			}
		}
	}
	else {
		BASS_StreamFree(str1);
		BASS_StreamFree(str2);
	}
}

void backTrack() {
	std::cout << b;
	if (BASS_ChannelBytes2Seconds(activeChannel, BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE)) > 20 || b == 1 || sizeOfSongs == 1) {
		BASS_ChannelSetPosition(activeChannel,0,BASS_POS_BYTE);
		BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
	}
	else {
		if (b) { //unecessary stretch? TODO - REDO LOGIC WITH B
			if (activeChannel == str1) {
				b = b - 2;
				str2 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
				BASS_ChannelGetInfo(str2, &info2);
				setActiveChannel(str2, info2.filename);
				BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
				//sempre vai acontecer,acho
				b = b + 1;
				str1 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
			}
			else if (activeChannel == str2) {
				b = b - 2;
				str1 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
				BASS_ChannelGetInfo(str1, &info2);
				setActiveChannel(str1, info2.filename);
				BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
				//sempre vai acontecer,acho
				b = b + 1;
				str2 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
			}
		}
		else {
			if (sizeOfSongs > 1) {
				b = sizeOfSongs - 2;
				str1 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
				BASS_ChannelGetInfo(str1, &info2);
				setActiveChannel(str1, info2.filename);
				BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
				//sempre vai acontecer,acho
				b = b + 1;
				str2 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
			}
		}
	}
}

void updateControls() {

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

	//skip button
	sf::RectangleShape skipButton(sf::Vector2f(50, 50));
	skipButton.setTexture(&buttonsTile);
	skipButton.setTextureRect(sf::IntRect(266, 0, 121, 112));
	skipButton.setPosition(200.f, 30.f);

	//back button
	sf::RectangleShape backButton(sf::Vector2f(50, 50));
	backButton.setTexture(&buttonsTile);
	backButton.setTextureRect(sf::IntRect(449, 0, 121, 112));
	backButton.setPosition(300.f, 30.f);

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

	coverArt.setPosition(10.f, 130.f);

	//TEST STUFF
	str1 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
	BASS_ChannelGetInfo(str1, &info);
	setActiveChannel(str1, songs[b]);
	if (sizeOfSongs != 1) {
		std::cout << "batata";
		b = b + 1;
		str2 = BASS_StreamCreateFile(FALSE, songs[b], 0, 0, BASS_STREAM_DECODE);
	}
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
						playback(1);
					}

					if (pauseButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y) && isPlaying == TRUE) {
						playback(0);
					}

					if (skipButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
						jumpTrack();
					}

					if (backButton.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
						backTrack();
					}

					if (trackBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
						//resizePlaybackBar();

						//updateTracking(s);

						draggingTrackbar = TRUE;

						if (BASS_ChannelIsActive(strout) == BASS_ACTIVE_PLAYING) {
							BASS_ChannelPause(strout);
						}

						resizePlaybackBar();

						BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //resets the buffer - it lags anyway, but this is not buggy/it's more elegant/manageable
					}


					//same thing, but for volume (change that later with slider functions)
					if (volumeBar.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
						draggingVolume = TRUE;
						updateVolume(strout);
					}
				}
				break; 

			case sf::Event::MouseMoved:
				if (draggingTrackbar) {
					resizePlaybackBar();
				}
				else if (draggingVolume) {
					updateVolume(strout);
				}
				break;

			case  sf::Event::MouseButtonReleased:
				if (event.mouseButton.button == sf::Mouse::Left && draggingTrackbar == TRUE) {
					draggingTrackbar = FALSE;
					if (isPlaying) {
						BASS_ChannelPlay(strout, FALSE);
					}
					BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //resets the buffer - it lags anyway, but this is not buggy/it's more elegant/manageable
				}
				else if (event.mouseButton.button == sf::Mouse::Left && draggingVolume == TRUE) {
					draggingVolume = FALSE;
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

		window.draw(skipButton);

		window.draw(backButton);

		window.display();
	}

	return 0;
}

//TODO - SUBSTITUIR BASS_ChannelGetLength(s, BASS_POS_BYTE)

/*NOTA => POSSO USAR O ACTIVE CHANNEL (testei parada n deu certo mas acho que rola)*/

//should updateTracking and resizeTrackBar be together?

//TODO - ISPLAYING = FALSE QUANDO TERMINA, DENTRE OUTRAS SITUAÇÕES

/*TRABALHAR COM PROPORCIONALDIADE NO SETACTIVECHANNEL (IF QUEUE > 1?)*/

//STOURT SIMPLESMENTE RECEBE DADOS DOS OUTROS - TENHO QUE ATUALIZAR A FUNÇÃO

//TODO - ATUALIZAR A BARRA NA HORA QUE DÁ SKIP/BACK