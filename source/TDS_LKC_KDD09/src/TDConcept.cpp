// TDConcept.cpp: implementation of the CTDConcept class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDCONCEPT_H)
    #include "TDConcept.h"
#endif

#if !defined(TDATTRIBUTE_H)
    #include "TDAttribute.h"
#endif

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

//**************
// CTDConcepts *
//**************
CTDConcepts::CTDConcepts()    
{
}

CTDConcepts::~CTDConcepts()
{
}

void CTDConcepts::cleanup()
{
    for (int i = 0; i < GetSize(); ++i)
        delete GetAt(i);

    RemoveAll();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDConcept* CTDConcepts::findConcept(LPCTSTR target)
{
    CTDConcept* pConcept = NULL;
    for (int i = 0; i < GetSize(); ++i) {
        pConcept = GetAt(i);
        if (pConcept->m_conceptValue.CompareNoCase(target) == 0)
            return pConcept;
    }
    return NULL;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDConcepts::initSensConcepts(LPCTSTR senStr, CTDAttribs* pAttribs, float defaultC)
{
    ASSERT(pAttribs);

    // Take away the first and last characters.
    CString restStr = senStr;
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] != TD_CONHCHY_OPENTAG || 
        restStr[len - 1] != TD_CONHCHY_CLOSETAG) {
        cerr << _T("CTDConcepts: Invalid sensitive concepts: ") << restStr << endl;
        ASSERT(false);
        return false;
    }
    restStr = restStr.Mid(1, len - 2);
    CBFStrHelper::trim(restStr);

    float firstCThreshold = 0.0f;
    CString firstSensAttrib, firstSensConcept;
    while (!restStr.IsEmpty()) {
        // Retrieve the first sensitive concept
        if (!parseFirstSensitive(firstCThreshold, firstSensAttrib, firstSensConcept, restStr))
            return false;
        if (firstCThreshold == 0.0f || firstSensAttrib.IsEmpty() || firstSensConcept.IsEmpty()) {
            cerr << _T("CTDConcepts: Invalid sensitive concepts: ") << restStr << endl;
            ASSERT(false);
            return false;
        }
        
        // Identify the sensitive attribute
        CTDAttrib* pSensAttrib = pAttribs->findAttribute(firstSensAttrib);
        if (!pSensAttrib) {
            cerr << _T("CTDConcepts: Invalid sensitive attribute: ") << firstSensAttrib << endl;
            ASSERT(false);
            return false;
        }
        if (pSensAttrib->m_bVirtualAttrib) {
            cerr << _T("CTDConcepts: An attribute cannot be both virtual and sensitive: ") << firstSensAttrib << endl;
            ASSERT(false);
            return false;
        }
        pSensAttrib->m_bSensAttrib = true;

        // Identify the sensitive concept
        CTDConcept* pSensConcept = pSensAttrib->getFlattenConcepts()->findConcept(firstSensConcept);
        if (!pSensConcept) {
            cerr << _T("CTDConcepts: Invalid sensitive concept: ") << firstSensConcept << endl;
            ASSERT(false);
            return false;
        }
        pSensConcept->m_bSensConcept = true;
        if (defaultC <= 0.0f)
            pSensConcept->m_cThreshold = firstCThreshold;   // However, this threshold is ignored.
        else
            pSensConcept->m_cThreshold = defaultC;

        Add(pSensConcept);
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDConcepts::parseFirstSensitive(float& cThreshold, CString& firstSAttrib, CString& firstSConcept, CString& restStr)
{
    cThreshold = 0.0f;
    firstSAttrib.Empty();
    firstSConcept.Empty();
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
                
                CString firstSens = restStr.Left(i + 1);
                if (!parseThresholdAttribConcept(firstSens, cThreshold, firstSAttrib, firstSConcept)) {
                    cerr << _T("CTDConcepts: Invalid sensitive concept: ") << firstSens << endl;
                    ASSERT(false);
                    return false;
                }

                restStr = restStr.Mid(i + 1);
                CBFStrHelper::trim(restStr);
                return true;
            }
        }
    }
    ASSERT(false);
    return false;
}

