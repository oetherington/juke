#include "song.hpp"
#include "player.hpp"
#include "util.hpp"
#include <cctype>

Song::Song(std::string path)
	: path(path)
{
	if (!path.size())
		throw "Empty path";

	Player player(path);

	title = player.GetMetaString(Meta::Title);
	if (!title.size())
		title = path;
	title = fs::path(title).stem().string();

	artist = player.GetMetaString(Meta::Artist);
	album = player.GetMetaString(Meta::Album);

	if (!artist.size() || !album.size()) {
		const std::vector<std::string> parts = Split(path, "/");
		if (parts.size() == 4) {
			if (!artist.size())
				artist = parts[1];
			if (!album.size())
				album = parts[2];
		}
	}

	if (!artist.size())
		artist = "<Unknown artist>";

	if (!album.size())
		album = "<Unknown album>";

	track = player.GetTrackNumber();
	if (track == 0 && title.size() >= 4) {
		if (isdigit(title[0]) && isdigit(title[1]) && title[2] == ' ') {
			track = (title[0] - '0') * 10 + (title[1] - '0');
			title = title.substr(3);
		} else if (isdigit(title[0]) && title[1] == ' ') {
			track = title[0] - '0';
			title = title.substr(2);
		}
	}

	length = player.GetLength();
}
