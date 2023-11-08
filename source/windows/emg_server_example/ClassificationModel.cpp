#include <windows.h>
#include <iostream>
#include "ClassificationModel.h"

bool TensorflowModel::Load(void*)
{
    std::cout << "TF version: " << TF_Version() << std::endl ;

    m_Graph = TF_NewGraph() ;
    m_Status = TF_NewStatus() ;

    m_SessionOpts = TF_NewSessionOptions() ;
    m_RunOpts = NULL ;

    const char* saved_model_dir = "C:\\userdata\\Tmp\\content\\model"; 
    const char* tags = "serve"; // default model serving tag; can change in future
    int ntags = 1 ;

    TF_Session* Session = TF_LoadSessionFromSavedModel(
        m_SessionOpts, 
        m_RunOpts, 
        saved_model_dir, 
        &tags, 
        ntags, 
        m_Graph, 
        NULL, m_Status
    ) ;
    if (TF_GetCode(m_Status) == TF_OK)
    {
        std::cout << "TF_LoadSessionFromSavedModel OK" ;
    }
    else
    {
        std::cout << TF_Message(m_Status) ;
        return false ;
    }
    return true ;
}

bool TensorflowModel::Predict()
{
    return false ;
}
