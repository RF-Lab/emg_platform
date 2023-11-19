// 
//

#include <windows.h>
#include <iostream>
#include "Controller.h"
#include "EmgController.h"
#include "ApplicationConnection.h"

Controller* gameController = NULL ;
ApplicationConnection* appConnection = NULL ;
WSADATA wsd ;

int main()
{
    std::wcout << L"Server initialization..."  << std::endl ;

    std::wcout << L"Initialize networking..." << std::endl ;
    if (WSAStartup(MAKEWORD(2, 1), &wsd) != 0)
    {
        std::wcout << L"Error WSAStartup" << std::endl ;
        return (-1) ;
    }

    gameController = new EmgController() ;//new KeyboardController() ;
    appConnection = new ConnectionToUnity() ;
    if (!gameController->Connect())
    {
        return (-1) ;
    }
    if (appConnection->Connect()<0)
    {
        return (-1) ;
    }

    while (1)
    {
        int controllerCode = -1 ;
        if (gameController)
        {
            //controllerCode = gameController->readSingleCode() ;
            float* probVector = gameController->readProbVector() ;
            if (nullptr != probVector)
            {
                appConnection->Send(probVector, 3);
            }
        }
        /*
        if (controllerCode == 27)
        {
            std::wcout << L"Server terminated." ;
            break ;
        }
        if (controllerCode >= 0)
        {
            appConnection->Send(controllerCode) ;
        }
        */
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

    WSACleanup() ;

}

