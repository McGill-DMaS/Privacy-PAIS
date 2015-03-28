// TDVirtualID.cpp: implementation of the CTDVirtualID class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDVIRTUALID_H)
    #include "TDVirtualID.h"
#endif

//***************
// CTDVirtualID *
//***************

CTDVirtualID::CTDVirtualID(int defaultK)
    : m_pMinACNode(NULL), m_pMaxCFNode(NULL), m_kThreshold(defaultK), m_vTreeNodeRoot(NULL, -1)
{
}

CTDVirtualID::~CTDVirtualID() 
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDVirtualID::initVID(LPCTSTR vidStr, CTDAttribs* pAttribs)
{
    m_virtualAttribs.RemoveAll();
    ASSERT(pAttribs);

    // Take away the first and last characters.
    CString restStr = vidStr;
    CBFStrHelper::trim(restStr);
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] != TD_CONHCHY_OPENTAG || 
        restStr[len - 1] != TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }
    restStr = restStr.Mid(1, len - 2);
    CBFStrHelper::trim(restStr);
    
    // Retrieve the k value for this VID.
    CString kStr;
    if (!parseVIDAttrib(kStr, restStr)) {
        cerr << _T("CTDVirtualID: Failed to extract k from ") << restStr << endl;
        return false;
    }
    if (m_kThreshold <= 0) {
        m_kThreshold = StrToInt((LPCTSTR) kStr);
        if (m_kThreshold <= 0) {
            cerr << _T("CTDVirtualID: Failed to extract k from ") << restStr << endl;
            ASSERT(false);
            return false;
        }
    }

    // Extract each Virtual Attribute.
    CString firstVIDAttrib;    
    CTDAttrib* pAttrib = NULL;
    int nAttribs = pAttribs->GetSize();
    bool bFound = false;
    while (!restStr.IsEmpty()) {
        if (!parseVIDAttrib(firstVIDAttrib, restStr)) {
            cerr << _T("CTDVirtualID: Failed to build VID from ") << restStr << endl;
            return false;
        }

        // Find the attribute.
        bFound = false;
        for (int i = 0; i < nAttribs; ++i) {
            pAttrib = pAttribs->GetAt(i);
            if (pAttrib->m_attribName.CompareNoCase(firstVIDAttrib) == 0) {
                // Found! 
                // Tag this attribute as Virtual Attribute.
                // Put this attribute into this VID.
                bFound = true;
                pAttrib->m_bVirtualAttrib = true;
                m_virtualAttribs.Add(pAttrib);
                break;
            }
        }

        if (!bFound) {
            cerr << _T("CTDVirtualID: Failed to find VID ") << firstVIDAttrib << endl;
            ASSERT(false);
            return false;
        }
    }

    if (m_virtualAttribs.GetSize() == 0) {
        ASSERT(false);
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------
// If path cannot be found, return 0.
//---------------------------------------------------------------------------
int CTDVirtualID::getVPathCount(const CTDIntArray& vPath) 
{ 
    ASSERT(m_virtualAttribs.GetSize() == vPath.GetSize());
    CTDVirtualTreeNode* pNode = m_vTreeNodeRoot.getVPathNode(vPath);
    if (!pNode) {
        // Node cannot be found.
        return 0;
    }
    return pNode->getCount();
}

//---------------------------------------------------------------------------
// If path does not exist, create it.
//---------------------------------------------------------------------------
bool CTDVirtualID::updateVPathCount(const CTDIntArray& vPath, int count, const CTDIntArray& sensCounts, bool bRollback) 
{ 
    ASSERT(m_virtualAttribs.GetSize() == vPath.GetSize());
    if (bRollback)
        return m_vTreeNodeRoot.updateVPathCount(vPath, count, sensCounts, &m_rollbackRecs);
    else
        return m_vTreeNodeRoot.updateVPathCount(vPath, count, sensCounts, NULL);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDVirtualID::updateMinAC_MaxCFNode()
{
    CTDVirtualTreeNode* pMinNode = NULL;
    CTDVirtualTreeNode* pMaxNode = NULL;
    if (!m_vTreeNodeRoot.searchMinAC_MaxCFNode(pMinNode, pMaxNode))
        return false;

    ASSERT(pMinNode && pMaxNode);
    m_pMinACNode = pMinNode;
    m_pMaxCFNode = pMaxNode;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDVirtualID::searchMinAC_MaxCF(int& minAC, float& maxCF)
{
    minAC = 0;
    maxCF = 0.0f;
    CTDVirtualTreeNode* pMinNode = NULL;
    CTDVirtualTreeNode* pMaxNode = NULL;
    if (!m_vTreeNodeRoot.searchMinAC_MaxCFNode(pMinNode, pMaxNode))
        return false;

    ASSERT(pMinNode && pMaxNode);
    minAC = pMinNode->getCount();
    maxCF = pMaxNode->getMaxConf();
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDVirtualID::rollback()
{
    CTDRollbackRec* pRec = NULL;
    int nRecs = m_rollbackRecs.GetSize();
    for (int i = 0; i < nRecs; ++i) {
        pRec = m_rollbackRecs.GetAt(i);
        pRec->m_pVTreeNode->updateCount(pRec->m_diff * -1);

        // rollback the sensitive counts
        CTDIntArray negSensCounts;
        negateNumbers(pRec->m_sensCounts, negSensCounts);
        pRec->m_pVTreeNode->updateSensCounts(negSensCounts);
    }
    m_rollbackRecs.cleanup();
    return true;
}

//---------------------------------------------------------------------------
// Given elems, find all combinations(elems.size(), req_len).
//---------------------------------------------------------------------------
bool CTDVirtualID::combinations(int req_len, CTDVirtualIDs& vids)
{
    vids.cleanup();
    CTDIntArray pos;
    for (int i = 0; i < req_len; ++i)
        pos.Add(0);
    return combinationsHelper(req_len, pos, 0, 0, vids);
}

//---------------------------------------------------------------------------
//The first function argument, elems, is a vector containing N elements. As you see,
//I chose characters as elements, just like in the examples above. The second argument,
//req_len, is the requested combination size or length (that is, M). pos is a vector
//of size req_len that will hold the positions or indexes of the chosen elements. The
//depth argument indicates how many elements we’ve chosen so far, that is, the recursion
//depth. Finally, the margin argument indicates the first position to start looking for a 
//new element (the left “margin”). Remember we have to choose elements to the right of 
//the last chosen one.
//
//Simple as that, the first block checks if we’ve chosen enough elements and prints
//the elements in the chosen positions. If we wanted to store the combination 
//instead of printing it, we could use one output argument, for example.
//
//The second block checks if there are enough remaining elements to the right. 
//In the example we put, when we chose c as the first element, there were no elements 
//to its right to be chosen as the second one. This is a generalization of that test 
//to stop as soon as possible. As the comments say, the function works if you remove 
//it. However, it can be slower. This check can also be integrated into the loop 
//condition in the third block, if you want.
//
//The third and final block chooses one more element starting from the margin, 
//iterating until the end of the vector, and recursively calls itself, indicating one 
//more depth level, and setting the margin to one position to the right of the current 
//chosen position.
//
//The initial call must use depth 0 and margin 0.
//---------------------------------------------------------------------------
bool CTDVirtualID::combinationsHelper(int req_len, CTDIntArray& pos, int depth, int margin, CTDVirtualIDs& vids)
{
    // Have we selected the requested number of elements?
    if (depth >= req_len) {
        CTDVirtualID* pNewVID = new CTDVirtualID(m_kThreshold);
        for (int ii = 0; ii < pos.GetSize(); ++ii)
            pNewVID->m_virtualAttribs.Add(m_virtualAttribs[pos[ii]]);
        vids.Add(pNewVID);
        return true;
    }

    // Are there enough remaining elements to be selected?
    // This test isn't required for the function to be
    // correct, but it saves futile calls.
    if (m_virtualAttribs.GetSize() - margin < req_len - depth)
        return true;

    // Try to select new elements to the right of the last
    // selected one.
    for (int ii = margin; ii < m_virtualAttribs.GetSize(); ++ii) {
        pos[depth] = ii;
        if (!combinationsHelper(req_len, pos, depth + 1, ii + 1, vids))
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDVirtualID::toString() const
{
    CString str = TD_VID_OPENTAG;
    CTDAttrib* pVAttrib = NULL;
    for (int a = 0; a < m_virtualAttribs.GetSize(); ++a) {
        pVAttrib = m_virtualAttribs.GetAt(a);
        str += pVAttrib->m_attribName;
 
        if (a + 1 < m_virtualAttribs.GetSize())
            str += TD_VID_DELIMETER;
    }
    str += TD_VID_CLOSETAG;
    return str;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDVirtualID::parseVIDAttrib(CString& firstAttrib, CString& restStr)
{
    firstAttrib.Empty();
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] != TD_CONHCHY_OPENTAG || 
        restStr[len - 1] != TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }

    int closeTagPos = restStr.Find(TD_CONHCHY_CLOSETAG);
    if (closeTagPos < 0) {
        ASSERT(false);
        return false;
    }

    // Extract the first element.
    firstAttrib = restStr.Mid(1, closeTagPos - 1);
    CBFStrHelper::trim(firstAttrib);
    if (firstAttrib.IsEmpty()) {
        ASSERT(false);
        return false;
    }

    // Update the rest of the string.
    restStr = restStr.Mid(closeTagPos + 1);
    CBFStrHelper::trim(restStr);
    return true;
}


//****************
// CTDVirtualIDs *
//****************
CTDVirtualIDs::CTDVirtualIDs()
{
}

CTDVirtualIDs::~CTDVirtualIDs()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDVirtualIDs::cleanup()
{
    int nVIDs = GetSize();
    for (int i = 0; i < nVIDs; ++i)
        delete GetAt(i);
    RemoveAll();
}

//---------------------------------------------------------------------------
// {{{attrib A} {attrib B}} {{attrib C} {attrib D} {attrib E}}}
//---------------------------------------------------------------------------
bool CTDVirtualIDs::initVIDs(LPCTSTR vidStr, CTDAttribs* pAttribs, int defaultK)
{
    ASSERT(pAttribs);

    // Take away the first and last characters.
    CString restStr = vidStr;
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] != TD_CONHCHY_OPENTAG || 
        restStr[len - 1] != TD_CONHCHY_CLOSETAG) {
        cerr << _T("CTDVirtualIDs: Invalid VID: ") << restStr << endl;
        ASSERT(false);
        return false;
    }
    restStr = restStr.Mid(1, len - 2);
    CBFStrHelper::trim(restStr);

    CString firstVID;
    while (!restStr.IsEmpty()) {
        // Retrieve the first VID
        if (!parseFirstVID(firstVID, restStr))
            return false;
        if (firstVID.IsEmpty()) {
            ASSERT(false);
            continue;
        }

        // Create a VID.
        CTDVirtualID* pNewVID = new CTDVirtualID(defaultK);
        if (!pNewVID) {
            ASSERT(false);
            return false;
        }

        if (!pNewVID->initVID(firstVID, pAttribs)) {
            cerr << _T("CTDVirtualIDs: Failed to init VID for ") << firstVID << endl;
            return false;
        }
        Add(pNewVID);
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDVirtualIDs::toString() const
{
    CString str;
    for (int i = 0; i < GetSize(); ++i)
        str += GetAt(i)->toString() + _T("\n");
    return str;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDVirtualIDs::parseFirstVID(CString& firstVID, CString& restStr)
{
    firstVID.Empty();
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] != TD_CONHCHY_OPENTAG || 
        restStr[len - 1] != TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }

    // Find the index number of the closing tag of the first concept.
    int tagCount = 0;
    for (int i = 0; i < len; ++i) {
        if (restStr[i] == TD_CONHCHY_OPENTAG)
            ++tagCount;
        else if (restStr[i] == TD_CONHCHY_CLOSETAG) {
            --tagCount;
            ASSERT(tagCount >= 0);
            if (tagCount == 0) {
                // Closing tag of first concept found!
                firstVID = restStr.Left(i + 1);
                restStr = restStr.Mid(i + 1);
                CBFStrHelper::trim(restStr);
                return true;
            }
        }
    }
    ASSERT(false);
    return false;
}