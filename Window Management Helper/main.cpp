#define print(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#include <Windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <iostream>
#include <list>
#include <tchar.h>
#include <fstream>
#include <sstream>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
using namespace std;

struct _threads
{
	THREADENTRY32 te32[256];
	DWORD count = 0;
};
typedef struct _threads threads;

HWND console_handle;

HANDLE handle;
char windowText[256];
char windowProcess[256];


POINT last_point;
int last_tick;
int last_left_click;
//config
string dll_path;
string dll64_path;
bool first_run;


list<HWND> hWnds;
HHOOK keyboard_hook;
HHOOK mouse_hook;
KBDLLHOOKSTRUCT kbdStruct;
MSLLHOOKSTRUCT mbdStruct;

void keyboard_events(DWORD key);
void mouse_events(DWORD key, MSLLHOOKSTRUCT msInfo);
char* get_window_text(HWND hWnd);
char* get_window_process(HWND hWnd);


DWORD hwnd_to_pid(HWND hWnd)
{
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	return dwProcessId;
}

LRESULT __stdcall keyboard_hook_callback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		
		if (wParam == WM_SYSKEYDOWN || wParam == WM_KEYDOWN )
		{
			kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
		    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000 && GetAsyncKeyState(VK_LMENU) & 0x8000 && !((GetAsyncKeyState(VK_SHIFT) & 0x8000) | (GetAsyncKeyState(VK_TAB) & 0x8000)))
				keyboard_events(kbdStruct.vkCode);
			
		}
	}
	return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
}
bool left_button = false;
bool right_button = false;
LRESULT __stdcall mouse_hook_callback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
			mbdStruct = *((MSLLHOOKSTRUCT*)lParam);
			

			LPMSG msg = (LPMSG)lParam;

			switch (wParam)
			{
			case 0x201:
				left_button = true;
				break;
			case 0x202:
				left_button = false;
				break;
			case 0x204:
				right_button = true;
				break;
			case 0x205:
				right_button = false;
				break;
			}
			if ((wParam == 0x200 || wParam == 0x201 || wParam == 0x204) && GetAsyncKeyState(VK_LMENU) & 0x8000 && !((GetAsyncKeyState(VK_SHIFT) & 0x8000) | (GetAsyncKeyState(VK_TAB) & 0x8000)))
			{
				mouse_events(wParam, mbdStruct);
				if (wParam == 0x201 || wParam == 0x204)
					return 1; // cancel mouse activity
			}
	}
	
	return CallNextHookEx(mouse_hook, nCode, wParam, lParam);
}

void set_hook()
{
	if (!(keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook_callback, GetModuleHandle(NULL), 0)))
	{
		MessageBox(NULL, "Failed to install hook!", "Error", MB_ICONERROR);
	}
	if (!(mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_hook_callback, GetModuleHandle(NULL), 0)))
	{
		MessageBox(NULL, "Failed to mouse hook!", "Error", MB_ICONERROR);
	}
}

void release_hook()
{
	UnhookWindowsHookEx(keyboard_hook);
	UnhookWindowsHookEx(mouse_hook);
}

threads* ListProcessThreads(DWORD dwOwnerPID)
{
	threads* _threads = (threads*)malloc(sizeof(threads));
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;

	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return _threads;

	int i = 0;
	_threads->te32[i].dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnap, &_threads->te32[i]))
	{
		CloseHandle(hThreadSnap);
		return _threads;
	}
	do
	{
		if (_threads->te32[i].th32OwnerProcessID == dwOwnerPID)
			i++;

		if (i == 256)
		{
			print("Maximum Thread Count Exceeded");
			break;
		}
		_threads->te32[i].dwSize = sizeof(THREADENTRY32);
	} while (Thread32Next(hThreadSnap, &_threads->te32[i]));

	CloseHandle(hThreadSnap);
	_threads->count = i;
	return _threads;
}

BOOL ResumeProcess(DWORD pid)
{
	threads* _threads = ListProcessThreads(pid);

	for (DWORD i = 0; i < _threads->count; i++)
	{
		HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, NULL, _threads->te32[i].th32ThreadID);
		if (hThread == NULL)
		{
			free(_threads);
			return false;
		}
		if (ResumeThread(hThread) == -1)
		{
			free(_threads);
			return false;
		}
	}
	free(_threads);
	return true;
}

BOOL SuspendProcess(DWORD pid)
{
	threads* _threads = ListProcessThreads(pid);
	for (DWORD i = 0; i < _threads->count; i++)
	{
		HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, NULL, _threads->te32[i].th32ThreadID);
		if (hThread == NULL)
		{
			free(_threads);
			return false;
		}
		if (SuspendThread(hThread) == -1)
		{
			free(_threads);
			return false;
		}
	}
	free(_threads);
	return true;

}

