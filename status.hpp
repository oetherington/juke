#pragma once

#include <string>

void SetStatus(std::string s);
void SetTempStatus(std::string s);
const std::string &GetStatus();
bool StatusChanged();
