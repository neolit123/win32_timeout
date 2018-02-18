#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <timeout_utils.h>

/* a wrapper for printf() that supports foreground and background colors */
void printfColor(ConsoleColor foreground, ConsoleColor background, const char *format, ...)
{
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD savedAttributes;
	va_list args;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	savedAttributes = consoleInfo.wAttributes;

	if (foreground == FCOLOR_NULL)
		foreground = savedAttributes & 0x000F;
	if (background == BCOLOR_NULL)
		background = savedAttributes & 0x00F0;

	SetConsoleTextAttribute(hConsole, (savedAttributes & 0xFF00) | background | foreground);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	SetConsoleTextAttribute(hConsole, savedAttributes);
}


void storeConsoleAttributes(void)
{
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	console_attributes = consoleInfo.wAttributes;
}

void restoreConsoleAttributes(void)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, console_attributes);
}

void setExecutionState(void)
{
	execution_state = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
}

void restoreExecutionState(void)
{
	SetThreadExecutionState(execution_state);
}

FTYPE roundLocal(FTYPE x)
{
	return x < 0 ? ceil(x - 0.5) : floor(x + 0.5);
}
