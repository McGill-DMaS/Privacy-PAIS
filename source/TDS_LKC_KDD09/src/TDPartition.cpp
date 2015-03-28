// TDPartition.cpp: implementation of the CTDPartition class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

//***************
// CTDPartition *
//***************

CTDPartition::CTDPartition(int partitionIdx, CTDAttribs* pAttribs, const CTDConcepts* pSensConcepts)
    : m_partitionIdx(partitionIdx), m_leafPos(NULL), m_pSensConcepts(pSensConcepts)
{
    // Add each attribute
    int nAttribs = pAttribs->GetSize();
    for (int i = 0; i < nAttribs - 1; ++i)
        m_partAttribs.AddTail(new CTDPartAttrib(pAttribs->GetAt(i)));

    // Number of classes
    m_nClasses = pAttribs->GetAt(nAttribs - 1)->getConceptRoot()->getNumChildConcepts();

    // Allocate the sens sums
    m_sensSums.SetSize(m_pSensConcepts->GetSize());
    for (int s = 0; s < m_sensSums.GetSize(); ++s)
        m_sensSums.SetAt(s, 0);
}

CTDPartition::~CTDPartition() 
{
    while (!m_partAttribs.IsEmpty())
        delete m_partAttribs.RemoveHead();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::addRecord(CTDRecord* pRecord)
{
    try {
        m_partRecords.Add(pRecord);
        return true;
    }
    catch (CMemoryException&) {
        ASSERT(false);
        return false;
    }
}

//---------------------------------------------------------------------------
// Deregister this parition from the concept.
//---------------------------------------------------------------------------
bool CTDPartition::deregisterPartition()
{
    // Get the first record of the partition.
    CTDRecord* pFirstRec = getRecord(0);
    if (!pFirstRec) {
        ASSERT(false);
        return false;
    }

    int a = 0;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCurrentConcept = NULL;
    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++a) {
        // Find the current concept of this attribute.
        pPartAttrib = m_partAttribs.GetNext(pos);
        pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();
        if (!pCurrentConcept) {
            ASSERT(false);
            return false;
        }
        pCurrentConcept->deregisterPartition(pPartAttrib->m_relatedPos);
        pPartAttrib->m_relatedPos = NULL;        
    }
    return true;
}

