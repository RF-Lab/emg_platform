#pragma once
#include "Controller.h"
#include "firFilter.h"
#include "ClassificationModel.h"
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
	float* m_circBuf ;
	unsigned int m_packetCounter;
	int m_head ;
	float m_maxValue ;
	int m_maxDelay ;
	float m_mean;
	float* m_meanBuf;
	firFilter* m_filter ;
	float m_sigScale ;
	float* m_flatBuf ;
	float* m_meanFlatBuf ;
	TensorflowModel* m_model ;

	float* m_rawBuf ;

} ;

