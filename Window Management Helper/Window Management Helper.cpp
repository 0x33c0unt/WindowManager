
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

void QueryKey(HKEY hKey)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD i, retCode;

	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hKey,                    // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 

	// Enumerate the subkeys, until RegEnumKeyEx fails.

	if (cSubKeys)
	{
		printf("\nNumber of subkeys: %d\n", cSubKeys);

		for (i = 0; i < cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				printf(("(%d) %s\n"), i + 1, achKey);
			}
		}
	}

	// Enumerate the key values. 

	if (cValues)
	{
		printf("\nNumber of values: %d\n", cValues);

		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = 256;
			achValue[0] = '\0';
			retCode = RegEnumValue(hKey, i,
				NULL,
				NULL,
				NULL,
				NULL,
				(LPBYTE)achValue,
				&cchValue);

			if (retCode == ERROR_SUCCESS)
			{
				printf("(%d) %s\n", i + 1, achValue);
			}
		}
	}
}


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
		print("Disabled");
	}
}

HKEY hKey;
DWORD buffersize = 1024;
char* lpData = new char[buffersize];

void regGetDefaultBeep()
{
	RegOpenKeyEx(HKEY_CURRENT_USER, "AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current", NULL, KEY_READ, &hKey);
	RegQueryValueEx(hKey, "", NULL, NULL, (LPBYTE)lpData, &buffersize);
	RegCloseKey(hKey);
}

void regSetDefaultBeep(char* newData)
{
	RegOpenKeyEx(HKEY_CURRENT_USER, "AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current", NULL, KEY_SET_VALUE, &hKey);
	print("%x", RegSetValueA(hKey, "", REG_SZ, newData, 0));
	//RegQueryValueEx(hKey, "", NULL, NULL, (LPBYTE)lpData, &buffersize);
	RegCloseKey(hKey);
}


bool beep = true;

int main()
{
	regGetDefaultBeep();
	char empty = '\0';
	while (true)
	{
		Sleep(10);
		HWND hWnd = GetForegroundWindow();
		
		if (GetAsyncKeyState(VK_MENU))
		{
			if (beep)
			{
				regSetDefaultBeep(&empty);
				beep = false;
			}
			disableWindows(hWnd);
			char text[256];
			GetWindowTextA(hWnd, text, 255);
			RECT rect;
			GetWindowRect(hWnd, &rect);
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			
			POINT point;
			if (GetCursorPos(&point)) {
				if (GetTickCount() - lastTick > 1000)
					lastPoint = point;
				if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
				{
					
					int newX;
					int newY;
					int newWidth;
					int newHeight;
					int flags = SWP_NOZORDER;
					if (IsZoomed(hWnd)) 
					{
						ShowWindow(hWnd, SW_RESTORE);
						while (IsZoomed(hWnd));
						newWidth = width / 2;
						newHeight = height / 2;
						flags |= SWP_NOMOVE;
						
					}
					else{
						newWidth = width;
						newHeight = height;
						newX = rect.left + (point.x - lastPoint.x);
						newY = rect.top + (point.y - lastPoint.y);
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
				else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
				{
				//disableWindows(hWnd);
					SetWindowPos(
						hWnd,
						NULL,
						rect.left,
						rect.top,
						width + (point.x - lastPoint.x),
						height + (point.y - lastPoint.y),
						0x40
					);
				}
				lastPoint = point;
				lastTick = GetTickCount();
			}
			//
		}
		else
		{
			std::list <HWND>::iterator i;
			for (i= hWnds.begin(); i != hWnds.end(); i++)
			{
				EnableWindow(*i, 1);
				print("ENABLED");
			}
			hWnds.clear();
			if (!beep)
			{
				regSetDefaultBeep(lpData);
				beep = true;
			}
		}
		/*if (GetAsyncKeyState(VK_RBUTTON))
			print("VK_RBUTTON");
		if (GetAsyncKeyState(VK_LBUTTON))
			print("VK_LBUTTON");*/

	}
}


