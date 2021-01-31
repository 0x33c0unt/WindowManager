
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <list>
#define print(fmt, ...) printf(fmt "\n", __VA_ARGS__)

POINT lastPoint;
int lastTick;
std::list<HWND> hWnds;


#include <tchar.h>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383


HWND hConsole;

void disableWindows(HWND hWnd)
{
	bool found = false;
	std::list <HWND>::iterator i;
	for (i = hWnds.begin(); i != hWnds.end(); i++)
	{
		if (hWnd == *i)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		EnableWindow(hWnd, 0);
		hWnds.push_back(hWnd);
	}
}

HKEY hKey;
DWORD buffersize = 1024;
char* lpData = new char[buffersize];

void regGetDefaultBeep()
{
	RegOpenKeyExA(HKEY_CURRENT_USER, "AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current", NULL, KEY_READ, &hKey);
	RegQueryValueExA(hKey, "", NULL, NULL, (LPBYTE)lpData, &buffersize);
	RegCloseKey(hKey);
}

void regSetDefaultBeep(char* newData)
{
	RegOpenKeyExA(HKEY_CURRENT_USER, "AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current", NULL, KEY_SET_VALUE, &hKey);
	RegSetValueA(hKey, "", REG_SZ, newData, 0);
	RegCloseKey(hKey);
}

void move(HWND hWnd, int width, int height, RECT rect, POINT point)
{
	disableWindows(hWnd);
	int newX = 0;
	int newY = 0;
	int newWidth = 0;
	int newHeight = 0;
	int flags = SWP_NOZORDER;
	if (IsZoomed(hWnd))
	{
		ShowWindow(hWnd, SW_RESTORE);
		while (IsZoomed(hWnd));
		newWidth = width / 2;
		newHeight = height / 2;
		flags |= SWP_NOMOVE;
		print("Exited from Full Screen");
	}
	else {
		newWidth = width;
		newHeight = height;
		newX = rect.left + (point.x - lastPoint.x);
		newY = rect.top + (point.y - lastPoint.y);
		print("MOVE X: %d Y: %d", newX, newY);
	}

	SetWindowPos(
		hWnd,
		NULL,
		newX,
		newY,
		newWidth,
		newHeight,
		flags
	);
}

char windowText[256];

char* getWindowText(HWND hWnd)
{
	GetWindowTextA(hWnd, windowText, 255);
	return windowText;
}

void resize(HWND hWnd, int width, int height, RECT rect, POINT point)
{
	disableWindows(hWnd);
	int newWidth = width + (point.x - lastPoint.x);
	int newHeight = height + (point.y - lastPoint.y);
	print("RESIZE Width: %d Height: %d", newWidth, newHeight);
	SetWindowPos(
		hWnd,
		NULL,
		rect.left,
		rect.top,
		newWidth,
		newHeight,
		0x40
	);
}

void setOnTop(HWND hWnd)
{
	if (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
		SetWindowPos(
			hWnd,
			HWND_NOTOPMOST,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE
		);

	else {
		SetWindowPos(
			hWnd,
			HWND_TOPMOST,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE
		);
	}
}

void kill(HWND hWnd)
{
	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);
	HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);
	TerminateProcess(h, 0);
}

void keyboardEvents(HWND hWnd)
{
	if (GetAsyncKeyState('T') && !IsZoomed(hWnd))
		setOnTop(hWnd);
	else if (GetAsyncKeyState('K') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
	{
		kill(hWnd);
		Sleep(300);
	}
	else if (GetAsyncKeyState('I') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
		exit(0);
	else if (GetAsyncKeyState('L') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
	{
		int r = GetWindowLong(hConsole, GWL_STYLE);
		if (r & WS_VISIBLE)
			ShowWindow(hConsole, SW_HIDE);
		else
			ShowWindow(hConsole, SW_SHOW);
		Sleep(300);
	}
}
void mouseEvents(HWND hWnd)
{
	RECT rect;
	GetWindowRect(hWnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	POINT point;
	if (GetCursorPos(&point)) {
		if (GetTickCount() - lastTick > 100)
			lastPoint = point;
		bool position_change = ((point.x - lastPoint.x) || (point.y - lastPoint.y));
		if (position_change)
		{
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
				move(hWnd, width, height, rect, point);
			else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
				resize(hWnd, width, height, rect, point);
		}
		lastPoint = point;
		lastTick = GetTickCount();
	}
}


bool beep = true;

int main()
{
	hConsole = GetConsoleWindow();
	ShowWindow(hConsole, SW_SHOW);
	regGetDefaultBeep();
	char empty = '\0';
	while (true)
	{
		Sleep(1);
		HWND hWnd = GetForegroundWindow();
		
		if (GetAsyncKeyState(VK_LMENU) && !GetAsyncKeyState(VK_SHIFT)  && !GetAsyncKeyState(VK_TAB))
		{
			if (beep)
			{
				regSetDefaultBeep(&empty);
				beep = false;
			}
			
			mouseEvents(hWnd);

			keyboardEvents(hWnd);
		}
		else
		{
			std::list <HWND>::iterator i;
			for (i= hWnds.begin(); i != hWnds.end(); i++)
				EnableWindow(*i, 1);
			hWnds.clear();
			if (!beep)
			{
				regSetDefaultBeep(lpData);
				beep = true;
			}
		}

	}
}