DWORD inject_basic(HWND hWnd)
{
	
	DWORD dwProcessId = hwnd_to_pid(hWnd);;
	
	handle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_SET_INFORMATION | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, 0, dwProcessId);

	PVOID dll_path_adr = VirtualAllocEx(handle, NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (dll_path_adr == NULL)
	{
		print("VirtualAllocEx returned 0");
		return 0;
	}
	BOOL is32bit;

	IsWow64Process(handle, &is32bit);
	const char* Dll;

	if (is32bit)
	{
		if (dll_path.empty())
		{
			print("no dll found (32 bit)");
			return 0;
		}
		Dll = dll_path.c_str();
	}
	else
	{
		if (dll64_path.empty())
		{
			print("no dll found (64 bit)");
			return 0;
		}
		Dll = dll64_path.c_str();
	}


	print("%x %s", is32bit, Dll);

	WriteProcessMemory(handle, dll_path_adr, Dll, strlen(Dll), NULL);

	HANDLE hThread = CreateRemoteThread(handle, NULL, 0, (LPTHREAD_START_ROUTINE)&LoadLibraryA, dll_path_adr, NULL, 0);
	if (hThread == NULL)
		return 0;

	DWORD r;
	WaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread, &r);

	get_window_process(hWnd);

	if (r > 0)
		print("%s injected to %s, Base: %x", (char*)(Dll + string(Dll).find_last_of("\\/") + 1), windowProcess, r);
	else
		print("Dll Injection Failed, further info;\nDll: %s\nProcess: %s", Dll, windowProcess);

	VirtualFreeEx(handle, dll_path_adr, 0, MEM_RELEASE);
	CloseHandle(handle);

	return r;
}


void run_on_startup() 
{
	char path[MAX_PATH];
	char path2[MAX_PATH+2] = "\"";
	int path_len = GetModuleFileNameA(nullptr, path, MAX_PATH);
	strncpy_s(path2 + 1, MAX_PATH + 1, path, path_len);
	path2[path_len+1] = '\"';
	path2[path_len+2] = '\0';
	HKEY hkey = NULL;
	LONG createStatus = RegCreateKey(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey); //Creates a key       
	LONG status = RegSetValueEx(hkey, "WindowManager", 0, REG_SZ, (BYTE*)path2, path_len+2);
}

bool read_config()
{
	ifstream infile("config.txt");
	string line;

	while (getline(infile, line))
	{

		if (!strncmp("dll=", line.c_str(), sizeof("dll=")-1))
			dll_path = line.substr(sizeof("dll=")-1);
		else if(!strncmp("dll64=", line.c_str(), sizeof("dll64=")-1))
			dll64_path = line.substr(sizeof("dll64=")-1);
		else if (!strncmp("first_run=", line.c_str(), sizeof("first_run=") - 1))
			first_run = atoi(line.substr(sizeof("first_run=") - 1).c_str());
	}
	infile.close();
	return true;
}

string get_last_error_as_string()
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

char* get_window_text(HWND hWnd)
{
	GetWindowTextA(hWnd, windowText, 255);
	return windowText;
}

