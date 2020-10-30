#include "config.hpp"
#include "library.hpp"
#include "player.hpp"
#include "status.hpp"
#include "termbox/termbox.h"
extern "C" {
#include "UTF8/UTF8.h"
}
#include <vector>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <stdexcept>
#include <climits>
#include <execinfo.h>
#include <signal.h>

#define COL_REVERSE (TB_DEFAULT | TB_REVERSE)

enum class Mode {
	Browse,
	Edit,
	Select,
};

static bool g_initialized = false;
static bool g_exit = false;
static Mode g_mode = Mode::Browse;
static std::string g_status("");
static std::string g_edit("");
static unsigned g_cursor = 0;
static Player g_player;
static Library g_library;
static size_t g_scroll = 0;
static size_t g_hover = 0;
static size_t g_browse_rows = 0;
static size_t g_playing = INT_MAX;
static std::vector<size_t> g_selection;

static inline void DrawString(size_t w, size_t start_x, size_t y,
		const std::string &s, int fg = TB_DEFAULT, int bg = TB_DEFAULT,
		bool fill_line = true)
{
	size_t i = 0;

	if (s.size()) {
		utf8_iterator iter;
		utf8_init(&iter, s.c_str());

		while (utf8_next(&iter)) {
			const size_t x = start_x + i;
			if (x >= w)
				break;
			i++;
			uint32_t ch;
			tb_utf8_char_to_unicode(&ch, utf8_getchar(&iter));
			tb_change_cell(x, y, ch, fg, bg);
		}
	}

	if (fill_line) {
		i += start_x;
		for ( ; i < w; i++)
			tb_change_cell(i, y, ' ', fg, bg);
	}
}

static void MakeLengthString(std::string &out, unsigned length)
{
	const size_t n = 6;
	char buf[6];
	const unsigned mins = length / 60;
	const unsigned secs = length % 60;
	snprintf(buf, n, "%.2u:%.2u", mins, secs);
	out = buf;
}

static void DrawSongList(size_t w, size_t start_y, size_t end_y)
{
	const size_t title_x = 0;
	const size_t length_w = 6;
	const size_t length_x = w - length_w;
	const size_t artist_x = length_x / 3;
	const size_t album_x = artist_x * 2;

	const int headfg = TB_WHITE;
	const int headbg = TB_BLUE;

	DrawString(artist_x - 1, title_x, start_y, "Title", headfg, headbg);
	DrawString(album_x - 1, artist_x, start_y, "Artist", headfg, headbg);
	DrawString(length_x - 1, album_x, start_y, "Album", headfg, headbg);
	DrawString(w, length_x, start_y, "Length", headfg, headbg);

	start_y += 1;

	const size_t height = end_y - start_y;
	const size_t count = g_library.Count() - g_scroll;
	g_browse_rows = (height < count ? height : count) - 1;

	std::string length_string;
	length_string.reserve(length_w);

	for (size_t i = 0; i <= g_browse_rows; i++) {
		const size_t idx = i + g_scroll;
		const Song &s = g_library.At(idx);

		int fg, bg;
		if (idx == g_hover && idx == g_playing) {
			fg = TB_GREEN;
			bg = COL_REVERSE;
		} else if (idx == g_hover) {
			fg = bg = COL_REVERSE;
		} else if (idx == g_playing) {
			fg = TB_GREEN;
			bg = TB_DEFAULT;
		} else {
			fg = bg = TB_DEFAULT;
		}


		MakeLengthString(length_string, s.length);

		DrawString(artist_x - 1, title_x, start_y + i, s.title, fg, bg);
		DrawString(album_x - 1, artist_x, start_y + i, s.artist, fg, bg);
		DrawString(length_x - 1, album_x, start_y + i, s.album, fg, bg);
		DrawString(w, length_x, start_y + i, length_string, fg, bg);
	}
}

