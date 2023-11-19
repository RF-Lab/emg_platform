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

    return (true) ;
}

char tmp_buf[256] ;
bool TensorflowModel::Predict(float* samples)
{

    // Define Input Tensor (Input of the network)
    //****** Get input tensor
    TF_Output t0 = { TF_GraphOperationByName(m_Graph, "serving_default_reshape_1_input"), 0 };
    if (t0.oper == NULL)
    {
        std::cout << "TensorflowModel::Load(): Failed to get input tensor from the loaded graph." << std::endl;
        return (false);
    }
    m_NumInputs = 1;
    m_Input = (TF_Output*) new byte[sizeof(TF_Output) * m_NumInputs]; //malloc(sizeof(TF_Output) * NumInputs);
    m_Input[0] = t0;

    // Define output tensor 
    //********* Get Output tensor
    TF_Output t2 = { TF_GraphOperationByName(m_Graph, "StatefulPartitionedCall"), 0 };
    if (t2.oper == NULL)
    {
        std::cout << "TensorflowModel::Load(): Failed to get output tensor from the loaded graph." << std::endl;
        return (false);
    }
    m_NumOutputs = 1;
    m_Output = (TF_Output*) new byte[sizeof(TF_Output) * m_NumOutputs]; //malloc(sizeof(TF_Output) * NumOutputs);
    m_Output[0] = t2;

    // Input values (samples)
    m_InputValues = (TF_Tensor**) new byte[sizeof(TF_Tensor*) * m_NumInputs];
    int ndims = 2;
    int64_t dims[] = { 1,numInputSamples };
    m_emgSamples = new float[numInputSamples];
    TF_Tensor* inp_tensor = TF_NewTensor(TF_FLOAT, dims, ndims, m_emgSamples, numInputSamples * sizeof(float), &NoOpDeallocator, 0);
    if (NULL == inp_tensor)
    {
        std::cout << "TensorflowModel::Load(): Failed to allocate input tensor." << std::endl;
        return (false);
    }
    m_InputValues[0] = inp_tensor;

    // Output tensor
    m_OutputValues = (TF_Tensor**) new byte[sizeof(TF_Tensor*) * m_NumOutputs];


    for (int i = 0; i < numInputSamples; i++)
    {
        m_emgSamples[i] = samples[i];
    }

    TF_SessionRun(m_Session, NULL, m_Input, m_InputValues, m_NumInputs, m_Output, m_OutputValues, m_NumOutputs, NULL, 0, NULL, m_Status) ;

    if (TF_GetCode(m_Status) == TF_OK)
    {
        std::cout << "TensorflowModel::Predict(): Session is OK." << std::endl ;
    }
    else
    {
        std::cout << TF_Message(m_Status) << std::endl ;
        return false ;
    }    

    TF_Tensor* out_tensor = m_OutputValues[0] ;
    float* pProbs = (float*) TF_TensorData(out_tensor) ;
    std::cout << "probs[0]: " << pProbs[0] << std::endl ;
    std::cout << "probs[1]: " << pProbs[1] << std::endl ;
    std::cout << "probs[2]: " << pProbs[2] << std::endl ;
    std::cout << "probs[3]: " << pProbs[3] << std::endl ;

    m_probVector[0] = pProbs[0] ;
    m_probVector[1] = pProbs[1] ;
    m_probVector[2] = pProbs[3] ;
    m_probVector[3] = pProbs[2] ;

    return true ;

}
