#pragma once
const int MAX_CONTROLLER_EVENTS = 16 ;
class Controller
{
public:
	Controller() = default ;
	virtual int Connect() { return false; }
	virtual int getType() { return -1; }
	virtual int readSingleCode() { return -1; }
	//virtual float* getProbVector() { return m_probVector; }
	//virtual int probVectorLength() { return 0; }
	virtual float* readProbVector() { return nullptr; }
protected:
	float m_probVector[MAX_CONTROLLER_EVENTS] ;
} ;

class KeyboardController : public Controller
{
public:
	KeyboardController() = default;
	int Connect() { return true; }
	virtual int getType() { return (0) ; }
	int readSingleCode() ;
} ;