//---------------------------------------------------------------------------
// Break: 0.5 workclass:Never-worked into three pieces.
//---------------------------------------------------------------------------
// static
bool CTDConcepts::parseThresholdAttribConcept(CString firstSens, float& cThreshold, CString& firstSAttrib, CString& firstSConcept)
{
    int len = firstSens.GetLength();
    if (len < 2 ||
        firstSens[0] != TD_CONHCHY_OPENTAG || 
        firstSens[len - 1] != TD_CONHCHY_CLOSETAG) {
        cerr << _T("CTDConcepts: Invalid sensitive concept: ") << firstSens << endl;
        ASSERT(false);
        return false;
    }
    firstSens = firstSens.Mid(1, len - 2).Trim();

    int spacePos = firstSens.Find(" ");
    if (spacePos == -1) {
        cerr << _T("CTDConcepts: Invalid sensitive concept: ") << firstSens << endl;
        ASSERT(false);
        return false;
    }

    cThreshold = (float) CBFStrHelper::strToFloat((LPCTSTR) firstSens.Left(spacePos));
    CString attribConceptStr = firstSens.Mid(spacePos + 1).Trim();
    
    int sepPos = attribConceptStr.Find(TD_CONHCHY_SENSEPARATOR);
    if (sepPos == -1) {
        cerr << _T("CTDConcepts: Invalid sensitive concept: ") << firstSens << endl;
        ASSERT(false);
        return false;
    }

    firstSAttrib = attribConceptStr.Left(sepPos).Trim();
    firstSConcept = attribConceptStr.Mid(sepPos + 1).Trim();
    return true;
}

//*************
// CTDConcept *
//*************

CTDConcept::CTDConcept(CTDAttrib* pAttrib)
    : m_pParentConcept(NULL), 
      m_pAttrib(pAttrib),
      m_childIdx(-1), 
      m_flattenIdx(-1), 
      m_depth(-1),
      m_infoGain(0.0f), 
      m_infoAnoyRatio(0.0f), 
      m_cThreshold(0.0f),
      m_bSensConcept(false),
      m_bCutCandidate(true),
      m_cutPos(NULL),
      m_pSplitSupMatrix(NULL)
{
    // use dynamic alocation in order to use forward declaration.
    m_pRelatedPartitions = new CTDPartitions();
}