//---------------------------------------------------------------------------
// Register this parition from the concept.
//---------------------------------------------------------------------------
bool CTDPartition::registerPartition()
{
    // Get the first record of the partition.
    CTDRecord* pFirstRec = getRecord(0);
    if (!pFirstRec) {
        ASSERT(false);
        return false;
    }

    int a = 0;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCurrentConcept = NULL;
    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++a) {
        // Find the current concept of this attribute.
        pPartAttrib = m_partAttribs.GetNext(pos);
        pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();
        if (!pCurrentConcept) {
            ASSERT(false);
            return false;
        }
        pPartAttrib->m_relatedPos = pCurrentConcept->registerPartition(this);
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::constructSupportMatrix()
{
    // Get the first record of the partition.
    CTDRecord* pFirstRec = getRecord(0);
    if (!pFirstRec) {
        ASSERT(false);
        return false;
    }

    int a = 0;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCurrentConcept = NULL;
    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++a) {
        // Find the current concept of this attribute.
        pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();
        pPartAttrib = m_partAttribs.GetNext(pos);
        
        if (!pPartAttrib->m_bCandidate)
            continue;

        // Need to find the split point.
        if (pPartAttrib->getActualAttrib()->m_bVirtualAttrib) {
            if (!pCurrentConcept->divideConcept(m_nClasses)) {
                ASSERT(false);
                return false;
            }
        }

        // If this attribute does not have child concepts, this cannot be candidate.
        if (pCurrentConcept->getNumChildConcepts() == 0) {
            pCurrentConcept->m_bCutCandidate = false;
            pPartAttrib->m_bCandidate = false;
            continue;
        }

        // Construct the support matrix.
        if (!pPartAttrib->initSupportMatrix(pCurrentConcept, m_nClasses, m_pSensConcepts)) {
            ASSERT(false);
            return false;
        }
    }

    // Compute the support matrix
    bool bOneClassOnly = false;
    CTDConcept* pClassConcept = NULL;
    CTDConcept* pLowerConcept = NULL;  
    CTDConcept* pCurrConcept = NULL;
    CTDConcept* pSensConcept = NULL;
    CTDAttrib* pSensAttrib = NULL;
    CTDMDIntArray* pSupMatrix = NULL;
    CTDMDIntArray* pSensMatrix = NULL;
    CTDIntArray* pSupSums = NULL;
    CTDIntArray* pClassSums = NULL;
    CTDRecord* pRec = NULL;
    int nRecs = getNumRecords();
    int classIdx = m_partAttribs.GetCount();
    for (int r = 0; r < nRecs; ++r) {       
        pRec = getRecord(r);
        
        // Get the class concept.
        pClassConcept = pRec->getValue(classIdx)->getCurrentConcept();

        // Check whether the record contains sensitive concepts.
        CTDIntArray sensitiveIndices;
        for (int s = 0; s < m_pSensConcepts->GetSize(); ++s) {
            pSensConcept = m_pSensConcepts->GetAt(s);
            pSensAttrib = pSensConcept->getAttrib();
            pCurrConcept = pRec->getValue(pSensAttrib->m_attribIdx)->getCurrentConcept();
            if (pCurrConcept == pSensConcept) {
                sensitiveIndices.Add(s);

                // Compute the sens sum of this matrix.
                ++m_sensSums[s]; 
            }
        }

        // Compute support counts for each attribute
        int aIdx = 0;
        for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++aIdx) {
            // The partition attribute.
            pPartAttrib = m_partAttribs.GetNext(pos);
            if (!pPartAttrib->m_bCandidate)
                continue;

            // Get the lower concept value
            pLowerConcept = pRec->getValue(aIdx)->getLowerConcept();
            if (!pLowerConcept) {
                cerr << _T("No more child concepts. This should not be a candidate.") << endl;
                ASSERT(false);
                return false;
            }

            // Construct the support matrix for classes.
            pSupMatrix = pPartAttrib->getSupportMatrix();
            if (!pSupMatrix) {
                ASSERT(false);
                return false;
            }
            ++((*pSupMatrix)[pLowerConcept->m_childIdx][pClassConcept->m_childIdx]);

            // Compute the support sum of this matrix.
            pSupSums = pPartAttrib->getSupportSums();
            if (!pSupSums) {
                ASSERT(false);
                return false;
            }
            ++((*pSupSums)[pLowerConcept->m_childIdx]);

            // Compute the class sum of this matrix.
            pClassSums = pPartAttrib->getClassSums();
            if (!pClassSums) {
                ASSERT(false);
                return false;
            }
            ++((*pClassSums)[pClassConcept->m_childIdx]);

            // Compute the support matrix for sensitive concepts.
            for (int sIdx = 0; sIdx < sensitiveIndices.GetSize(); ++sIdx) {
                pSensMatrix = pPartAttrib->getSensMatrix();
                if (!pSensMatrix) {
                    ASSERT(false);
                    return false;
                }
                ++((*pSensMatrix)[pLowerConcept->m_childIdx][sIdx]);
            }

            // Check whether this partition only contains one class.
            ASSERT((*pClassSums)[pClassConcept->m_childIdx] <= nRecs);
            if ((*pClassSums)[pClassConcept->m_childIdx] == nRecs) {
                //bOneClassOnly = true;
                //break;
            }
        }

        if (bOneClassOnly)
            break;
    }

    if (bOneClassOnly) {
        // This partition only contains one class. Thus, cannot be further specialized.
#ifdef _DEBUG_PRT_INFO
        cout << _T("* * * * * constructSupportMatrix: Partition #") << m_partitionIdx << _T(" has only one class.") << endl;
#endif
        for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL;) {
            // The partition attribute.            
            pPartAttrib = m_partAttribs.GetNext(pos);
            pPartAttrib->m_bCandidate = false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDPartition::getPartitionVIDPath(CTDPartition* pPartition, CTDVirtualID* pVID, CTDIntArray& vidPath)
{
    vidPath.RemoveAll();
    CTDRecord* pFirstRec = pPartition->getRecord(0);
    if (!pFirstRec) {
        ASSERT(false);
        return false;
    }
    
    CTDAttrib* pVAttrib = NULL;
    CTDValue* pValue = NULL;
    CTDConcept* pParentConcept = NULL;
    CTDAttribs* pVAttribs = pVID->getVirtualAttribs();
    int nVAttribs = pVAttribs->GetSize();
    for (int a = 0; a < nVAttribs; ++a) {
        pVAttrib = pVAttribs->GetAt(a);
        pValue = pFirstRec->getValue(pVAttrib->m_attribIdx);
        vidPath.Add(pValue->getCurrentConceptFlattenIdx());
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDPartition& partition)
{
#ifdef _DEBUG_PRT_INFO
    os << _T("--------------------------------------------------------------------------") << endl;
    os << _T("Partition #") << partition.m_partitionIdx << endl;
    os << partition.m_partRecords;
#endif
    return os;
}

//****************
// CTDPartitions *
//****************
CTDPartitions::CTDPartitions()
{
}

CTDPartitions::~CTDPartitions()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDPartitions::cleanup()
{
    while (!IsEmpty())
        delete RemoveHead();
}

//---------------------------------------------------------------------------
// Delete empty partitions.
//---------------------------------------------------------------------------
void CTDPartitions::deleteEmptyPartitions()
{    
    POSITION tempPos = NULL;
    CTDPartition* pPartition = NULL;
    for (POSITION pos = GetHeadPosition(); pos != NULL;) {
        tempPos = pos;
        pPartition = GetNext(pos);
        if (pPartition->getNumRecords() <= 0) {
            RemoveAt(tempPos);
            delete pPartition;
            pPartition = NULL;
        }
    }
}

//---------------------------------------------------------------------------
// Join all the records in all paritions.
//---------------------------------------------------------------------------
bool CTDPartitions::joinPartitions(CTDRecords& jointRecs)
{
    jointRecs.RemoveAll();
    for (POSITION pos = GetHeadPosition(); pos != NULL;) {
        jointRecs.Append(*(GetNext(pos)->getAllRecords()));
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDPartitions& partitions)
{
#ifdef _DEBUG_PRT_INFO
    CTDPartition* pPartition = NULL;
    for (POSITION pos = partitions.GetHeadPosition(); pos != NULL;) {
        pPartition = partitions.GetNext(pos);
        os << *pPartition << endl;
    }
#endif
    return os;
}

