#include <windows.h>
#include <iostream>
#include "EmgController.h"
#include "ClassificationModel.h"

int EmgController::Connect()
{
	TensorflowModel model ;
	model.Load("C:\\userdata\\projects\\models\\model") ;
	model.Predict() ;
	return 0 ;
}
