#include "library.hpp"
#include "util.hpp"
#include <cstdio>

Library::Library()
{
	if (sqlite3_open_v2(":memory:", &m_db,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
		throw "Couldn't open database";

	SimpleQuery("CREATE TABLE songs ("
				"path TEXT UNIQUE NOT NULL, "
				"title TEXT, "
				"artist TEXT, "
				"album TEXT, "
				"track INTEGER, "
				"length INTEGER);");
}

Library::~Library()
{
	sqlite3_close(m_db);
}

Library::Library(Library &&l)
{
	m_db = l.m_db;
	l.m_db = nullptr;
}

void Library::Scan()
{
	sqlite3_stmt *inserter;
	if (sqlite3_prepare_v2(m_db,
				"INSERT INTO songs (path, title, artist, album, track, length) "
				"VALUES (?, ?, ?, ?, ?, ?);", 4096, &inserter, nullptr))
		throw "Couldn't create song inserter query";

	for(const auto &p : fs::recursive_directory_iterator(".")) {
		const fs::path path = p.path();
		if (!IsAudioPath(path))
			continue;

		const Song s(path.string());

		if (sqlite3_bind_text(inserter, 1, s.path.c_str(), s.path.size(),
					SQLITE_TRANSIENT) != SQLITE_OK)
			throw "Cannot bind path query data";
		if (sqlite3_bind_text(inserter, 2, s.title.c_str(), s.title.size(),
					SQLITE_TRANSIENT) != SQLITE_OK)
			throw "Cannot bind title query data";
		if (sqlite3_bind_text(inserter, 3, s.artist.c_str(), s.artist.size(),
					SQLITE_TRANSIENT) != SQLITE_OK)
			throw "Cannot bind artist query data";
		if (sqlite3_bind_text(inserter, 4, s.album.c_str(), s.album.size(),
					SQLITE_TRANSIENT) != SQLITE_OK)
			throw "Cannot bind album query data";
		if (sqlite3_bind_int(inserter, 5, s.track) != SQLITE_OK)
			throw "Cannot bind track query data";
		if (sqlite3_bind_int(inserter, 6, s.length) != SQLITE_OK)
			throw "Cannot bind length query data";

		if (sqlite3_step(inserter) != SQLITE_DONE)
			throw "Cannot insert song into database";
		if (sqlite3_reset(inserter) != SQLITE_OK)
			throw "Cannot reset insertion query";
	}

	sqlite3_finalize(inserter);

	LoadFullList();
}

unsigned Library::QueryCount() const
{
	sqlite3_stmt *query;
	if (sqlite3_prepare_v2(m_db, "SELECT count(*) FROM songs;",
				128, &query, nullptr))
		throw "Couldn't create count query";

	if (sqlite3_step(query) != SQLITE_ROW)
		throw "Couldn't count database entries";

	const unsigned count = sqlite3_column_int(query, 0);

	sqlite3_finalize(query);

	return count;
}

Song Library::WithId(unsigned id)
{
	sqlite3_stmt *query;
	if (sqlite3_prepare_v2(m_db,
				"SELECT path, title, artist, album, track, length "
				"FROM songs WHERE rowid = ?;", 128, &query, nullptr))
		throw "Couldn't create song selection query";

	if (sqlite3_bind_int(query, 1, (int)id) != SQLITE_OK)
		throw "Cannot bind id query data";

	const int result = sqlite3_step(query);
	if (result == SQLITE_DONE)
		throw "Invalid song index";
	else if (result != SQLITE_ROW)
		throw "Cannot query database";

	Song s;
	s.path = (const char *)sqlite3_column_text(query, 0);
	s.title = (const char *)sqlite3_column_text(query, 1);
	s.artist = (const char *)sqlite3_column_text(query, 2);
	s.album = (const char *)sqlite3_column_text(query, 3);
	s.track = sqlite3_column_int(query, 4);
	s.length = sqlite3_column_int(query, 5);

	sqlite3_finalize(query);

	return s;
}

void Library::SimpleQuery(const char *const query)
{
	char *errmsg = nullptr;

	if (sqlite3_exec(m_db, query, nullptr, nullptr, &errmsg) != SQLITE_OK) {
		if (errmsg) {
			fprintf(stderr, "SQL error: %s\n", errmsg);
			sqlite3_free(errmsg);
		}

		throw "Error running SQL query";
	}
}

void Library::LoadFullList()
{
	sqlite3_stmt *query;
	if (sqlite3_prepare_v2(m_db,
				"SELECT path, title, artist, album, track, length FROM songs "
				"ORDER BY artist, album, track, title, rowid;",
				128, &query, nullptr))
		throw "Couldn't create song selection query";

	m_songs.clear();
	m_songs.reserve(QueryCount());

	while (1) {
		const int result = sqlite3_step(query);
		if (result == SQLITE_ROW) {
			Song s;
			s.path = (const char *)sqlite3_column_text(query, 0);
			s.title = (const char *)sqlite3_column_text(query, 1);
			s.artist = (const char *)sqlite3_column_text(query, 2);
			s.album = (const char *)sqlite3_column_text(query, 3);
			s.track = sqlite3_column_int(query, 4);
			s.length = sqlite3_column_int(query, 5);
			m_songs.push_back(s);
		} else if (result == SQLITE_DONE) {
			break;
		} else {
			fprintf(stderr, "SQL Step Error: %d: %s\n",
					result, sqlite3_errmsg(m_db));
			throw "SQL Step Error!";
		}
	}

	sqlite3_finalize(query);
}

void Library::LoadSearch(const std::string &search)
{
	sqlite3_stmt *query;
	if (sqlite3_prepare_v2(m_db,
				"SELECT path, title, artist, album, track, length FROM songs "
				"WHERE title LIKE  ? "
				// "WHERE ((title+artist+album) LIKE  ?) "
				"ORDER BY artist, album, track, title, rowid;",
				128, &query, nullptr))
		throw "Couldn't create song selection query";

	if (sqlite3_bind_text(query, 1, search.c_str(), search.size(),
				SQLITE_TRANSIENT) != SQLITE_OK)
		throw "Can't bind search query";

	m_songs.clear();

	while (1) {
		const int result = sqlite3_step(query);
		if (result == SQLITE_ROW) {
			Song s;
			s.path = (const char *)sqlite3_column_text(query, 0);
			s.title = (const char *)sqlite3_column_text(query, 1);
			s.artist = (const char *)sqlite3_column_text(query, 2);
			s.album = (const char *)sqlite3_column_text(query, 3);
			s.track = sqlite3_column_int(query, 4);
			s.length = sqlite3_column_int(query, 5);
			m_songs.push_back(s);
		} else if (result == SQLITE_DONE) {
			break;
		} else {
			fprintf(stderr, "SQL Step Error: %d: %s\n",
					result, sqlite3_errmsg(m_db));
			throw "SQL Step Error!";
		}
	}

	sqlite3_finalize(query);
}
