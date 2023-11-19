#pragma once
class firFilter
{
public:
	firFilter() ;
	bool load(const char* szFileName) ;
	~firFilter() ;
	float operator()(double sample) ;
private:
	double* m_circBuf ;
	int m_head ;
	double* m_impResp ;
	int m_size ;
} ;

