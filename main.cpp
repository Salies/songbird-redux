/*
Songbird, Copyright (C) 2019  Daniel "Salies" Serezane serezane(at)protonmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <filesystem>
namespace fs = std::filesystem;

#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include "bass.h"

#include "covers.h"

#include <taglib/fileref.h> //are all of these necesary?
#include <taglib/tag.h>
#include <taglib/taglib.h>

HSTREAM activeChannel, str1, str2, strout;

BASS_CHANNELINFO info, info2;

DWORD r;

bool isPlaying = FALSE, dragging[2] = { FALSE, FALSE }, inSearchBar = FALSE;

int b;

float resize[2];

size_t searchSize;

sf::RenderWindow window(sf::VideoMode(654, 244), "Songbird", sf::Style::None);

sf::Event event;

sf::Vector2i mousePos;

sf::RectangleShape cover(sf::Vector2f(200, 200)), rects[5], border[2], controls[3];

sf::Text searchText, songTime, trackInfo[4];

sf::Font fonts[2];

sf::Texture coverArt, controlTileset;

std::vector <std::string> songs;

sf::String supportedFiles[7] = { "wav", "aiff", "mp3", "mp2", "mp1", "ogg", "flac"}, searchInput;

struct rV{ //WORKS!
	float sizeX;
	float sizeY;
	float posX;
	float posY;
	sf::Uint8 red;
	sf::Uint8 green;
	sf::Uint8 blue;
};

struct {
	QWORD currentPos; //current song position
	QWORD bts; //song length in bytes
	int secs; //song length in seconds
} lens;

rV rectValues[5] = {
	{200, 15, 233, 177, 50, 50, 50}, //search bar --- 1 + 20 + 1 + 200 + 1 + 10 (horizontal) / 178 vertical
	{200, 15, 309, 207, 50, 50, 50}, //trackbar (background) --- 1 + 20 + 1 + 200 + 1 + 10 + 11 + 15 + 14 + 15 + 11 + 10 / 208 - 1 cause yes (vertical)
	{0, 15, 309, 207, 185, 185, 185}, //trackbar (itself)
	{44, 15, 589, 207, 50, 50, 50}, //volume bar (background)
	{44, 15, 589, 207, 185, 185, 185} //volume bar (itself)
};

rV borderValues[2] = {
	{652, 242, 1, 1, 30, 30, 30},
	{200, 200, 22, 22, 26, 26, 26}
};

rV textValues[4] = { //font (0 = REGULAR, 1 = BOLD), fontsize, posX, posY, color, color, color - TODO - REDO THIS (not necessary, too many repeated values)
	{1, 15, 232, 22, 240, 240, 240},
	{0, 12, 232, 41, 215, 215, 215},
	{0, 12, 232, 74, 215, 215, 215},
	{0, 12, 232, 89, 215, 215, 215}
};

std::string toHourFormat(int seconds) {
	int minutes = seconds / 60; //int works! (rounds to less) / equivalent of Math.floor() method --- TODO(?) predeclare these variables
	int remainingSeconds = seconds - minutes * 60;
	std::string fmtSeconds = fmt::format("{:02d}", remainingSeconds);
	std::string formatedTime = std::to_string(minutes) + ":" + fmtSeconds;
	return formatedTime;
}

void updateTime(int newPos) {
	songTime.setString(toHourFormat(newPos) + " / " + toHourFormat(lens.secs));
}

void updateTrackBar(float newSize) {
	rects[2].setSize(sf::Vector2f(newSize, 15));
}

void updateTracking() {
	lens.currentPos = BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE);
	updateTime(BASS_ChannelBytes2Seconds(activeChannel, lens.currentPos));
	updateTrackBar(200 * lens.currentPos / BASS_ChannelGetLength(activeChannel, BASS_POS_BYTE));
}

void updateVolume() {
	resize[1] = sf::Mouse::getPosition(window).x - 589;
	if (resize[1] <= 0) {
		resize[1] = 0;
	}
	else if (resize[1] >= 44) {
		resize[1] = 44;
	}
	BASS_ChannelSetAttribute(strout, BASS_ATTRIB_VOL, resize[1] / 44);
	rects[4].setSize(sf::Vector2f(resize[1], 15));
}

void resizeTrackBar() {
	resize[0] = sf::Mouse::getPosition(window).x - 309;
	if (resize[0] <= 0) {
		resize[0] = 0;
	}
	else if (resize[0] >= 200) {
		resize[0] = 200;
	}
	updateTime(BASS_ChannelBytes2Seconds(activeChannel, lens.bts * resize[0] / 200)); //TODO - var?
	updateTrackBar(resize[0]);
}

void leaveSearchBar() {
	inSearchBar = FALSE;
	rects[0].setFillColor(sf::Color(50, 50, 50));
	searchText.setFillColor(sf::Color::White);
}

int checkSupported(fs::path extension) {
	for (unsigned int i = 0; i < 7; ++i) {
		if ("." + supportedFiles[i] == extension.string()) {
			return 1;
		}
	}
	return 0;
}

void playback(int c) {
	isPlaying = c;
	if (isPlaying) {
		BASS_ChannelPlay(strout, FALSE);
		controls[0].setTextureRect(sf::IntRect(19, 0, 14, 13));
	}
	else {
		BASS_ChannelPause(strout);
		controls[0].setTextureRect(sf::IntRect(0, 0, 14, 13));
	}
}

void clearStream() {
	BASS_StreamFree(strout);
	BASS_StreamFree(str1);
	BASS_StreamFree(str2);
	playback(0);
}

void setInfo(TagLib::FileRef file) {
	trackInfo[0].setString(file.tag()->title().toWString()); //TODO - IF NAME TOO BIG // I GUESS THIS WORKS ON GNU/LINUX BECAUSE IT'S DEFINED BY TAGLIB -- NEEDS CONFIRMATION
	trackInfo[1].setString(file.tag()->artist().toWString());
	trackInfo[2].setString(file.tag()->album().toWString());
	if (file.tag()->genre() != TagLib::String::null) {
		trackInfo[3].setString(std::to_string(file.tag()->year()) + " / " + file.tag()->genre().toCString());
	}
	else {
		trackInfo[3].setString(std::to_string(file.tag()->year()));
	}
}

void setActiveChannel(HSTREAM channel, const char* filename) {
	activeChannel = channel;
	BASS_ChannelGetInfo(channel, &info);

	lens.bts = BASS_ChannelGetLength(channel, BASS_POS_BYTE);
	lens.secs = BASS_ChannelBytes2Seconds(channel, lens.bts);

	updateTracking();

	TagLib::FileRef file(filename);
	setInfo(file);
	getCover(file, coverArt);
	cover.setTexture(&coverArt, true);
}

DWORD CALLBACK StreamProc(HSTREAM handle, void* buf, DWORD len, void* user) //shoutouts to Ian @ un4seen for this awesome post back in 2003 https://www.un4seen.com/forum/?topic=2050.msg13435#msg13435 - I adapated the functin to be loopable
{
	updateTracking();

	if (BASS_ChannelIsActive(activeChannel)) {
		r = BASS_ChannelGetData(activeChannel, buf, len);
	}
	else if (!BASS_ChannelIsActive(activeChannel) && BASS_ChannelIsActive(str2)) {
		BASS_ChannelGetInfo(str2, &info2);
		if (songs[b + 1].c_str() && !BASS_ChannelIsActive(str1)) {
			b = b + 1;
			str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
		}
		else if (!songs[b + 1].c_str()) {
			b = NULL;
		}
		setActiveChannel(str2, info2.filename);
		r = BASS_ChannelGetData(str2, buf, len);
	}
	else if (!BASS_ChannelIsActive(activeChannel) && BASS_ChannelIsActive(str1)) {
		BASS_ChannelGetInfo(str1, &info2);
		if (songs[b + 1].c_str() && !BASS_ChannelIsActive(str2)) {
			b = b + 1;
			str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
		}
		else if (!songs[b + 1].c_str()) {
			b = NULL;
		}
		setActiveChannel(str1, info2.filename);
		r = BASS_ChannelGetData(str1, buf, len);
	}
	else {
		r = BASS_STREAMPROC_END;
		clearStream();
	}
	return r;
}

void initializeBass(int mutipleSongs) {
	clearStream();
	b = 0;
	str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
	BASS_ChannelGetInfo(str1, &info);
	setActiveChannel(str1, songs[b].c_str());
	if (mutipleSongs) {
		b = b + 1;
		str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
	}
	strout = BASS_StreamCreate(info.freq, info.chans, 0, StreamProc, 0); // create the output stream
	playback(1);
}

void jumpTrack() {
	if (b) {
		if (activeChannel == str1) {
			BASS_ChannelGetInfo(str2, &info2);
			setActiveChannel(str2, info2.filename);
			BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //maybe setting this later "bites" the beggining of the song? idk, it sounds smooth. maybe this will have to be a down.
			if (b != songs.size() - 1) {
				b = b + 1;
				str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
			}
			else {
				b = NULL;
			}
		}
		else if (activeChannel == str2) {
			BASS_ChannelGetInfo(str1, &info2);
			setActiveChannel(str1, info2.filename);
			BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
			if (b != songs.size() - 1) {
				b = b + 1;
				str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
			}
			else {
				b = NULL;
			}
		}
	}
	else {
		clearStream();
	}
}

void backTrack() {
	if (BASS_ChannelBytes2Seconds(activeChannel, BASS_ChannelGetPosition(activeChannel, BASS_POS_BYTE)) > 20 || b == 1 || songs.size() == 1) {
		BASS_ChannelSetPosition(activeChannel, 0, BASS_POS_BYTE);
		BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
	}
	else {
		if (b) { //unecessary stretch? TODO - REDO LOGIC WITH B
			if (activeChannel == str1) {
				b = b - 2;
				str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
				BASS_ChannelGetInfo(str2, &info2);
				setActiveChannel(str2, info2.filename);
				BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
				//sempre vai acontecer,acho
				b = b + 1;
				str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
			}
			else if (activeChannel == str2) {
				b = b - 2;
				str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
				BASS_ChannelGetInfo(str1, &info2);
				setActiveChannel(str1, info2.filename);
				BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
				//sempre vai acontecer,acho
				b = b + 1;
				str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
			}
		}
		else {
			if (songs.size() > 1) {
				b = songs.size() - 2;
				str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
				BASS_ChannelGetInfo(str1, &info2);
				setActiveChannel(str1, info2.filename);
				BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE);
				//sempre vai acontecer,acho
				b = b + 1;
				str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
			}
		}
	}
}

int parsePath(sf::String dir, fs::path path, fs::file_status pathStatus) {
	if (fs::is_regular_file(pathStatus)) { //is a single file
		if (checkSupported(path.extension())) {
			songs.clear();
			songs.push_back(dir);
			initializeBass(0);
			return 1;
		}
		return 0;
	}
	else if (fs::is_directory(pathStatus)) {
		if (!songs.empty()) { //reset array
			songs.clear();
		};

		for (const auto& entry : fs::directory_iterator(path)) {
			if (checkSupported(entry.path().extension())) { //MAYBE TODO - INSTEAD OF PUSHING TO SONGS, PUSH TO TEMP ARRAY AND THEN IF TEMP IS NOT EMPTY PUSH TO SONGS (THIS WAY SONGS IS NOT CLEARED UNNECESSARILY)
				songs.push_back(entry.path().string());
			}
		}

		if (songs.empty()) {
			return 0;
		}

		if (songs.size() > 1) {
			initializeBass(1);
		}
		else {
			initializeBass(0);
		}

		return 1;
	}
	else {
		return 0;
	}
}

int main() {
	/* ===== BASS INIT ===== */
	BASS_PluginLoad("bassflac.dll", 0);

	BASS_Init(-1, 44100, 0, 0, NULL);

	/* ===== FRONT END - LOADING STUFF ===== */
	controlTileset.loadFromFile("assets/controls.png");
	fonts[0].loadFromFile("assets/fonts/OpenSans-Regular.ttf");
	fonts[1].loadFromFile("assets/fonts/OpenSans-Bold.ttf");

	/* ===== FRONT END - SETTING UP RECTS =====*/
	cover.setPosition(22, 22);

	for (unsigned int i = 0u; i < 3; ++i) {
		controls[i].setTexture(&controlTileset, 1);
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

	for (unsigned int i = 0u; i < 2; ++i) {
		border[i].setSize(sf::Vector2f(borderValues[i].sizeX, borderValues[i].sizeY));
		border[i].setPosition(borderValues[i].posX, borderValues[i].posY);
		border[i].setFillColor(sf::Color::Transparent);
		border[i].setOutlineColor(sf::Color(borderValues[i].red, borderValues[i].green, borderValues[i].blue));
		border[i].setOutlineThickness(1);
	}

	for (unsigned int i = 0u; i < 4; ++i) {
		trackInfo[i].setFont(fonts[(int)textValues[i].sizeX]);
		trackInfo[i].setCharacterSize(textValues[i].sizeY);
		trackInfo[i].setPosition(232, textValues[i].posY);
		trackInfo[i].setFillColor(sf::Color(textValues[i].red, textValues[i].green, textValues[i].blue));
	}

	/* ===== FRONT END - SETTING UP TEXT =====*/
	searchText.setFont(fonts[0]);
	searchText.setFillColor(sf::Color::White);
	searchText.setCharacterSize(11);
	searchText.setPosition(235, 178);

	songTime.setString("0:00 / 0:00");
	songTime.setFont(fonts[0]);
	songTime.setFillColor(sf::Color(50, 50, 50));
	songTime.setCharacterSize(12);
	songTime.setPosition(514, 207);

	while (window.isOpen())
	{
		/* ===== EVENTS ===== */
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left) {
					mousePos = sf::Mouse::getPosition(window);
					if(controls[0].getGlobalBounds().contains(mousePos.x, mousePos.y)) { //is there a better way to do this other than this ridiculous if? if so, please push a commit!
						if (isPlaying) {
							playback(0);
						}
						else {
							playback(1);
						}
					}
					else if (controls[1].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						backTrack();
					}
					else if (controls[2].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						jumpTrack();
					}
					else if (rects[1].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						dragging[0] = TRUE;

						if (BASS_ChannelIsActive(strout) == BASS_ACTIVE_PLAYING) {
							BASS_ChannelPause(strout);
						}

						resizeTrackBar();
					}
					else if (rects[3].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						dragging[1] = TRUE;
						updateVolume();
					}
					
					if (rects[0].getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						inSearchBar = TRUE;
						rects[0].setFillColor(sf::Color(185, 185, 185));
						searchText.setFillColor(sf::Color::Black);
					}
					else {
						if (inSearchBar) {
							leaveSearchBar();
						}
					}
				}
				break;

			case sf::Event::MouseMoved:
				if (dragging[0]) {
					resizeTrackBar();
				}
				else if (dragging[1]) {
					updateVolume();
				}
				break;

			case  sf::Event::MouseButtonReleased:
				if (event.mouseButton.button == sf::Mouse::Left && dragging[0] == TRUE) {
					dragging[0] = FALSE;
					if (resize[0] == 200) {
						BASS_ChannelSetPosition(activeChannel, lens.bts - 1, BASS_POS_BYTE); //this WASN'T actually necessary. but at compile time it was needed. idk why. no idea at all.
					}
					else {
						BASS_ChannelSetPosition(activeChannel, lens.bts* resize[0] / 200, BASS_POS_BYTE);
					}
					if (isPlaying) {
						BASS_ChannelPlay(strout, FALSE);
					}
					BASS_ChannelSetPosition(strout, 0, BASS_POS_BYTE); //resets the buffer - it lags anyway, but this is not buggy/it's more elegant/manageable
				}
				else if (event.mouseButton.button == sf::Mouse::Left && dragging[1] == TRUE) {
					dragging[1] = FALSE;
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
						if (event.text.unicode != 22) { //prevent weird chat in ctrl + v
							searchInput += static_cast<char>(event.text.unicode);
							if (searchSize <= 27) {
								searchText.setString(searchInput);
							}
							else {
								searchText.setString(searchInput.substring(0, 25) + "...");
							}
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
				else if (event.key.code == sf::Keyboard::Enter && inSearchBar) {
					fs::path pa(searchInput);
					parsePath(searchInput, pa, fs::status(pa)); //TODO - ADD PARSER/ERROR DISPLAYER
					searchInput = "";
					searchText.setString(searchInput);
					leaveSearchBar();
				}
				break;

			default:
				break;
			}
		};

		window.clear(sf::Color::Black);

		window.draw(cover);

		window.draw(songTime);

		for (unsigned int i = 0u; i < 3; ++i) {
			window.draw(controls[i]);
		}

		for (unsigned int i = 0u; i < 2; ++i) {
			window.draw(border[i]);
		}

		for (unsigned int i = 0u; i < 5; ++i) {
			window.draw(rects[i]);
		}

		for (unsigned int i = 0u; i < sizeof(textValues) / sizeof(*textValues); ++i) {
			window.draw(trackInfo[i]);
		}

		window.draw(searchText);

		window.display();
	}

	return 1;
}

/*
TODO - REBUILD HEADER FILES FOR GCC/LINUX COMPATIBILITY

TODO - ERROR PARSING => IF NOT ALL THE FILES FROM THE FOLDER ARE FROM THE SAME FORMAT ===> DONE!
TODO - ERROR PARSING => BASS CALLS AND MORE ERROR CHECKS IN GENERAL
TODO - BASS => GET SYSTEM FREQUENCY INSTEAD OF SETTING IT TO 44100 BY DEFAULT
TODO - FUCNTIONS => REWRITE THE STREAM PROCESSING FUNCTION (SteamProc) -> MORE FUNCTIONS!!!
TODO - CLASSES => CREATE A CLASS FOR THE PLAYER (PLAYBACK, JUMPTRACK, BACKTRACK) FUNCTIONS
TODO - ADD FADE IN/OUT (SKIP/FORWARD(?)/CLICK BAR)
TODO - CLEAR INFO WHEN STREAM CLEAR
TODO(?) - CLOSE/MINIMIZE (NOT YET POSSIBLE) BUTTONS
TODO - PROCESS IMAGE (SO IT'S NOT PIXELATED)
TODO - INITIAL BLACK COVER ART
*/