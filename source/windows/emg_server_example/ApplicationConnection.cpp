#include <windows.h>

#include "ApplicationConnection.h"
#include <iostream>

LPCTSTR mutexName = L"testmapmutex" ;
LPCTSTR fileMapName = L"MemoryFile" ;

// Shared memory structure
// +----------------+---------------+-----------------------------*
// |  byte offset   |  type         | Description                 |
// +----------------+---------------+-----------------------------+
// |  0             | int32         | Number of gestures          |
// |  4             | float32       | Probability of gesture #0   |
// |  8             | float32       | Probability of gesture #1   |
// |  12            | float32       | Probability of gesture #2   |
// |  16            | float32       | Probability of gesture #3   |
// |  20            | float32       | Probability of gesture #4   |
// |  24            | float32       | Probability of gesture #5   |
// |  28            | bool8         | Updated flag                |
// |  29            | float32[2000] | Signal                      |
// +----------------+---------------+-----------------------------+
const int NumGestures = 6 ;
__declspec(align(1)) struct mapMemoryDataFormat
{
    int numGestures ;
    float prob[NumGestures] ;
    unsigned char bUpdated ;
} ;

ConnectionToUnity::ConnectionToUnity()
{
    m_hMutex = NULL ;
    m_hFileMap = NULL ;
    m_mapView = NULL ;
}

int ConnectionToUnity::Connect()
{
    if (NULL != m_hMutex)
    {
        std::wcout << L"[ConnectionToUnity]: Already connected!" ;
        return (-1) ;
    }

    // Create synchronization object
    m_hMutex = CreateMutex(
        NULL,           // Default security attributes
        TRUE,           // Initial state : owned
        mutexName
    );

    if (NULL == m_hMutex)
    {
        std::wcout << L"[ConnectionToUnity]: Error: can't create mutex: " << mutexName ;
        return (-1) ;
    }
    std::wcout << L"[ConnectionToUnity]: Mutex " << mutexName << L" created successfully." << std::endl ;


    m_hFileMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        29 + 2000 * 4, // 6 * 4 + 4 + 2000 * 4 + 1,
        fileMapName
    ) ;
    if (NULL == m_hFileMap)
    {
        std::wcout << L"[ConnectionToUnity]: Error: can't create file mapping: " << fileMapName << ". Error: " << GetLastError();
        CloseHandle(m_hMutex); m_hMutex = NULL ;
        return (-1) ;
    }

    m_mapView = MapViewOfFile(m_hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (NULL==m_mapView)
    {
        std::wcout << L"[ConnectionToUnity] Error: can't get access to shared memory: " << fileMapName << ". Error: " << GetLastError();
        CloseHandle(m_hMutex) ; m_hMutex = NULL ;
        CloseHandle(m_hFileMap) ; m_hFileMap = NULL ;
        return (-1) ;
    }

    // Initialize shared memory
    __try
    {
        mapMemoryDataFormat* pSharedData = (mapMemoryDataFormat*)m_mapView ;
        pSharedData->numGestures = NumGestures ;
        pSharedData->prob[0] = 0.0 ;
        pSharedData->prob[1] = 0.0 ;
        pSharedData->prob[2] = 0.0 ;
        pSharedData->prob[3] = 0.0 ;
        pSharedData->prob[4] = 0.0 ;
        pSharedData->prob[5] = 0.0 ;
        pSharedData->bUpdated = false ;
    }
    __except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        // Failed to write to the view.
        std::wcout << L"[ConnectionToUnity] Error: failed to write to shared memory.";
    }

    if (!FlushViewOfFile(m_mapView, 29 + 2000 * 4))
    {
        std::wcout << L"[ConnectionToUnity] Error: failed to flush shared memory.";
    }
    ReleaseMutex(m_hMutex) ;
    std::wcout << L"[ConnectionToUnity] Connection established." ;
    return (1) ;
}

int ConnectionToUnity::Release()
{
    if (NULL == m_hMutex)
    {
        std::wcout << L"[ConnectionToUnity]: Not connected!";
        return (-1) ;
    }
    UnmapViewOfFile(m_mapView); m_mapView = NULL;
    CloseHandle(m_hFileMap) ; m_hFileMap = NULL ;
    CloseHandle(m_hMutex); m_hMutex = NULL ;

    std::wcout << L"[ConnectionToUnity] Connection released." ;
    return (1) ;
}

int ConnectionToUnity::Send(int controllerCode)
{
    if (NULL == m_hMutex)
    {
        std::wcout << L"[ConnectionToUnity]: Not connected!";
        return (-1);
    }
    DWORD dwWaitResult = WaitForSingleObject(
        m_hMutex,     // handle to mutex
        INFINITE) ;  // no time-out interval
    __try
    {
        mapMemoryDataFormat* pSharedData = (mapMemoryDataFormat*)m_mapView ;
        pSharedData->numGestures = NumGestures ;
        pSharedData->prob[0] = 0.0 ;
        pSharedData->prob[1] = 0.0 ;
        pSharedData->prob[2] = 0.0 ;
        pSharedData->prob[3] = 0.0 ;
        pSharedData->prob[4] = 0.0 ;
        pSharedData->prob[5] = 0.0 ;
        pSharedData->prob[controllerCode] = 1.0 ;
        pSharedData->bUpdated = true ;
    }
    __except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        // Failed to write to the view.
        std::wcout << L"[ConnectionToUnity]: Error: failed to write to shared memory.";
    }

    if (!FlushViewOfFile(m_mapView, 29 + 2000 * 4))
    {
        std::wcout << L"[ConnectionToUnity]:Error: failed to flush shared memory." ;
        return (-1) ;
    }

    ReleaseMutex(m_hMutex) ;
    return (1) ;
}

int ConnectionToUnity::Send(float* probVector, int size)
{
    if (NULL == m_hMutex)
    {
        std::wcout << L"[ConnectionToUnity]: Not connected!";
        return (-1);
    }
    DWORD dwWaitResult = WaitForSingleObject(
        m_hMutex,     // handle to mutex
        INFINITE);  // no time-out interval
    __try
    {
        mapMemoryDataFormat* pSharedData = (mapMemoryDataFormat*)m_mapView;
        pSharedData->numGestures = NumGestures;
        pSharedData->prob[0] = probVector[0] ;
        pSharedData->prob[1] = probVector[1] ;
        pSharedData->prob[2] = probVector[2] ;
        pSharedData->prob[3] = 0.0;
        pSharedData->prob[4] = 0.0;
        pSharedData->prob[5] = 0.0;
        pSharedData->bUpdated = true;
    }
    __except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        // Failed to write to the view.
        std::wcout << L"[ConnectionToUnity]: Error: failed to write to shared memory.";
    }

    if (!FlushViewOfFile(m_mapView, 29 + 2000 * 4))
    {
        std::wcout << L"[ConnectionToUnity]:Error: failed to flush shared memory.";
        return (-1);
    }

    ReleaseMutex(m_hMutex);
    return (1);
}
