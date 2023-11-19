#include <iostream>
#include "firFilter.h"

const int MAX_FILT_LEN = 1024 ;

firFilter::firFilter()
{
	m_circBuf = nullptr ;
	m_head = 0 ;
	m_impResp = nullptr ;
	m_size = 0 ;
}

bool firFilter::load(const char* szFileName)
{
    FILE* f = nullptr ;
    char* tmp_buf = new char[256] ;
    m_impResp = new double[MAX_FILT_LEN] ;
    m_size = 0 ;
    fopen_s( &f, szFileName, "rt") ;
    if (f)
    {
        while(!feof(f))
        {
            if (fgets(tmp_buf, 255, f))
            {
                if (sscanf_s(tmp_buf, "%lf", m_impResp+m_size) != 1)
                {
                    std::cout << "firFilter::load: Bad format " << szFileName << std::endl;
                    break ;
                }
                m_size++ ;
                if (m_size >= MAX_FILT_LEN)
                {
                    std::cout << "firFilter::load: too long filter " << szFileName << std::endl ;
                    break ;
                }
            }
        }
        fclose(f) ;
    }
    delete[] tmp_buf; tmp_buf = nullptr;
    m_circBuf = new double[m_size] ;
    for (int i = 0; i < m_size; i++)
    {
        m_circBuf[i] = 0.0;
    }
    m_head = 0 ;
    std::cout << "firFilter::load: " << m_size << " taps loaded from " << szFileName << std::endl ;

    return true ;
}

firFilter::~firFilter()
{
    if (nullptr != m_circBuf)
    {
        delete[] m_circBuf ;
    }
    if (nullptr != m_impResp)
    {
        delete[] m_impResp;
    }
}

float firFilter::operator()(double sample)
{
    m_circBuf[m_head] = sample ;
    m_head = (m_head + 1) % m_size;
    double out_sample = 0.0 ;
    for (int nTap=0;nTap<m_size;nTap++)
    {
        out_sample += m_impResp[m_size - nTap - 1] * m_circBuf[(m_head + nTap) % m_size] ;
    }
    //std::cout << out_sample << std::endl ;
    return ((float)out_sample) ;
}
