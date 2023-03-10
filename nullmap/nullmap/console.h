#pragma once

enum ConsoleColor
{
	Default,
	DarkBlue,
	DarkGreen,
	DarkCyan,
	DarkRed,
	DarkPurple,
	DarkYellow,
	DarkWhite,
	DarkGrey,
	DarkBlueLite,
	Green,
	Cyan,
	Red,
	Purple,
	Yellow,
	White
};

void ConsoleBase(enum ConsoleColor color, const char* prefix, const char* text, va_list args);
void ConsoleInfo(const char* text, ...);
void ConsoleWarning(const char* text, ...);
void ConsoleError(const char* text, ...);
void ConsoleSuccess(const char* text, ...);
void ConsoleTitle(const char* name);