// TDController.cpp: implementation of the TDController class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDCONTROLLER_H)
    #include "TDController.h"
#endif

/*
#if !defined(BFFILEHELPER_H)
	#include "BFFileHelper.h"
#endif

char runningChars [] = {'-', '\\', '|', '/'};
CDCDebug* debugObject = new CDCDebug();
*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDController::CTDController(LPCTSTR rawDataFile, 
                             LPCTSTR attributesFile,
                             LPCTSTR nameFile, 
                             LPCTSTR supFile,
                             LPCTSTR transformedDataFile, 
                             LPCTSTR transformedTestFile, 
                             LPCTSTR transformedSVMDataFile, 
                             LPCTSTR transformedSVMTestFile,
                             bool  bIncludeNonVA, 
                             int   defaultL,
                             int   defaultK,
                             float defaultC,
                             int   nInputRecs,
                             int   nTraining)
    : m_attribMgr(attributesFile, nameFile, supFile, bIncludeNonVA), 
      m_virtualMgr(attributesFile, defaultL, defaultK, defaultC),
      m_dataMgr(rawDataFile, transformedDataFile, transformedTestFile, transformedSVMDataFile, transformedSVMTestFile, bIncludeNonVA, nInputRecs, nTraining)
{
    if (!m_virtualMgr.initialize(&m_attribMgr))
        ASSERT(false);
    if (!m_dataMgr.initialize(&m_attribMgr))
        ASSERT(false);
    if (!m_partitioner.initialize(&m_attribMgr, &m_virtualMgr, &m_dataMgr))
        ASSERT(false);
    if (!m_evalMgr.initialize(&m_attribMgr, &m_partitioner))
        ASSERT(false);
}

CTDController::~CTDController()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDController::runTopDown()
{	
    cout << _T("***************************************") << endl;
    cout << _T("* Privacy Data Transformer - Top Down *") << endl;
    cout << _T("***************************************") << endl;

    long startProgTime = getTime();
    if (!m_attribMgr.readAttributes())
        return false;

    if (!m_virtualMgr.readVIDandSensitive())
        return false;
        
    if (!m_virtualMgr.breakIntoCombinations())  // Implements the concept of "L"
        return false;
    
    if (!m_dataMgr.readRecords())
        return false;

#ifndef _TD_C45_SVM_DATA_CONVERTER
    long startTransformTime = getTime();
    if (!m_partitioner.transformData())
        return false;

    long startWritingTime = getTime();
    if (!m_dataMgr.writeRecords(false))
    //if (!m_dataMgr.writeRecords(false, 40700))
        return false;
#endif

    printTime();
    if (!m_attribMgr.writeNameFile())
        return false;

    //printTime();
    //if (!m_dataMgr.writeRecordsSVM())
    //    return false;   
#ifdef _TD_SCORE_FUNTION_TRANSACTION
    if (!m_attribMgr.writeSupFile())
        return false;
#endif
    long startEvalTime = getTime();
    int catDistortion = 0;
    float contDistortion = 0.0f;
    if (!m_evalMgr.countNumDistortions(catDistortion, contDistortion)) {
        ASSERT(false);
        return false;
    }
    cout << _T("Distortion for categorical: ") << catDistortion << endl;
    cout << _T("Distortion for continuous: ") << contDistortion << endl;


    Int64u discern = 0;
    if (!m_evalMgr.countNumDiscern(discern)) {
        ASSERT(false);
        return false;
    }
    cout << _T("Discernibility: ") << discern  << endl;
    cout << _T("Discernibility ratio: ") << long double(discern) / square(m_dataMgr.getRecords()->GetSize())  << endl;

    std::cout << _T("Spent: ") << startTransformTime - startProgTime << _T("s on reading records.") << std::endl;
    std::cout << _T("Spent: ") << startWritingTime - startTransformTime << _T("s on anonymization.") << std::endl;
    std::cout << _T("Spent: ") << startEvalTime - startWritingTime << _T("s on writing records.") << std::endl;
    std::cout << _T("Spent: ") << startEvalTime - startProgTime << _T("s in total (excluding evaluation).") << std::endl;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDController::removeUnknowns()
{	
    cout << _T("****************************************") << endl;
    cout << _T("* Removing Records With Unknown Values *") << endl;
    cout << _T("****************************************") << endl;
    
    if (!m_attribMgr.readAttributes())
        return false;

    if (!m_dataMgr.readRecords())
        return false;

    if (!m_dataMgr.writeRecords(true))
        return false;

    return true;
}