char* get_window_process(HWND hWnd)
{
	DWORD pid = 0;
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

char* get_window_process_full_path(HANDLE h)
{
	DWORD size = 255;
	QueryFullProcessImageNameA(h, 0, windowProcess, &size);
	return windowProcess;
}


HKEY hKey;
DWORD buffersize = 1024;
char* lpData = new char[buffersize];

void exit_full_screen(HWND hWnd, int width, int height, RECT& rect, POINT point)
{
	int newWidth = width / 2;
	int newHeight = height / 2;
	int newX = point.x - newWidth/2;
	int newY = point.y - newHeight/2;
	int flags = SWP_NOZORDER;
	ShowWindow(hWnd, SW_RESTORE);
	while (IsZoomed(hWnd));

	print("%s Exited from Full Screen", get_window_text(hWnd));
	SetWindowPos(
		hWnd,
		NULL,
		newX,
		newY,
		newWidth,
		newHeight,
		flags
	);
	GetWindowRect(hWnd, &rect);
	
}

void move(HWND hWnd, int width, int height, RECT rect, POINT point)
{
	int newX = 0;
	int newY = 0;
	int newWidth = 0;
	int newHeight = 0;
	int flags = SWP_NOZORDER | SWP_NOSIZE;
	
	newWidth = width;
	newHeight = height;
	newX = rect.left + (point.x - last_point.x);
	newY = rect.top + (point.y - last_point.y);
	print("Moving %s to X: %d Y: %d", get_window_text(hWnd), newX, newY);

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
	int newWidth = width + (point.x - last_point.x);
	int newHeight = height + (point.y - last_point.y);
	int flags = SWP_NOZORDER;
	print("Resizing: %s to Width: %d Height: %d", get_window_text(hWnd), newWidth, newHeight);
	SetWindowPos(
		hWnd,
		NULL,
		rect.left,
		rect.top,
		newWidth,
		newHeight,
		flags
	);
}

void set_on_top(HWND hWnd)
{
	if (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
	{
		print("%s on not Top", get_window_text(hWnd));
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
		print("%s on Top", get_window_text(hWnd));
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

inline void print_last_error()
{
	print("%s", get_last_error_as_string().c_str());
}

bool kill(HWND hWnd)
{
	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);
	HANDLE h = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, 0, pid);
	if (h == NULL)
	{
		print_last_error();
		return false;
	}
	get_window_process(hWnd);
	if (TerminateProcess(h, 0))
		print("%s is Terminated", windowProcess);
	else
	{
		print_last_error();
		return false;
	}
	return true;
}

BOOL is_running(DWORD pid)
{
	threads* _threads = ListProcessThreads(pid);
	if (_threads->te32[0].th32ThreadID == GetCurrentThreadId())
		return true;

	HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, NULL, _threads->te32[0].th32ThreadID);
	if (!hThread)
		return true;

	int r = SuspendThread(hThread);
	int rr = ResumeThread(hThread);
	if (r > 0)
		return false;

	return true;
}

void suspend_resume(HWND hWnd)
{
	DWORD pid = hwnd_to_pid(hWnd);

	if (is_running(pid))
	{
		if (SuspendProcess(pid))
			print("Suspended process: %s", get_window_process(hWnd));
	}
	else
		if(ResumeProcess(pid))
			print("Resumed process: %s", get_window_process(hWnd));
}
void keyboard_events(DWORD key)
{
	HWND hWnd = GetForegroundWindow();

	if (key == 'I')
	{
		release_hook();
		exit(0);
	}
	else if (key == 'L')
	{
		int r = GetWindowLong(console_handle, GWL_STYLE);
		if (r & WS_VISIBLE)
			ShowWindow(console_handle, SW_HIDE);
		else
		{
			SetForegroundWindow(console_handle);
			ShowWindow(console_handle, SW_SHOW);
		}
	}

	if (hWnd == console_handle)
		return;

	if (key == 'T' && !IsZoomed(hWnd))
	{
		set_on_top(hWnd);
	}
	else if (key == 'K')
	{
		kill(hWnd);
	}
	else if (key == 'D')
		inject_basic(hWnd);
	else if (key == 'P')
		suspend_resume(hWnd);

}
void mouse_events(DWORD key, MSLLHOOKSTRUCT msInfo)
{
	HWND hWnd = GetForegroundWindow();
	if (hWnd == console_handle)
		return;

	RECT rect;
	GetWindowRect(hWnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	POINT point = msInfo.pt;
	if (GetTickCount() - last_tick > 100)
		last_point = point;

	if (key == 0x201)
	{
		if(GetTickCount() - last_left_click < 200)
			if(IsZoomed(hWnd))
				ShowWindow(hWnd, SW_RESTORE);
			else
				ShowWindow(hWnd, SW_MAXIMIZE);
		last_left_click = GetTickCount();
	}
		
	bool position_change = ((point.x - last_point.x) || (point.y - last_point.y));
	if (position_change)
	{
		if (IsZoomed(hWnd) && GetAsyncKeyState(VK_LBUTTON) & 0x8000 )
			exit_full_screen(hWnd, width, height, rect, point);
		if (left_button)
			move(hWnd, width, height, rect, point);
		else if (right_button)
			resize(hWnd, width, height, rect, point);
	}

	last_point = point;
	last_tick = GetTickCount();
}

int WINAPI msg_handler(int param1)
{
	set_hook();
	MSG Msg;
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return 1;
}

bool is_already_open()
{
	HANDLE event_handle = CreateEvent(NULL, true, false, "WindowManager");
	if (GetLastError() == ERROR_ALREADY_EXISTS) 
		return true;
	return false;
}

int main()
{
	if (is_already_open())
	{
		MessageBox(0, "An instance is already running", "Window Manager", 0);
		return 1;
	}

	read_config();

	console_handle = GetConsoleWindow();
	if (first_run)
	{
		ShowWindow(console_handle, SW_SHOW);
		print("Do you want to run this app on startup? [Y/N]");
		char answer = getchar();
		if (answer == 'y' || answer == 'Y')
			run_on_startup();
		
		ofstream out("config.txt");
		out << "dll=" << dll_path << endl;
		out << "dll64=" << dll64_path << endl;
		out << "first_run=" << 0 << endl;
		out.close();
	}
	ShowWindow(console_handle, SW_HIDE);

	SetConsoleTitle("Window Manager - 0x33c0unt");

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&msg_handler, NULL, NULL, NULL);

	char cmd[256];
	while (true)
	{
		int r = scanf_s("%s", cmd, 255);
		if (!strcmp(cmd, "exit"))
			exit(0);
	}
}


