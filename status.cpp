#include "status.hpp"

static std::string g_status = "";
static bool g_changed = true;

void SetStatus(std::string s)
{
	g_status = s;
	g_changed = true;
}

void SetTempStatus(std::string s)
{

}

const std::string &GetStatus()
{
	g_changed = false;
	return g_status;
}

bool StatusChanged()
{
	return g_changed;
}
