#pragma once
class Controller
{
public:
	Controller() = default ;
	virtual int getType() { return -1; }
	virtual int readSingleCode() { return -1; }
} ;

class KeyboardController : public Controller
{
public:
	KeyboardController() = default;
	virtual int getType() { return (0) ; }
	int readSingleCode() ;
} ;