static void Draw()
{
	const size_t w = tb_width();
	const size_t h = tb_height();

	const size_t status_y = g_mode == Mode::Edit ? h - 2 : h - 1;
	const size_t edit_y = h - 1;

	tb_clear();

	DrawString(w, 0, status_y, g_status, COL_REVERSE, COL_REVERSE);

	if (g_mode == Mode::Edit) {
		tb_set_cursor(2 + g_cursor, edit_y);
		DrawString(w, 0, edit_y, "> ", TB_DEFAULT, TB_DEFAULT, false);
		DrawString(w, 2, edit_y, g_edit);
	} else if (g_mode == Mode::Browse) {
		tb_set_cursor(TB_HIDE_CURSOR, TB_HIDE_CURSOR);
	}

	DrawSongList(w, 0, status_y);

	tb_present();
}

static int Execute(const std::string &query)
{
	if (query == "exit" || query == "quit") {
		g_exit = true;
	} else if (query == "scan") {
		g_library.Scan();
		SetStatus("Scanning library on filesystem...");
	} else {
		g_library.LoadSearch(query);
	}

	return 0;
}

static void SubmitEdit()
{
	const std::string query = g_edit;
	g_edit = "";
	g_cursor = 0;
	if (Execute(query))
		SetStatus("Invalid query: \"" + query + "\"");
}

static void PlayLibraryIndex(size_t idx)
{
	const Song &s = g_library.At(idx);
	g_playing = idx;
	g_player.Open(s.path);
	g_player.Play();
	SetStatus(std::string("Playing: ") + s.title + " - " +
			s.artist + " - " + s.album);
}

static void SelectScreenRow(int row, bool play)
{
	const int y_offs = 1;
	int select = row - y_offs + g_scroll;

	if (select < 0)
		select = 0;

	if (select >= (int)g_library.Count())
		select = (int)g_library.Count() - 1;

	g_hover = (size_t)select;

	if (play)
		PlayLibraryIndex(g_hover);
}

static void ScrollDown()
{
	g_hover++;

	if (g_hover >= g_library.Count())
		g_hover = g_library.Count() - 1;
	else if (g_hover > g_scroll + g_browse_rows)
		g_scroll = g_hover - g_browse_rows;
}

static void ScrollUp()
{
	if (g_hover == 0)
		return;

	g_hover--;

	if (g_hover < g_scroll)
		g_scroll = g_hover;
}

static void ScrollToEnd()
{
	if (g_hover == g_library.Count() - 1) {
		g_hover = 0;
		g_scroll = 0;
	} else {
		g_hover = g_library.Count() - 1;
		g_scroll = g_hover - g_browse_rows;
	}
}

static void HandleKeyBrowse(const int key)
{
	switch (key) {
	case TB_KEY_SPACE:
		g_player.Pause();
		break;

	case TB_KEY_ENTER:
		PlayLibraryIndex(g_hover);
		break;

	case TB_KEY_ARROW_UP:
		g_player.VolumeUp();
		break;

	case TB_KEY_ARROW_DOWN:
		g_player.VolumeDown();
		break;

	default:
		break;
	}
}

static void HandleCharBrowse(const int ch)
{
	switch (ch) {
	case 'i':
		g_mode = Mode::Edit;
		SetStatus("Enter query...");
		break;

	case 'j':
		ScrollDown();
		break;

	case 'k':
		ScrollUp();
		break;


	case 'G':
		ScrollToEnd();
		break;

	default:
		break;
	}
}

