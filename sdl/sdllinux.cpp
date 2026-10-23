/*
 * Descent 3: Piccu Engine
 * [linux-native] Linux implementations of the few OS shims the Windows SDL build gets from win32/.
 * Licensed under the GPL-3.0, like the rest of the engine.
 */
#ifdef __LINUX__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <string>

#include "linux/linux_fix.h"
#include "debug.h"
#include "AppConsole.h"

// --- Win32 memory and misc helpers used by the sound loaders, the UI and the netcode ---
void GlobalFree(void* mptr)
{
	if (mptr)
		free(mptr);
}

void* GlobalAlloc(int flags, int size)
{
	if (size <= 0)
		return NULL;
	return malloc(size);
}

void* GlobalLock(HGLOBAL hMem) { return hMem; }

void Sleep(int millis)
{
	struct timespec ts;
	ts.tv_sec = millis / 1000;
	ts.tv_nsec = (long)(millis % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

char* strupr(char* string)
{
	for (char* p = string; p && *p; p++)
		*p = (char)toupper((unsigned char)*p);
	return string;
}

char* itoa(int value, char* string, int radix)
{
	if (radix == 16)
		sprintf(string, "%x", value);
	else
		sprintf(string, "%d", value);
	return string;
}

// --- debug.h ---
bool Debug_break = false;

bool Debug_Init(bool debugger, bool mono_debug)
{
	Debug_break = debugger;
	return true;
}

int Debug_ErrorBox(int type, const char* topstring, const char* title, const char* bottomstring)
{
	fprintf(stderr, "\n%s(%s)\n\n%s\n", title ? title : "", topstring ? topstring : "", bottomstring ? bottomstring : "");
	return 0;
}

int Debug_MessageBox(int type, const char* title, const char* str)
{
	return Debug_ErrorBox(type, str, title, "");
}

// --- the dedicated-server console (stdin/stdout), the SDL app's con_init/con_shutdown/con_defer ---
static std::string coninputbuf;
static std::string coninputcommand;
static bool con_active = false;

void con_Printf(const char* fmt, ...)
{
	char buf[2048];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	fputs(buf, stdout);
	fflush(stdout);
}

bool con_Input(char* buf, int buflen)
{
	if (coninputcommand.empty())
		return false;
	strncpy(buf, coninputcommand.c_str(), buflen);
	buf[buflen - 1] = 0;
	coninputcommand.clear();
	return true;
}

void con_init() { con_active = true; }

void con_shutdown() { con_active = false; }

// Non-blocking line read from stdin.
void con_defer()
{
	if (!con_active)
		return;
	struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
	while (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN))
	{
		char c;
		if (read(STDIN_FILENO, &c, 1) != 1)
		{
			con_active = false;
			return;
		}
		if (c == '\n' || c == '\r')
		{
			if (!coninputbuf.empty())
			{
				coninputcommand = coninputbuf;
				coninputbuf.clear();
			}
			return;
		}
		coninputbuf += c;
	}
}
#endif
