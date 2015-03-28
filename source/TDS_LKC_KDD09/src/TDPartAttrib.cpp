// TDPartAttrib.cpp: implementation of the CTDPartAttrib class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDPARTATTRIB_H)
    #include "TDPartAttrib.h"
#endif

//****************
// CTDPartAttrib *
//****************

CTDPartAttrib::CTDPartAttrib(CTDAttrib* pActualAttrib)
    : m_pActualAttrib(pActualAttrib), 
      m_pSupportMatrix(NULL), 
      m_pSensMatrix(NULL),
      m_bCandidate(true),
      m_relatedPos(NULL)
{
}

CTDPartAttrib::~CTDPartAttrib() 
{
    delete m_pSupportMatrix;
    delete m_pSensMatrix;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartAttrib::initSupportMatrix(CTDConcept* pCurrCon, int nClasses, const CTDConcepts* pSensConcepts)
{
    // Allocate the matrix
    int clsDims[] = {pCurrCon->getNumChildConcepts(), nClasses};
    m_pSupportMatrix = new CTDMDIntArray(sizeof(clsDims) / sizeof(int), clsDims);

    // Allocate support sums
    m_supportSums.SetSize(pCurrCon->getNumChildConcepts());

    // Allocate class sums
    m_classSums.SetSize(nClasses);

    // Initialize to zeros
    for (int i = 0; i < pCurrCon->getNumChildConcepts(); ++i) {
        for (int j = 0; j < nClasses; ++j) {
            (*m_pSupportMatrix)[i][j] = 0;
            m_classSums.SetAt(j, 0);
        }
        m_supportSums.SetAt(i, 0);
    }

    // Allocate the matrix for sensitive concepts
    int senDims[] = {pCurrCon->getNumChildConcepts(), pSensConcepts->GetSize()};
    m_pSensMatrix = new CTDMDIntArray(sizeof(senDims) / sizeof(int), senDims);


    // Initialize to zeros
    for (int i = 0; i < pCurrCon->getNumChildConcepts(); ++i)
        for (int j = 0; j < pSensConcepts->GetSize(); ++j)
            (*m_pSensMatrix)[i][j] = 0;

    return true;
}



//*****************
// CTDPartAttribs *
//*****************
CTDPartAttribs::CTDPartAttribs()
{
}

CTDPartAttribs::~CTDPartAttribs()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
