#pragma once

#define WINVER       0x0A00
#define _WIN32_WINNT 0x0A00

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define NOGDICAPMASKS
#define NOMENUS
#define NOICONS
#define NORASTEROPS
#define NOOEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NONLS
#define NOMETAFILE
#define NOMINMAX
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

// See https://developercommunity.visualstudio.com/t/wdk-and-sdk-are-not-compatible-with-experimentalpr/695656
#pragma warning( push )
#pragma warning( disable : 5105 )
#include <windows.h>
#pragma warning( pop )