#include <windows.h>
#include <iostream>
#include "Controller.h"


int KeyboardController::readSingleCode()
{
	HANDLE handle = GetStdHandle(STD_INPUT_HANDLE) ;
	DWORD events ;
	INPUT_RECORD buffer ;
	PeekConsoleInput(handle, &buffer, 1, &events) ;
	if (events > 0)
	{
		ReadConsoleInput(handle, &buffer, 1, &events) ;
		if (buffer.EventType == KEY_EVENT)
		{
			if (buffer.Event.KeyEvent.bKeyDown)
			{
				std::wcout << buffer.Event.KeyEvent.wVirtualKeyCode << std::endl ;
				if (buffer.Event.KeyEvent.wVirtualKeyCode == 39)
				{
					std::wcout << L"keyb_controller: right recognized" << std::endl ;
					return (0) ; // Move left
				}
				else if (buffer.Event.KeyEvent.wVirtualKeyCode == 37)
				{
					std::wcout << L"keyb_controller: left recognized" << std::endl;
					return (1) ; // Move right
				}
				else if (buffer.Event.KeyEvent.wVirtualKeyCode == 32)
				{
					std::wcout << L"keyb_controller: space recognized" << std::endl;
					return (2) ; // Rocket
				}
				else if (buffer.Event.KeyEvent.wVirtualKeyCode == 27)
				{
					std::wcout << L"keyb_controller: ESC" << std::endl ;
					return (27) ; // Rocket
				}
			}
		}
	}
	return (-1) ;
}
