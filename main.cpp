#include <iostream>
#include <filesystem>
#include <clocale>
namespace fs = std::filesystem;

#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include "bass.h"

//HSTREAM activeChannel, primary, secondary, out;

HSTREAM activeChannel, str1, str2, strout;

BASS_CHANNELINFO info, info2;

DWORD r;

bool isPlaying = FALSE, dragging[2] = { FALSE, FALSE }, inSearchBar = FALSE;

int songLengthInSeconds, sizeOfSongs, b;

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

void setActiveChannel(HSTREAM channel, const char* filename) {
	activeChannel = channel;
	BASS_ChannelGetInfo(channel, &info);
	/*TagLib::FileRef file(filename);
	setInfo(file);
	getCover(file, testTexture);
	coverArt.setTexture(&testTexture);*/
}

DWORD CALLBACK StreamProc(HSTREAM handle, void* buf, DWORD len, void* user) //shoutouts to Ian @ un4seen for this awesome post back in 2003 https://www.un4seen.com/forum/?topic=2050.msg13435#msg13435 - I adapated the functin to be loopable
{
	//AQUI HAVIA O ATUALIZADOR DE TEMPO/BARRA

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
		BASS_StreamFree(str1);
		BASS_StreamFree(str2);
	}
	return r;
}

void initializeBass(int mutipleSongs) {
	b = 0;
	str1 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
	BASS_ChannelGetInfo(str1, &info);
	setActiveChannel(str1, songs[b].c_str());
	if (mutipleSongs) {
		b = b + 1;
		str2 = BASS_StreamCreateFile(FALSE, songs[b].c_str(), 0, 0, BASS_STREAM_DECODE);
	}
	strout = BASS_StreamCreate(info.freq, info.chans, 0, StreamProc, 0); // create the output stream
	BASS_ChannelPlay(strout, FALSE);
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

		//INITIALIZE BASS

		if (songs.size() > 1) {
			initializeBass(1);
		}
		else {
			initializeBass(0);
		}

		return 1;
	}
	else {
		std::cout << "é porra nenhuma!";
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
							leaveSearchBar();
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
					parsePath(searchInput, pa, fs::status(pa));
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

	return 1;
}

/*
TODO - ERROR PARSING => IF NOT ALL THE FILES FROM THE FOLDER ARE FROM THE SAME FORMAT ===> DONE!
TODO - ERROR PARSING => BASS CALLS AND MORE ERROR CHECKS IN GENERAL
TODO - BASS => GET SYSTEM FREQUENCY INSTEAD OF SETTING IT TO 44100 BY DEFAULT
TODO - FUCNTIONS => REWRITE THE STREAM PROCESSING FUNCTION (SteamProc) -> MORE FUNCTIONS!!!
TODO - CLASSES => CREATE A CLASS FOR THE PLAYER (PLAYBACK, PAUSE, SKIP, ETC.) FUNCTIONS
*/