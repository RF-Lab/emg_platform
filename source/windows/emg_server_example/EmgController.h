#pragma once
#include "Controller.h"
class EmgController : public Controller
{
public:
	EmgController() ;
	~EmgController() ;
	int Connect() ;
	virtual float* readProbVector();
protected:
	bool ConnectToBoard(const char* szAddress, USHORT Port) ;
	bool CreateSocket() ;
	void CloseSocket() ;
private:
	SOCKET  m_socket ;
	unsigned char* m_recvBuf ;
	int* m_circBuf ;
	unsigned int m_packetCounter;
	int m_head ;
	int m_maxValue ;
	int m_maxDelay ;
	float m_mean;
} ;

