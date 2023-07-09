#include <sstream>
#include <string>
#include <string_view>

#define __L(s) L##s
#define _L(s) __L(s)

#ifdef _WIN32

#include <tchar.h>

typedef wchar_t tchar;
typedef std::wstring tstring;
typedef std::wstring_view tstring_view;
typedef std::wstringstream tstringstream;

#else

#define _T(s) s

typedef char tchar;
typedef std::string tstring;
typedef std::string_view tstring_view;
typedef std::stringstream tstringstream;

#endif
