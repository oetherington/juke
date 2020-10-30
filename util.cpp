#include "util.hpp"

static const char *const exts[] = {
	".m4a",
	".mp3",
	".ogg",
	".opus",
	".wav",
	".wma",
};

bool IsAudioPath(const fs::path &p)
{
	const fs::path ext = p.extension();

	for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); i++)
		if (ext == exts[i])
			return true;

	return false;
}

std::vector<std::string> Split(const std::string &str, const std::string &delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0;
	size_t pos = 0;

	do {
		pos = str.find(delim, prev);
		if (pos == std::string::npos)
			pos = str.length();
		std::string token = str.substr(prev, pos - prev);
		if (!token.empty())
			tokens.push_back(token);
		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());

	return tokens;
}

std::string RemoveExtension(const std::string &s)
{
	return fs::path(s).stem().string();
}
