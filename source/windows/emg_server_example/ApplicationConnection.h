#pragma once
// Connection to application to deliver controller codes to
class ApplicationConnection
{
public:
	ApplicationConnection() = default ;
	virtual int Connect() { return (-1) ; }
	virtual int Release() { return (-1); }
	virtual int Send(int controllerCode) { return (-1); }
	virtual int Send(float* probVector, int size) { return (-1); }
	~ApplicationConnection() { Release();  }
} ;

class ConnectionToUnity: public ApplicationConnection
{
public:
	ConnectionToUnity() ;
	virtual int Connect() ;
	virtual int Release() ;
	virtual int Send(int controllerCode) ;
	virtual int Send(float* probVector, int size) ;
private:
	HANDLE m_hMutex = NULL ;
	HANDLE m_hFileMap = NULL ;
	LPVOID m_mapView = NULL ;
};
