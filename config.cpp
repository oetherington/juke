#include "config.hpp"
#include "INIReader.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#define CONFIG_FILE "/.juke.ini"

Config::Config()
{
	struct passwd *pw = getpwuid(getuid());
	const std::string home(pw->pw_dir);

	try {
		INIReader reader(home + CONFIG_FILE);

		m_map["directory"] = reader.Get("juke", "directory", "~/Music");
	} catch (...) {}
}