static void HandleKeyEdit(const int key)
{
	switch (key) {
	case TB_KEY_ESC:
		g_mode = Mode::Browse;
		SetStatus("Juke Music Player");
		break;

	case TB_KEY_SPACE:
		g_edit.insert(g_cursor, 1, ' ');
		g_cursor += 1;
		break;

	case TB_KEY_BACKSPACE:
	case TB_KEY_BACKSPACE2:
		if (g_cursor > 0) {
			g_edit.erase(g_cursor - 1, 1);
			g_cursor -= 1;
		}
		break;

	case TB_KEY_DELETE:
		if (g_cursor < g_edit.size())
			g_edit.erase(g_cursor, 1);
		break;

	case TB_KEY_ARROW_LEFT:
		if (g_cursor > 0)
			g_cursor -= 1;
		break;

	case TB_KEY_ARROW_RIGHT:
		if (g_cursor < g_edit.size())
			g_cursor += 1;
		break;

	case TB_KEY_ARROW_UP:
		g_player.VolumeUp();
		break;

	case TB_KEY_ARROW_DOWN:
		g_player.VolumeDown();
		break;

	case TB_KEY_ENTER:
		SubmitEdit();
		break;

	default:
		break;
	}
}

static void HandleCharEdit(const int ch)
{
	g_edit.insert(g_cursor, 1, ch);
	g_cursor += 1;
}

static void HandleInput(const int key, const int ch)
{
	if (g_mode == Mode::Browse) {
		if (key)
			HandleKeyBrowse(key);
		else
			HandleCharBrowse(ch);
	} else if (g_mode == Mode::Edit) {
		if (key)
			HandleKeyEdit(key);
		else
			HandleCharEdit(ch);
	}
}

static int HandleException(const char *const msg)
{
	if (g_initialized)
		tb_shutdown();
	fprintf(stderr, "Juke: Uncaught exception: %s\n", msg);
	return 1;
}

static void HandleSegfault(int sig)
{
	if (g_initialized)
		tb_shutdown();

	const size_t n = 32;
	void *array[n];
	const size_t size = backtrace(array, n);

	fprintf(stderr, "Error: Signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

int main(int argc, char **argv)
{
	signal(SIGSEGV, HandleSegfault);

	try {
		PlayerGlobalInit();

		Config cfg;

		{
			const std::string &dir = cfg.Get("directory");

			if (chdir(dir.c_str())) {
				fprintf(stderr, "Can't enter library directory \"%s\"\n",
						dir.c_str());
				return 1;
			}

			SetStatus("Juke Music Player: Using library \"" + dir + "\"");
		}

		g_library.Scan();

		if (tb_init()) {
			fprintf(stderr, "Can't initialize terminal\n");
			return 1;
		}

		g_initialized = true;

		tb_select_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

		Draw();

		struct tb_event ev;
		while (!g_exit) {
			bool dirty = false;

			if (tb_peek_event(&ev, 10)) {
				switch (ev.type) {
				case TB_EVENT_KEY:
					if (ev.key == TB_KEY_CTRL_Q)
						g_exit = true;
					else
						HandleInput(ev.key, ev.ch);
					break;

				case TB_EVENT_RESIZE:
					dirty = true;
					break;

				case TB_EVENT_MOUSE:
					if (ev.key == TB_KEY_MOUSE_WHEEL_DOWN) {
						ScrollDown();
					} else if (ev.key == TB_KEY_MOUSE_WHEEL_UP) {
						ScrollUp();
					} else if (ev.key == TB_KEY_MOUSE_LEFT) {
						SelectScreenRow(ev.y, false);
					} else if (ev.key == TB_KEY_MOUSE_RIGHT) {
						SelectScreenRow(ev.y, true);
					}
					break;

				default:
					break;
				}

				dirty = true;
			}

			if (StatusChanged()) {
				g_status = GetStatus();
				dirty = true;
			}

			if (g_player.IsFinished()) {
				g_player.SetFinished(false);
				g_playing++;
				if (g_playing < g_library.Count())
					PlayLibraryIndex(g_playing);
			}

			if (dirty)
				Draw();
		}

		if (g_initialized)
			tb_shutdown();

		PlayerGlobalDestroy();
	} catch (const char *const s) {
		return HandleException(s);
	} catch (const std::runtime_error &e) {
		return HandleException(e.what());
	} catch (...) {
		return HandleException(nullptr);
	}

	return 0;
}
