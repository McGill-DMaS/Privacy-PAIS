// TDRollbackRec.cpp: implementation of the CTDRollbackRec class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDROLLBACKREC_H)
    #include "TDRollbackRec.h"
#endif

//******************
// CTDRollbackRecs *
//******************

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDRollbackRecs::cleanup() {
    int nRecs = GetSize();
    for (int i = 0; i < nRecs; ++i)
        delete GetAt(i);
    RemoveAll();
}


//*****************
// CTDRollbackRec *
//*****************

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDRollbackRec::CTDRollbackRec(CTDVirtualTreeNode* pNode, int diff, const CTDIntArray& sensCounts) 
    : m_pVTreeNode(pNode), m_diff(diff) 
{
    m_sensCounts.Append(sensCounts);
}