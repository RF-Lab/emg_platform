#pragma once
class firFilter
{
public:
	firFilter() ;
	bool load(const char* szFileName) ;
	~firFilter() ;
	float operator()(float sample) ;
private:
	float* m_circBuf ;
	int m_head ;
	float* m_impResp ;
	int m_size ;
} ;