CTDConcept::~CTDConcept() 
{
    delete m_pSplitSupMatrix;
    m_pSplitSupMatrix = NULL;

    delete m_pRelatedPartitions;
    m_pRelatedPartitions = NULL;
    m_childConcepts.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDConcept::addChildConcept(CTDConcept* pConceptNode)
{
    try {
        pConceptNode->m_pParentConcept = this;
        pConceptNode->m_childIdx = m_childConcepts.Add(pConceptNode);
        return true;
    }
    catch (CMemoryException&) {
        ASSERT(false);
        return false;
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int CTDConcept::getNumChildConcepts() const
{ 
    return m_childConcepts.GetSize(); 
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDConcept* CTDConcept::getChildConcept(int idx) const
{
    return m_childConcepts.GetAt(idx); 
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDConcept* CTDConcept::getParentConcept() 
{ 
    return m_pParentConcept; 
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDPartitions* CTDConcept::getRelatedPartitions() 
{ 
    return m_pRelatedPartitions; 
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
POSITION CTDConcept::registerPartition(CTDPartition* pPartition) 
{ 
    return m_pRelatedPartitions->AddTail(pPartition);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDConcept::deregisterPartition(POSITION pos) 
{
    m_pRelatedPartitions->RemoveAt(pos);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDConcept::initSplitMatrix(int nConcepts, int nClasses)
{
    // Allocate the matrix
    int dims[] = {nConcepts, nClasses};
    m_pSplitSupMatrix = new CTDMDIntArray(sizeof(dims) / sizeof(int), dims);
    if (!m_pSplitSupMatrix) {
        ASSERT(false);
        return false;
    }

    // Allocate support sums
    m_splitSupSums.SetSize(nConcepts);

    // Allocate class sums
    m_splitClassSums.SetSize(nClasses);

    for (int i = 0; i < nConcepts; ++i) {
        for (int j = 0; j < nClasses; ++j) {
            (*m_pSplitSupMatrix)[i][j] = 0;
            m_splitClassSums.SetAt(j, 0);
        }
        m_splitSupSums.SetAt(i, 0);
    }
    return true;
}

#if defined(_TD_SCORE_FUNTION_DISTORTION) || defined(_TD_SCORE_FUNCTION_DISTORTION_BY_ANONYLOSS)
//---------------------------------------------------------------------------
// Compute information gain from all linked partitions.
//---------------------------------------------------------------------------
bool CTDConcept::computeInfoGain(int nClasses)
{
    // No need to compute if already computed.
    if (m_infoGain != 0.0f || !m_bCutCandidate)
        return true;

    // For each linked partition.    
    CTDPartition* pLinkedPart = NULL;
    CTDPartitions* pLinkedParts = getRelatedPartitions();
    for (POSITION partPos = pLinkedParts->GetHeadPosition(); partPos != NULL;) {
        pLinkedPart = pLinkedParts->GetNext(partPos);
        m_infoGain += float(pLinkedPart->getNumRecords());
    }
    return true;
}
#elif defined(_TD_SCORE_FUNTION_TRANSACTION)
//---------------------------------------------------------------------------
// Compute information gain from all linked partitions.
// Count the number of records having "1".
//---------------------------------------------------------------------------
bool CTDConcept::computeInfoGain(int nClasses)
{
    // No need to compute if already computed.
    if (m_infoGain != 0.0f || !m_bCutCandidate)
        return true;

    if (m_childConcepts.GetSize() == 0) {
        m_bCutCandidate = false;
        return true;
    } 

    // Find the child concept index having value "1".
    int transPresentIdx = -1;
    for (int c = 0; c < m_childConcepts.GetSize(); ++c) {
        if (m_childConcepts[c]->m_conceptValue.CompareNoCase(TD_TRANSACTION_ITEM_PRESENT) == 0) {
            transPresentIdx = c;
            break;
        }
    }
    if (transPresentIdx == -1) {
        cout << _T("CTDConcept::computeInfoGain: Failed to locate transaction item present index.") << endl;
        ASSERT(false);
        return false;
    }

    // For each linked partition.    
    CTDPartAttrib* pPartAttrib = NULL;
    CTDPartition* pLinkedPart = NULL;
    CTDPartitions* pLinkedParts = getRelatedPartitions();
    for (POSITION partPos = pLinkedParts->GetHeadPosition(); partPos != NULL;) {
        pLinkedPart = pLinkedParts->GetNext(partPos);
        pPartAttrib = pLinkedPart->getPartAttribs()->GetAt(pLinkedPart->getPartAttribs()->FindIndex(m_pAttrib->m_attribIdx));
        if (!pPartAttrib->m_bCandidate) {
            m_bCutCandidate = false;
            return true;
        } 
        m_infoGain += float((*pPartAttrib->getSupportSums())[transPresentIdx]);
    }
    return true;
}
#elif defined(_TD_SCORE_FUNTION_DISCERNIBILITY)
//---------------------------------------------------------------------------
// Compute information gain from all linked partitions.
//---------------------------------------------------------------------------
bool CTDConcept::computeInfoGain(int nClasses)
{
    // No need to compute if already computed.
    if (m_infoGain != 0.0f || !m_bCutCandidate)
        return true;

    // For each linked partition.    
    CTDPartition* pLinkedPart = NULL;
    CTDPartitions* pLinkedParts = getRelatedPartitions();
    for (POSITION partPos = pLinkedParts->GetHeadPosition(); partPos != NULL;) {
        pLinkedPart = pLinkedParts->GetNext(partPos);
        m_infoGain += float(square(pLinkedPart->getNumRecords()));
    }
    return true;
}
#elif defined(_TD_SCORE_FUNCTION_DEFAULT) || defined(_TD_SCORE_FUNCTION_INFOGAIN)
//---------------------------------------------------------------------------
// Compute information gain from all linked partitions.
//---------------------------------------------------------------------------
bool CTDConcept::computeInfoGain(int nClasses)
{
    if (m_infoGain != 0.0f || !m_bCutCandidate)
        return true;

    // Allocate the matrix
    int nChildConcepts = getNumChildConcepts();
    int dims[] = {nChildConcepts, nClasses};
    CTDMDIntArray supMatrix(sizeof(dims) / sizeof(int), dims);

    // Allocate support sums
    CTDIntArray supSums;
    supSums.SetSize(nChildConcepts);

    // Allocate class sums
    CTDIntArray classSums;
    classSums.SetSize(nClasses);
    
    // Initialization
    int i = 0;
    int j = 0;
    for (i = 0; i < nChildConcepts; ++i) {
        for (j = 0; j < nClasses; ++j) {
            supMatrix[i][j] = 0;
            classSums.SetAt(j, 0);
        }
        supSums.SetAt(i, 0);
    }

    // For each linked partition.
    CTDPartAttrib* pPartAttrib = NULL;
    CTDPartition* pLinkedPart = NULL;
    CTDPartitions* pLinkedParts = getRelatedPartitions();
    for (POSITION partPos = pLinkedParts->GetHeadPosition(); partPos != NULL;) {
        pLinkedPart = pLinkedParts->GetNext(partPos);
        pPartAttrib = pLinkedPart->getPartAttribs()->GetAt(pLinkedPart->getPartAttribs()->FindIndex(m_pAttrib->m_attribIdx));
        if (!pPartAttrib->m_bCandidate) {
            m_bCutCandidate = false;
            return true;
        } 
        
        // Add up all the counts.
        ASSERT(pPartAttrib->getSupportSums()->GetSize() == nChildConcepts);        
        for (i = 0; i < nChildConcepts; ++i) {
            for (j = 0; j < nClasses; ++j) {
                supMatrix[i][j] += (*pPartAttrib->getSupportMatrix())[i][j];
            }
            supSums[i] += (*pPartAttrib->getSupportSums())[i];
        }

        ASSERT(pPartAttrib->getClassSums()->GetSize() == nClasses);
        for (j = 0; j < nClasses; ++j) {
            classSums[j] += (*pPartAttrib->getClassSums())[j];
        }
    }

    if (!computeInfoGainHelper(computeEntropy(&classSums), supSums, classSums, supMatrix, m_infoGain)) {
        ASSERT(false);
        return false;
    }
    return true;
}
#else
    XXXXXXXXXXXXXXX;
#endif

//---------------------------------------------------------------------------
// Compute information gain from linked partitions.
//---------------------------------------------------------------------------
// static
bool CTDConcept::computeInfoGainHelper(float entropy, 
                                       const CTDIntArray& supSums, 
                                       const CTDIntArray& classSums, 
                                       CTDMDIntArray& supMatrix,
                                       float& infoGainDiff)
{
    infoGainDiff = 0.0f;
    int total = 0, s = 0;
    int nSupports = supSums.GetSize();    
    for (s = 0; s < nSupports; ++s)
        total += supSums.GetAt(s);
    ASSERT(total > 0);

    int nClasses = classSums.GetSize();
    int c = 0;
    float r = 0.0f, mutualInfo = 0.0f, infoGainS = 0.0f;
    for (s = 0; s < nSupports; ++s) {
        infoGainS = 0.0f;
        for (c = 0; c < nClasses; ++c) {
            //ASSERT(supSums->GetAt(s) > 0); It is possible that some classes have 0 support.
            r = float(supMatrix[s][c]) / supSums.GetAt(s);
            if (r > 0.0f) 
                infoGainS += (r * log2f(r)) * -1; 
        }        
        mutualInfo += (float(supSums.GetAt(s)) / total) * infoGainS;
    }
    infoGainDiff = entropy - mutualInfo;
    return true;
}

//---------------------------------------------------------------------------
// Compute entropy of all linked partitions.
//---------------------------------------------------------------------------
// static
float CTDConcept::computeEntropy(CTDIntArray* pClassSums) 
{
    float entropy = calEntropy(pClassSums);
    if (entropy == 0.0f) {
        // One class in this partition.
        return FLT_MAX;
    }
    return entropy;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDConcept::parseFirstConcept(CString& firstConcept, CString& restStr)
{
    firstConcept.Empty();
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] !=  TD_CONHCHY_OPENTAG || 
        restStr[len - 1] !=  TD_CONHCHY_CLOSETAG) {
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
                firstConcept = restStr.Left(i + 1);
                restStr = restStr.Mid(i + 1);
                CBFStrHelper::trim(restStr);
                return true;
            }
        }
    }
    ASSERT(false);
    return false;
}

//*****************
// CTDDiscConcept *
//*****************

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDDiscConcept::CTDDiscConcept(CTDAttrib* pAttrib) 
    : CTDConcept(pAttrib), m_pSplitConcept(NULL)
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDDiscConcept::~CTDDiscConcept() 
{
}

//---------------------------------------------------------------------------
// {Any Location {BC {Vancouver} {Surrey} {Richmond}} {AB {Calgary} {Edmonton}}}
//---------------------------------------------------------------------------
bool CTDDiscConcept::initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches)
{
    // Parse the conceptValue and the rest of the string.
    CString restStr;
    if (!parseConceptValue(conceptStr, m_conceptValue, restStr)) {
        cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << conceptStr << endl;
        return false;
    }
    m_depth = depth;

    // Depth-first build.
    CString firstConcept;
    while (!restStr.IsEmpty()) {
        if (!parseFirstConcept(firstConcept, restStr)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << restStr << endl;
            return false;
        }

        CTDDiscConcept* pNewConcept = new CTDDiscConcept(m_pAttrib);
        if (!pNewConcept)
            return false;
        
        if (!pNewConcept->initHierarchy(firstConcept, depth + 1, maxBranches)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << firstConcept << endl;
            return false;
        }

        if (!addChildConcept(pNewConcept))
            return false;
    }

    // Update the maximum # of branches at this level
    int nChildren = getNumChildConcepts();
    if (nChildren > 0) {
        while (depth > maxBranches.GetUpperBound())
            maxBranches.Add(0);
        if (nChildren > maxBranches[depth])
            maxBranches[depth] = nChildren;
    }
    return true;
}

//---------------------------------------------------------------------------
// Split this discrete concept if it is a suppression attribute.
// 1) Join all the records in related partitions.
// 2) Find the optimal split point.
// 3) Add the child concepts to this concept.
//---------------------------------------------------------------------------
bool CTDDiscConcept::divideConcept(int nClasses)
{
    if (!getAttrib()->isMaskTypeSup()) {
        // Not suppression. No need to split.
        return true;
    }

    if (m_pSplitConcept) {
        // No need to split again.
        return true;
    }

    if (getNumChildConcepts() == 0) {
        m_bCutCandidate = false;
        return true;
    }

    // Join all the records in related partitions.
    CTDRecords jointRecs;
    if (!m_pRelatedPartitions->joinPartitions(jointRecs))
        return false;

    if (jointRecs.GetSize() <= 0) {
        // not sure whether it will happen.
        ASSERT(false);
        m_bCutCandidate = false;
        return true;
    }

    // Find optimal split point.
    if (!findOptimalSplitPoint(jointRecs, nClasses))
        return false;

    // Cannot split on this concept.
    if (!m_bCutCandidate)
        return true;

    // Add the child concepts to this concept.
    if (!makeChildConcepts())
        return false;

    return true;
}

//---------------------------------------------------------------------------
// Find the child concept that yields maximum information gain.
// 1. First count support for each child concept.
// 2. Compute entropy.
// 3. Compute infogain for each child concept.
//---------------------------------------------------------------------------
bool CTDDiscConcept::findOptimalSplitPoint(CTDRecords& recs, int nClasses)
{
    int nChildConcepts = getNumChildConcepts();
    if (!initSplitMatrix(nChildConcepts, nClasses)) {
        ASSERT(false);
        return false;
    }

    if (m_pAttrib->isContinuous()) {
        ASSERT(false);
        return false;
    }

    // Support count for each child concept.
    int attribIdx = m_pAttrib->m_attribIdx;
    int classIdx = recs.GetAt(0)->getNumValues() - 1;
    int nRecs = (int) recs.GetSize();
    int childConceptIdx = 0, childClassIdx = 0;   
    CTDValue* pVal = NULL;
    CTDRecord* pRec = NULL;    
    for (int r = 0; r < nRecs; ++r) {
        // Get the class concept and lower concept.
        pRec = recs.GetAt(r);
        childConceptIdx = pRec->getValue(attribIdx)->getLowerConcept()->m_childIdx;
        childClassIdx = pRec->getValue(classIdx)->getCurrentConcept()->m_childIdx;

        // Compute support counters.            
        ++((*m_pSplitSupMatrix)[childConceptIdx][childClassIdx]);
        
        // Compute support sums and class sums.
        ++(m_splitSupSums[childConceptIdx]);
        ++(m_splitClassSums[childClassIdx]);
    }

    // Compute entropy.  
    float entropy = computeEntropy(&m_splitClassSums);

    // Allocate the matrix
    int dims[] = {2, nClasses};
    CTDMDIntArray supMatrix(sizeof(dims) / sizeof(int), dims);

    // Allocate support sums
    CTDIntArray supSums;
    supSums.SetSize(2);
  
    // Compute information gain for each child concept.
    int i = 0, j = 0;
    float infoGain = 0.0f, maxInfoGain = -1.0f;
    for (int c = 0; c < nChildConcepts; ++c) {
        // Reset counters
        for (i = 0; i < 2; ++i) {
            for (j = 0; j < nClasses; ++j)
                supMatrix[i][j] = 0;
            supSums[i] = 0;
        }

        // Reduce the matrix:
        //        Y   N
        //     L  2   5
        //     M  7   2      <-- Suppose this is c.
        //     H  9   1
        // into a matrix:
        //        Y   N
        //     c  7   2
        // not c  11  6
        for (i = 0; i < nChildConcepts; ++i) {
            for (j = 0; j < nClasses; ++j) {
                if (i == c)
                    supMatrix[0][j] = (*m_pSplitSupMatrix)[i][j];
                else
                    supMatrix[1][j] += (*m_pSplitSupMatrix)[i][j];
            }
            
            if (i == c)
                supSums[0] = m_splitSupSums[i];
            else
                supSums[1] += m_splitSupSums[i];
        }

        if (!computeInfoGainHelper(entropy, supSums, m_splitClassSums, supMatrix, infoGain)) {
            ASSERT(false);
            return false;
        }

        if (infoGain > maxInfoGain) {
            maxInfoGain = infoGain;
            m_pSplitConcept = getChildConcept(c);
        }
    }

    // It cannot be a cut candidate.
    if (!m_pSplitConcept) {
        cerr << _T("CTDDiscConcept: Failed to split concept ") << m_conceptValue << endl;
        ASSERT(false);  // should not happen.
        m_bCutCandidate = false;
    }
    return true;
}

//---------------------------------------------------------------------------
// Transform hierarchy from:
//      Any
//     / | \
//    A  B  C
// into:
//      Any
//      /  \
//     B   Any
//         / \
//        A   C
// Privided that B is the winner.
//---------------------------------------------------------------------------
bool CTDDiscConcept::makeChildConcepts()
{
    if (!m_pSplitConcept) {
        cerr << _T("CTDDiscConcept: Failed to make child concepts ") << m_conceptValue << endl;
        ASSERT(false);
        return false;
    }

    // Make right child.
    CTDDiscConcept* pRightConcept = new CTDDiscConcept(m_pAttrib);
    if (!pRightConcept) {
        ASSERT(false);
        return false;
    }
    pRightConcept->m_conceptValue = m_conceptValue;    
    pRightConcept->m_flattenIdx = (int)  m_pAttrib->getFlattenConcepts()->Add(pRightConcept);
    
    // Add split concept to this concept.
    // Then add non-split child concepts under this right child.
    CTDConcepts tempConcepts;
    tempConcepts.Append(m_childConcepts);
    m_childConcepts.RemoveAll();
    CTDConcept* pChildConcept = NULL;
    for (int c = 0; c < tempConcepts.GetSize(); ++c) {
        pChildConcept = tempConcepts.GetAt(c);
        if (pChildConcept == m_pSplitConcept) {
            //std::cout << _T("Disclosed:") << pChildConcept->m_conceptValue << std::endl;
            if (!addChildConcept(pChildConcept))
                return false;
        }
        else {
            if (!pRightConcept->addChildConcept(pChildConcept))
                return false;
        }
    }   
    if (!addChildConcept(pRightConcept))
        return false;

    // Indicate it is not specialization candidate if it has no child concepts.
    if (pRightConcept->getNumChildConcepts() == 0)
        pRightConcept->m_bCutCandidate = false;

    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDDiscConcept::toString()
{
    return m_conceptValue;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDDiscConcept::parseConceptValue(LPCTSTR str, CString& conceptVal, CString& restStr)
{
    conceptVal.Empty();
    restStr.Empty();

    CString wrkStr = str;
    if (wrkStr.GetLength() < 2 ||
        wrkStr[0] !=  TD_CONHCHY_OPENTAG || 
        wrkStr[wrkStr.GetLength() - 1] !=  TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }

    // Extract "Canada {BC {Vancouver} {Surrey} {Richmond}} {AB {Calgary} {Edmonton}}"
    wrkStr = wrkStr.Mid(1, wrkStr.GetLength() - 2);
    CBFStrHelper::trim(wrkStr);
    if (wrkStr.IsEmpty()) {
        ASSERT(false);
        return false;
    }

    // Extract "Canada"
    int openPos = wrkStr.Find(TD_CONHCHY_OPENTAG);
    if (openPos < 0) {
        // This is root value, e.g., "Vancouver".
        conceptVal = wrkStr;
        return true;
    }
    else {
        conceptVal = wrkStr.Left(openPos);
        CBFStrHelper::trim(conceptVal);
        if (conceptVal.IsEmpty()) {
            ASSERT(false);
            return false;
        }
    }

    // Extract "{BC {Vancouver} {Surrey} {Richmond}} {AB {Calgary} {Edmonton}}"
    restStr = wrkStr.Mid(openPos);
    return true;
}

//*****************
// CTDContConcept *
//*****************

CTDContConcept::CTDContConcept(CTDAttrib* pAttrib) 
    : CTDConcept(pAttrib), m_lowerBound(0.0f), m_upperBound(0.0f), m_splitPoint(FLT_MAX)
{
}

CTDContConcept::~CTDContConcept() 
{
}

//---------------------------------------------------------------------------
// {0-100 {0-50 {<25} {25-50}} {50-100 {50-75} {75-100}}}
//---------------------------------------------------------------------------
bool CTDContConcept::initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches)
{
    // Parse the conceptValue and the rest of the string.
    CString restStr;
    if (!parseConceptValue(conceptStr, m_conceptValue, restStr, m_lowerBound, m_upperBound)) {
        cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << conceptStr << endl;
        return false;
    }
#ifdef _TD_MANUAL_CONTHRCHY
    // Depth-first build.
    CString firstConcept;
    while (!restStr.IsEmpty()) {
        if (!parseFirstConcept(firstConcept, restStr)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << restStr << endl;
            return false;
        }

        CTDContConcept* pNewConcept = new CTDContConcept();
        if (!pNewConcept)
            return false;
        
        if (!pNewConcept->initHierarchy(firstConcept)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << firstConcept << endl;
            return false;
        }

        if (!addChildConcept(pNewConcept))
            return false;
    }
#endif
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDContConcept::toString()
{
#ifdef _TD_TREAT_CONT_AS_CONT
    CString str;
    LPTSTR tempStr = FloatToStr((m_lowerBound + m_upperBound) / 2.0f, TD_CONTVALUE_NUMDEC);
    str = tempStr;
    delete [] tempStr;
    return str;    
#else
    CString str, tempStr;
    CBFStrHelper::floatToStr(m_lowerBound, TD_CONTVALUE_NUMDEC, tempStr);
    str += tempStr;
    str += _T("-");
    CBFStrHelper::floatToStr(m_upperBound, TD_CONTVALUE_NUMDEC, tempStr);
    str += tempStr;
    return str;
#endif
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDContConcept::parseConceptValue(LPCTSTR str, 
                                       CString& conceptVal, 
                                       CString& restStr, 
                                       float& lowerBound, 
                                       float& upperBound)
{
    conceptVal.Empty();
    restStr.Empty();
    lowerBound = 0.0f;
    upperBound = 0.0f;

    CString wrkStr = str;
    if (wrkStr.GetLength() < 2 ||
        wrkStr[0] !=  TD_CONHCHY_OPENTAG || 
        wrkStr[wrkStr.GetLength() - 1] !=  TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }

    wrkStr = wrkStr.Mid(1, wrkStr.GetLength() - 2);
    CBFStrHelper::trim(wrkStr);
    if (wrkStr.IsEmpty()) {
        ASSERT(false);
        return false;
    }

    // Extract "0-100"
    LPTSTR tempStr = NULL;
    int openPos = wrkStr.Find(TD_CONHCHY_OPENTAG);
    if (openPos < 0) {
        // This is root value, e.g., "0-50".
        conceptVal = wrkStr;
        if (!parseLowerUpperBound(conceptVal, lowerBound, upperBound))
            return false;
    }
    else {
        conceptVal = wrkStr.Left(openPos);
        CBFStrHelper::trim(conceptVal);
        if (conceptVal.IsEmpty()) {
            ASSERT(false);
            return false;
        }

        if (!parseLowerUpperBound(conceptVal, lowerBound, upperBound))
            return false;
    }

    // Convert again to make sure exact match for decimal places.
    if (!makeRange(lowerBound, upperBound, conceptVal)) {
        ASSERT(false);
        return false;
    }

    restStr = wrkStr.Mid(openPos);
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDContConcept::makeRange(float lowerB, float upperB, CString& range)
{
    CString tempStr;
    CBFStrHelper::floatToStr(lowerB, TD_CONTVALUE_NUMDEC, tempStr);
    range = tempStr;
    CBFStrHelper::floatToStr(upperB, TD_CONTVALUE_NUMDEC, tempStr);
    range += _T("-");
    range += tempStr;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDContConcept::parseLowerUpperBound(const CString& str, float& lowerB, float& upperB)
{
    lowerB = upperB = 0.0f;

    int dashPos = str.Find(TD_CONHCHY_DASHSYM);
    if (dashPos < 0) {
        cerr << _T("CTDDiscConcept: Failed to parse ") << str << endl;
        ASSERT(false);
        return false;
    }

    CString lowStr = str.Left(dashPos);
    CBFStrHelper::trim(lowStr);
    if (lowStr.IsEmpty()) {
        cerr << _T("CTDDiscConcept: Failed to parse ") << str << endl;
        ASSERT(false);
        return false;
    }
    
    CString upStr = str.Mid(dashPos + 1);
    CBFStrHelper::trim(upStr);
    if (upStr.IsEmpty()) {
        cerr << _T("CTDDiscConcept: Failed to parse ") << str << endl;
        ASSERT(false);
        return false;
    }

    lowerB = (float) CBFStrHelper::strToFloat((LPCTSTR) lowStr);
    upperB = (float) CBFStrHelper::strToFloat((LPCTSTR) upStr);
    if (lowerB > upperB) {
        ASSERT(false);
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------
// Split this continuous concept.
// 1) Join all the records in related partitions.
// 2) Sort the joint recrods according to their raw values of this attribute.
// 3) Find the optimal split point.
// 4) Add the child concepts to this concept.
//---------------------------------------------------------------------------
bool CTDContConcept::divideConcept(int nClasses)
{
    if (m_splitPoint != FLT_MAX) {
        // No need to find again.        
        return true;
    }

    // Join all the records in related partitions.
    CTDRecords jointRecs;
    if (!getRelatedPartitions()->joinPartitions(jointRecs))
        return false;

    if (jointRecs.GetSize() <= 0) {
        // not sure whether it will happen.
        ASSERT(false);
        m_bCutCandidate = false;
        return true;
    }

    // Sort the joint recrods according to their raw values of this attribute.
    if (!jointRecs.sortByAttrib(m_pAttrib->m_attribIdx))
        return false;

    // Find optimal split point.
    if (!findOptimalSplitPoint(jointRecs, nClasses))
        return false;

    // Cannot split on this concept.
    if (!m_bCutCandidate)
        return true;

    // Add the child concepts to this concept.
    if (!makeChildConcepts())
        return false;

    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDContConcept::findOptimalSplitPoint(CTDRecords& recs, int nClasses)
{
    if (!initSplitMatrix(2, nClasses))
        return false;

    if (!m_pAttrib->isContinuous()) {
        ASSERT(false);
        return false;
    }

    int attribIdx = m_pAttrib->m_attribIdx;
    int classIdx = recs.GetAt(0)->getNumValues() - 1;
    int nRecs = recs.GetSize();

    // Initialize counters
	int r = 0;
    CTDRecord* pCurrRec = NULL;
    CTDRecord* pNextRec = NULL;
    CTDConcept* pClassConcept = NULL;
    for (r = 0; r < nRecs; ++r) {
        pCurrRec = recs.GetAt(r);
        pClassConcept = pCurrRec->getValue(classIdx)->getCurrentConcept();
        ++((*m_pSplitSupMatrix)[1][pClassConcept->m_childIdx]);
        ++(m_splitSupSums[1]);
        ++(m_splitClassSums[pClassConcept->m_childIdx]);
    }
    
    float entropy = 0.0f;
    float infoGain = 0.0f;
    float maxInfoGain = -1.0f;
    CTDNumericValue* pCurrValue = NULL;
    CTDNumericValue* pNextValue = NULL;
    for (r = 0; r < nRecs - 1; ++r) {
        pCurrRec = recs.GetAt(r);
        pNextRec = recs.GetAt(r + 1);
        pCurrValue = static_cast<CTDNumericValue*> (pCurrRec->getValue(attribIdx));
        pNextValue = static_cast<CTDNumericValue*> (pNextRec->getValue(attribIdx));
        if (!pCurrValue || !pNextValue) {
            ASSERT(false);
            return false;
        }

        // Get the class concept.
        pClassConcept = pCurrRec->getValue(classIdx)->getCurrentConcept(); 

        // Compute support counters.            
        ++((*m_pSplitSupMatrix)[0][pClassConcept->m_childIdx]);
        --((*m_pSplitSupMatrix)[1][pClassConcept->m_childIdx]);
        
        // Compute support sums, but class sums remain unchanged.
        ++(m_splitSupSums[0]);
        --(m_splitSupSums[1]);

        // Compare with next value. If different, then compute info gain.
        if (pCurrValue->getRawValue() != pNextValue->getRawValue()) {
            if (!computeInfoGainHelper(computeEntropy(&m_splitClassSums), m_splitSupSums, m_splitClassSums, *m_pSplitSupMatrix, infoGain))
                return false;

            if (infoGain > maxInfoGain) {
                maxInfoGain = infoGain;
                m_splitPoint = pNextValue->getRawValue();
            }
        }
    }

    // It cannot be a cut candidate
    if (m_splitPoint == FLT_MAX) {
        m_splitPoint = 0.0f;
        m_bCutCandidate = false;
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDContConcept::makeChildConcepts()
{
    // Make left child.
    CTDContConcept* pLeftConcept = new CTDContConcept(m_pAttrib);
    if (!pLeftConcept) {
        ASSERT(false);
        return false;
    }
    pLeftConcept->m_lowerBound = m_lowerBound;
    pLeftConcept->m_upperBound = m_splitPoint;
    pLeftConcept->m_flattenIdx = m_pAttrib->getFlattenConcepts()->Add(pLeftConcept);

    CString tempStr;
    CBFStrHelper::floatToStr(m_lowerBound, TD_CONTVALUE_NUMDEC, tempStr);
    pLeftConcept->m_conceptValue = tempStr;
    pLeftConcept->m_conceptValue += _T("-");    
    CBFStrHelper::floatToStr(m_splitPoint, TD_CONTVALUE_NUMDEC, tempStr);
    pLeftConcept->m_conceptValue += tempStr;
    if (!addChildConcept(pLeftConcept))
        return false;

    // Make right child.
    CTDContConcept* pRightConcept = new CTDContConcept(m_pAttrib);
    if (!pRightConcept) {
        ASSERT(false);
        return false;
    }
    pRightConcept->m_lowerBound = m_splitPoint;
    pRightConcept->m_upperBound = m_upperBound;
    pRightConcept->m_flattenIdx = m_pAttrib->getFlattenConcepts()->Add(pRightConcept);

    CBFStrHelper::floatToStr(m_splitPoint, TD_CONTVALUE_NUMDEC, tempStr);
    pRightConcept->m_conceptValue = tempStr;
    pRightConcept->m_conceptValue += _T("-");
    CBFStrHelper::floatToStr(m_upperBound, TD_CONTVALUE_NUMDEC, tempStr);
    pRightConcept->m_conceptValue += tempStr;
    if (!addChildConcept(pRightConcept))
        return false;

    return true;
}
