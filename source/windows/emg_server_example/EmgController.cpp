#include <windows.h>
#include <iostream>
#include "EmgController.h"
#include "ClassificationModel.h"

int EmgController::Connect()
{
	TensorflowModel model ;
	model.Load(NULL) ;
	return 0 ;
}
