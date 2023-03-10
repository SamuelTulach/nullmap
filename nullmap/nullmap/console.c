#include "general.h"

void ConsoleBase(enum ConsoleColor color, const char* prefix, const char* text, va_list args)
{
	const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(consoleHandle, White);
	printf("[");

	SetConsoleTextAttribute(consoleHandle, color);
	printf("%s", prefix);

	SetConsoleTextAttribute(consoleHandle, White);
	printf("] ");

	SetConsoleTextAttribute(consoleHandle, DarkWhite);
	vprintf(text, args);
	printf("\n");
}

void ConsoleInfo(const char* text, ...)
{
	va_list args;
	va_start(args, text);
	ConsoleBase(Cyan, "i", text, args);
	va_end(args);
}

void ConsoleWarning(const char* text, ...)
{
	va_list args;
	va_start(args, text);
	ConsoleBase(Yellow, "w", text, args);
	va_end(args);
}

void ConsoleError(const char* text, ...)
{
	va_list args;
	va_start(args, text);
	ConsoleBase(Red, "e", text, args);
	va_end(args);
}

void ConsoleSuccess(const char* text, ...)
{
	va_list args;
	va_start(args, text);
	ConsoleBase(Green, "s", text, args);
	va_end(args);
}

void ConsoleTitle(const char* name)
{
	const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(consoleHandle, Purple);
	printf("%s\n", name);

	SetConsoleTextAttribute(consoleHandle, DarkWhite);
	printf("build on %s\n\n", __DATE__);
}