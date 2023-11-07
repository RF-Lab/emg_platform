#pragma once
const int MaxCNumlasses = 16 ;
class ClassificationModel
{
public:
	ClassificationModel() = default ;
	virtual bool Load(void*) { return (false); }
	virtual bool Predict() { return (false); }
private:
	float predProb[MaxCNumlasses] ;
} ;

class TensorflowModel: public ClassificationModel
{
public:
	TensorflowModel() = default ;
	virtual bool Load(void*) ;
	virtual bool Predict() ;
private:
	float predProb[MaxCNumlasses];
};

