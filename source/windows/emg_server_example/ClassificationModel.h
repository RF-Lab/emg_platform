#pragma once

#include "include\tensorflow\c\c_api.h"

const int MaxCNumlasses = 16 ;
class ClassificationModel
{
public:
	ClassificationModel() = default ;
	virtual bool Load(const char*) { return (false); }
	virtual bool Predict(float*) { return (false); }
public:
	float m_probVector[MaxCNumlasses] ;
} ;

const int numInputSamples = 2000 ;
class TensorflowModel: public ClassificationModel
{
public:
	TensorflowModel() = default ;
	virtual bool Load(const char*) ;
	virtual bool Predict(float*) ;

private:
	TF_Graph* m_Graph ;
	TF_Status* m_Status ;
	TF_SessionOptions* m_SessionOpts ;
	TF_Session* m_Session ;
	TF_Buffer* m_RunOpts ;
	TF_Output* m_Input ; // Input of the network
	TF_Output* m_Output ; // Output of the network
	TF_Tensor** m_InputValues ;
	float* m_emgSamples ;
	TF_Tensor** m_OutputValues ;
	int m_NumInputs;
	int m_NumOutputs;
} ;

