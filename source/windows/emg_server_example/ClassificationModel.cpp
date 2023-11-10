#include <windows.h>
#include <iostream>
#include "ClassificationModel.h"

extern "C" void NoOpDeallocator(void* data, size_t a, void* b) { ; }

bool TensorflowModel::Load(const char* szModelName)
{
    std::cout << "TF version: " << TF_Version() << std::endl ;

    m_Graph = TF_NewGraph() ;
    m_Status = TF_NewStatus() ;

    m_SessionOpts = TF_NewSessionOptions() ;
    m_RunOpts = NULL ;

    const char* tags = "serve"; // default model serving tag; can change in future
    int ntags = 1 ;

    m_Session = TF_LoadSessionFromSavedModel(
        m_SessionOpts, 
        m_RunOpts, 
        szModelName,
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

char tmp_buf[256] ;
bool TensorflowModel::Predict()
{
    //****** Get input tensor
    TF_Output t0 = { TF_GraphOperationByName(m_Graph, "serving_default_reshape_1_input"), 0 } ;
    if (t0.oper == NULL)
    {
        std::cout << "TensorflowModel::Predict(): Failed to get input tensor from the loaded graph." << std::endl ;
        return (false) ;
    }
    int NumInputs = 1 ;
    TF_Output* Input = (TF_Output*) new byte[sizeof(TF_Output) * NumInputs]; //malloc(sizeof(TF_Output) * NumInputs);
    Input[0] = t0 ;

    //********* Get Output tensor
    TF_Output t2 = { TF_GraphOperationByName(m_Graph, "StatefulPartitionedCall"), 0 } ;
    if (t2.oper == NULL)
    {
        std::cout << "TensorflowModel::Predict(): Failed to get output tensor from the loaded graph." << std::endl ;
        return (false) ;
    }
    int NumOutputs = 1 ;
    TF_Output* Output = (TF_Output*) new byte[sizeof(TF_Output) * NumOutputs] ; //malloc(sizeof(TF_Output) * NumOutputs);
    Output[0] = t2 ;

    TF_Tensor** InputValues = (TF_Tensor**) new byte[sizeof(TF_Tensor*) * NumInputs] ;
    TF_Tensor** OutputValues = (TF_Tensor**) new byte[sizeof(TF_Tensor*) * NumOutputs] ;

    int ndims = 2 ;
    int64_t dims[] = { 1,2000 } ;
    float data[2000] ;
    FILE* f = NULL;
    fopen_s(&f, ".\\data\\p_0_0.txt", "rt") ;
    if (f)
    {
        for (int n = 0; n < 2000; n++)
        {
            fgets(tmp_buf, 255, f) ;
            sscanf_s(tmp_buf, "%f", data + n) ;
        }
        fclose(f) ;
    }
    int ndata = sizeof(data) ;

    TF_Tensor* inp_tensor = TF_NewTensor(TF_FLOAT, dims, ndims, data, ndata, &NoOpDeallocator, 0) ;
    if (NULL == inp_tensor)
    {
        std::cout << "TensorflowModel::Predict(): Failed to allocate input tensor." << std::endl ;
        return (false) ;
    }

    InputValues[0] = inp_tensor ;
    TF_SessionRun(m_Session, NULL, Input, InputValues, NumInputs, Output, OutputValues, NumOutputs, NULL, 0, NULL, m_Status) ;

    if (TF_GetCode(m_Status) == TF_OK)
    {
        std::cout << "TensorflowModel::Predict(): Session is OK." << std::endl ;
    }
    else
    {
        std::cout << TF_Message(m_Status) << std::endl ;
        return false ;
    }    

    TF_Tensor* out_tensor = OutputValues[0] ;
    float* pProbs = (float*) TF_TensorData(out_tensor) ;
    std::cout << "probs[0]: " << pProbs[0] << std::endl ;
    std::cout << "probs[1]: " << pProbs[1] << std::endl ;
    std::cout << "probs[2]: " << pProbs[2] << std::endl ;
    std::cout << "probs[3]: " << pProbs[3] << std::endl ;
    
    return true ;

}
