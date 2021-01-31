#define print(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#include <Windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <iostream>
#include <list>


POINT lastPoint;
int lastTick;



#include <tchar.h>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383


HWND hConsole;

char windowText[256];
char windowProcess[256];

using namespace std;

list<HWND> hWnds;

string GetLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return string();
	}

	LPSTR messageBuffer = nullptr;

	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	string message(messageBuffer, size);

	LocalFree(messageBuffer);

	return message;
}

char* getWindowText(HWND hWnd)
{
	GetWindowTextA(hWnd, windowText, 255);
	return windowText;
}

char* getWindowProcess(HWND hWnd)
{
	DWORD pid;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hWnd != 0)
	{
		GetWindowThreadProcessId(hWnd, &pid);
	}
	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (pid == entry.th32ProcessID)
			{
				strcpy_s((char*)windowProcess, 255, (char*)&entry.szExeFile);
				return windowProcess;
			}
		}
	}
	return 0;
}
char* getWindowProcessFullPath(HANDLE h)
{
	DWORD size = 255;
	QueryFullProcessImageNameA(h, 0, windowProcess, &size);
	return windowProcess;
}



void disableWindows(HWND hWnd)
{
	bool found = false;
	list <HWND>::iterator i;
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
		print("%s Exited from Full Screen", getWindowText(hWnd));
	}
	else {
		newWidth = width;
		newHeight = height;
		newX = rect.left + (point.x - lastPoint.x);
		newY = rect.top + (point.y - lastPoint.y);
		print("Moving %s to X: %d Y: %d", getWindowText(hWnd), newX, newY);
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

void resize(HWND hWnd, int width, int height, RECT rect, POINT point)
{
	disableWindows(hWnd);
	int newWidth = width + (point.x - lastPoint.x);
	int newHeight = height + (point.y - lastPoint.y);
	print("Resizing: %s to Width: %d Height: %d %", getWindowText(hWnd), newWidth, newHeight);
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
	{
		print("%s on not Top", getWindowText(hWnd));
		SetWindowPos(
			hWnd,
			HWND_NOTOPMOST,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE
		);
	}
	else {
		print("%s on Top", getWindowText(hWnd));
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

inline void printLastError()
{
	print("%s", GetLastErrorAsString().c_str());
}

bool kill(HWND hWnd)
{
	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);
	HANDLE h = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, 0, pid);
	if (h == NULL)
	{
		printLastError();
		return false;
	}
	if (TerminateProcess(h, 0))
	{
		if (getWindowProcess(hWnd))
			print("%s is Terminated", windowProcess);
		else
		{
			printLastError();
			return false;
		}
	}
	else
	{
		printLastError();
		return false;
	}
	return true;
}

void test()
{
	HWND hWnd = GetForegroundWindow();
	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);
	HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, 1, pid);
	getWindowProcess(hWnd);
}

void keyboardEvents(HWND hWnd)
{
	if (GetAsyncKeyState('T') && !IsZoomed(hWnd))
	{
		setOnTop(hWnd);
		Sleep(300);
	}
	else if (GetAsyncKeyState('K') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
	{
		kill(hWnd);
		Sleep(300);
	}
	else if (GetAsyncKeyState('I') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
		exit(0);
	else if (GetAsyncKeyState('B') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
		test();
	else if (GetAsyncKeyState('L') && GetAsyncKeyState(VK_LCONTROL) & 0x8000)
	{
		int r = GetWindowLong(hConsole, GWL_STYLE);
		if (r & WS_VISIBLE)
			ShowWindow(hConsole, SW_HIDE);
		else
		{
			ShowWindow(hConsole, SW_SHOW);
			BringWindowToTop(hConsole);
		}
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
	SetConsoleTitle("Window Manager - 0x33c0unt");
	hConsole = GetConsoleWindow();
	ShowWindow(hConsole, SW_HIDE);
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
			list <HWND>::iterator i;
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


