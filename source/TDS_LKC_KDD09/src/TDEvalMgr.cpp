// TDAttribMgr.cpp: implementation of the CTDAttribMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDEVALMGR_H)
    #include "TDEvalMgr.h"
#endif

CTDEvalMgr::CTDEvalMgr() 
{
}

CTDEvalMgr::~CTDEvalMgr() 
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDEvalMgr::initialize(CTDAttribMgr* pAttribMgr, CTDPartitioner* pPartitioner)
{
    if (!pAttribMgr || !pPartitioner) {
        ASSERT(false);
        return false;
    }
    m_pAttribMgr = pAttribMgr;
    m_pPartitioner = pPartitioner;
    return true;
}

//---------------------------------------------------------------------------
// Count the number of distortions.
// Each time a record is generalized from a child value to a parent value, 
// we charge 1 unit of distortion. So if 100 records are involved in the 
// generalization, we charge 100 unit. 
//---------------------------------------------------------------------------
bool CTDEvalMgr::countNumDistortions(int& catDistortion, float& contDistortion)
{
    cout << _T("Counting number of distortions...") << endl;
    catDistortion = 0;
    contDistortion = 0.0f;
    int nRecs = 0, nValues = 0;    
    CTDRecord* pRec = NULL;
    CTDAttrib* pAttrib = NULL;
    CTDPartition* pPartition = NULL;
    CTDValue* pValue = NULL;
    CTDConcept* pCurrentConcept = NULL;
    CTDConcept* pRawConcept = NULL;
    CTDPartitions* pLeafPartitions = m_pPartitioner->getLeafPartitions();

    // For each partition.
    for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
        pPartition = pLeafPartitions->GetNext(leafPos);
        nRecs = pPartition->getNumRecords();

        // For each record.
        for (int r = 0; r < nRecs; ++r) {
            pRec = pPartition->getRecord(r);
            nValues = pRec->getNumValues();

            // For each value.
            for (int v = 0; v < nValues; ++v) {
                pAttrib = m_pAttribMgr->getAttribute(v);
                if (!pAttrib->m_bVirtualAttrib)
                    continue;

                pValue = pRec->getValue(v);
                pCurrentConcept = pValue->getCurrentConcept();
                if (pAttrib->isContinuous()) {
                    CTDContConcept* pContConcept = (CTDContConcept*) pCurrentConcept;
                    CTDContConcept* pRoot = (CTDContConcept*) pAttrib->getConceptRoot();
                    contDistortion += (pContConcept->m_upperBound - pContConcept->m_lowerBound) / (pRoot->m_upperBound - pRoot->m_lowerBound);
                }
                else {
                    pRawConcept = ((CTDStringValue*) pValue)->getRawConcept();
#if defined(_TD_SCORE_FUNTION_TRANSACTION)
                    // In case of transaction data, count a distortion only if suppressing "1".
                    if (pRawConcept->m_conceptValue.CompareNoCase(TD_TRANSACTION_ITEM_PRESENT) != 0)
                        continue;
#endif
                    if (pRawConcept->m_depth < 0 || pCurrentConcept->m_depth < 0) {
                        cout << _T("CSAEvalMgr::countNumDistortions: Negative depth.") << endl;
                        ASSERT(false);
                        return false;
                    }
                    catDistortion += pRawConcept->m_depth - pCurrentConcept->m_depth;
                }
            }
        }
    }
    cout << _T("Counting number of distortions succeeded.") << endl;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDEvalMgr::countNumDiscern(Int64u& discern)
{
    cout << _T("Counting discernibility...") << endl;
    discern = 0;
    int nRecs = 0;    
    CTDPartition* pPartition = NULL;
    CTDPartitions* pLeafPartitions = m_pPartitioner->getLeafPartitions();

    // For each partition.
    for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
        pPartition = pLeafPartitions->GetNext(leafPos);
        nRecs = pPartition->getNumRecords();
        discern += square(nRecs);
    }
    cout << _T("Counting discernibility succeeded.") << endl;
    return true;
}
