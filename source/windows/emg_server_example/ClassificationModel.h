#pragma once

#include "include\tensorflow\c\c_api.h"

const int MaxCNumlasses = 16 ;
class ClassificationModel
{
public:
	ClassificationModel() = default ;
	virtual bool Load(const char*) { return (false); }
	virtual bool Predict() { return (false); }
private:
	float predProb[MaxCNumlasses] ;
} ;

class TensorflowModel: public ClassificationModel
{
public:
	TensorflowModel() = default ;
	virtual bool Load(const char*) ;
	virtual bool Predict() ;

private:
	TF_Graph* m_Graph ;
	TF_Status* m_Status ;
	TF_SessionOptions* m_SessionOpts ;
	TF_Session* m_Session ;
	TF_Buffer* m_RunOpts ;

} ;

