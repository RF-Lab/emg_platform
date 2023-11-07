// 
//

#include <windows.h>
#include <iostream>
#include "Controller.h"
#include "ApplicationConnection.h"

Controller* gameController = NULL ;
ApplicationConnection* appConnection = NULL ;

int main()
{
    std::wcout << L"Server initialization..."  << std::endl ;

    gameController = new KeyboardController() ;
    appConnection = new ConnectionToUnity() ;
    if (appConnection->Connect()<0)
    {
        return (-1) ;
    }

    while (1)
    {
        int controllerCode = -1 ;
        if (gameController)
        {
            controllerCode = gameController->readSingleCode() ;
        }
        if (controllerCode == 27)
        {
            std::wcout << L"Server terminated." ;
            break ;
        }
        if (controllerCode >= 0)
        {
            appConnection->Send(controllerCode) ;
        }
        Sleep(10) ; // Relax CPU usage
    }


    if (gameController)
    {
        delete gameController ;
    }
    if (appConnection)
    {
        appConnection->Release() ;
        delete appConnection ;
    }

}

