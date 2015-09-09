#pragma once

#include <string>

#ifdef UNICODE
#define tstring std::wstring
#else
#define tstring std::string
#endif
