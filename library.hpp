#pragma once

#include "song.hpp"
#include "sqlite/sqlite3.h"
#include <vector>

class Library {
private:
	sqlite3 *m_db;
	std::vector<Song> m_songs;

	void SimpleQuery(const char *const query);
	unsigned QueryCount() const;

public:
	Library();
	~Library();
	Library(Library &&l);
	Library(const Library &l) = delete;

	void Scan();

	inline unsigned Count() const { return m_songs.size(); }

	inline Song &At(unsigned idx) { return m_songs[idx]; }

	Song WithId(unsigned id);

	void LoadFullList();
	void LoadSearch(const std::string &search);
};
