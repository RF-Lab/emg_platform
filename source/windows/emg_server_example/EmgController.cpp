#include <windows.h>
//#include <winsock2.h>
#include <iostream>
#include "EmgController.h"
#include "ClassificationModel.h"

const int DATA_BUF_SIZE = 528*2 ;
const int CIRC_BUF_SIZE = 2000 ;
const int SAMPLES_PER_BLOCK = 128;

const int PACKET_SIZE = 528; // bytes

const float det_th = 0.01 ;

void save_to_file(const char* szFileName, float* data, int size)
{
    FILE* f = nullptr;
    fopen_s(&f, szFileName, "wt") ;
    if (nullptr != f)
    {
        for (int n = 0; n < size; n++)
        {
            fprintf(f, "%16.15f\n", data[n]) ;
        }
        fclose(f) ;
    }

}

EmgController::EmgController()
{
	m_socket = INVALID_SOCKET ;
    m_recvBuf = new unsigned char[DATA_BUF_SIZE] ;
    m_circBuf = new float[CIRC_BUF_SIZE] ;
    m_flatBuf = new float[CIRC_BUF_SIZE] ;
    m_packetCounter = 0 ;
    m_head = 0;
    m_maxValue = 0 ;
    m_maxDelay = 0 ;
    m_mean = 0.0 ;
    m_filter = new firFilter() ;
    m_filter->load("filter.txt") ;
    m_sigScale = 0.0000228 ;
    m_model = new TensorflowModel();
}

EmgController::~EmgController()
{
    CloseSocket() ;
    delete[] m_recvBuf; m_recvBuf = nullptr ;
    delete[] m_circBuf; m_circBuf = nullptr ;
    delete[] m_flatBuf; m_flatBuf = nullptr ;
    delete m_filter; m_filter = nullptr ;
    delete m_model; m_model = nullptr;
}

bool EmgController::CreateSocket()
{
    if (m_socket == INVALID_SOCKET)
    {
        m_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) ;
        if (m_socket == INVALID_SOCKET)
        {
            std::wcout << L"Error WSAStartup" << std::endl ;
            return (false) ;
        }
    }
    return (true) ;
}

void EmgController::CloseSocket()
{
    if (m_socket != INVALID_SOCKET)
    {
        //shutdown(m_socket,SD_BOTH) ;
        closesocket(m_socket) ;
        m_socket = INVALID_SOCKET ;
    }
}

bool EmgController::ConnectToBoard(const char* szAddress, USHORT Port)
{
    sockaddr_in sin ;
    memset(&sin, 0, sizeof(sockaddr_in)) ;
    sin.sin_family = AF_INET ;
    sin.sin_port = htons(Port) ;
    sin.sin_addr.S_un.S_addr = inet_addr(szAddress) ;
    if (connect(m_socket, (sockaddr*)&sin, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
        CloseSocket() ;
        std::cout << "Can't establish connection to " << szAddress << std::endl ;
        return (false) ;
    }
    std::cout << "Connected to " << szAddress << std::endl ;
    return (true) ;
}

int EmgController::Connect()
{
    if (!CreateSocket())
    {
        return (false) ;
    }
    if (!ConnectToBoard("192.168.1.101", 3000))
    {
        return (false) ;
    }

	m_model->Load("C:\\userdata\\projects\\models\\model") ;

	return true ;
}

float* EmgController::readProbVector()
{
    while (1)
    {
        // fill exactly PACKET_SIZE bytes in m_recvBuf
        int receivedBytes = 0;
        while (receivedBytes < PACKET_SIZE)
        {
            int retCode = recv(m_socket, ((char*)m_recvBuf) + receivedBytes, PACKET_SIZE - receivedBytes, 0);
            if (retCode < 0)
            {
                return nullptr;
            }
            receivedBytes += retCode;
        }

        // Check if buffer started with EMG label (start of the packet)
        if (m_recvBuf[0] != 'E' || m_recvBuf[1] != 'M' || m_recvBuf[2] != 'G')
        {
            // Header not aligned - begin sync algorithm
            int offset = -1;
            for (int i = 0; i < PACKET_SIZE - 2; i++)
            {
                if (m_recvBuf[i] == 'E' && m_recvBuf[i + 1] == 'M' && m_recvBuf[i + 2] == 'G')
                {
                    offset = i;
                }
            }
            if (offset < 0)
            {
                std::cout << "Can't get synchronization (wrong proto parameters)" << std::endl ;
                return nullptr ;
            }
            // read left part of packet
            int retCode = recv(m_socket, ((char*)m_recvBuf), PACKET_SIZE - offset, 0) ;
        }
        else
        {
            // synch is ok
            break ;
        }
    }

    // check block counter
    unsigned int* pCount = (unsigned int*)(m_recvBuf + 8) ;
    //std::cout << "packet counter at " << (*pCount) << std::endl;
    if ((*pCount) != m_packetCounter)
    {
        std::cout << "Resync packet counter at " << (*pCount) << std::endl ;
        m_packetCounter = *pCount ;
    }

    int* pBlockSamples = (int*)(m_recvBuf + 16) ;
    for (int i = 0; i < SAMPLES_PER_BLOCK; i++)
    {
        m_circBuf[m_head] = (*m_filter)(pBlockSamples[i] * m_sigScale) ;

        m_mean = m_mean * 0.9 + 0.1 * m_circBuf[m_head] ;

        m_circBuf[m_head] -= m_mean ;


        //std::cout << m_circBuf[m_head] << std::endl;

        if (abs(m_circBuf[m_head]) > m_maxValue)
        {
            m_maxValue = abs(m_circBuf[m_head]) ;
            m_maxDelay = 0 ;
            //std::cout << "maxValue:" << m_maxValue << std::endl;
        }
        else
        {
            m_maxDelay++;
            //std::cout << "maxDelay:" << m_maxDelay << std::endl;

            if (m_maxDelay == 1000 && m_maxValue > det_th)
            {
                std::cout << "Action potential: max value: " << m_maxValue << std::endl ;

                m_maxValue = 0 ;
                m_maxDelay = 0 ;

                for (int u = 0; u < CIRC_BUF_SIZE; u++)
                {
                    m_flatBuf[u] = m_circBuf[(m_head + 1 + u) % CIRC_BUF_SIZE];
                }
                save_to_file("signal.txt", m_flatBuf, CIRC_BUF_SIZE) ;
                m_model->Predict(m_flatBuf) ;
            }
        }

        m_head = (m_head + 1) % CIRC_BUF_SIZE ;
    }

    m_packetCounter++ ;
    return nullptr ;
}

