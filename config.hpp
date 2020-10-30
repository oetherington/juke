#pragma once

#include <unordered_map>
#include <string>

class Config {
private:
	std::unordered_map<std::string, std::string> m_map;

public:
	Config();

	inline const std::string &Get(const std::string &key)
	{
		return m_map[key];
	}

	inline const std::string &Get(const char *const key)
	{
		return m_map[key];
	}
};
