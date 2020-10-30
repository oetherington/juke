#pragma once

#include <string>

struct Song {
	std::string path;
	std::string title;
	std::string artist;
	std::string album;
	unsigned track;
	unsigned length;

	Song() {}
	Song(std::string path);
};
