// TDPartitioner.cpp: implementation of the CTDPartitioner class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDPARTITIONER_H)
    #include "TDPartitioner.h"
#endif

static int gPartitionIndex = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDPartitioner::CTDPartitioner() 
    : m_pAttribMgr(NULL), m_pDataMgr(NULL)
{
}

CTDPartitioner::~CTDPartitioner() 
{
    m_leafPartitions.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartitioner::initialize(CTDAttribMgr* pAttribMgr, 
                                CTDVirtualMgr* pVirtualMgr,
                                CTDDataMgr* pDataMgr)
{
    if (!pAttribMgr || !pVirtualMgr || !pDataMgr) {
        ASSERT(false);
        return false;
    }
    m_pAttribMgr = pAttribMgr;
    m_pVirtualMgr = pVirtualMgr;
    m_pDataMgr = pDataMgr;    
    return true;
}

//---------------------------------------------------------------------------
// The main algorithm.
//---------------------------------------------------------------------------
bool CTDPartitioner::transformData()
{
    cout << _T("Partitioning data...") << endl;

    // Initialize the first partition.
    CTDPartition* pRootPartition = initRootPartition();
    if (!pRootPartition)
        return false;

    // Update VID trees.
    CTDPartitions childPartitions;
    childPartitions.AddTail(pRootPartition);
    if (!updateVIDTrees(NULL, NULL, &childPartitions)) {
        delete pRootPartition;
        return false;
    }
    childPartitions.RemoveAll();
    if (!updateVIDTreesMinAC()) {
        delete pRootPartition;
        return false;
    }

    // Register this root partition to the related concepts.
    if (!pRootPartition->registerPartition()) {
        delete pRootPartition;
        return false;
    }

    // Construct raw counts of the partition.
    if (!pRootPartition->constructSupportMatrix()) {
        delete pRootPartition;
        return false;
    }

    // Compute informaiton gain of each concept in the cut.
    if (!m_pAttribMgr->computeInfoGain()) {
        delete pRootPartition;
        return false;
    }

    // Compute anonymity counts of each concept in the cut for each VID.
    // Also, calculate the I/A ratio for each concept in the cut.
    // Filter out the non-candidates.
    if (!computeGR_ALratios()) {
        delete pRootPartition;
        return false;
    }

    // Add root partition to leaf partitions.
    m_leafPartitions.cleanup();
    pRootPartition->m_leafPos = m_leafPartitions.AddTail(pRootPartition);
    pRootPartition = NULL;

    // Select an attribute to specialize.
    int splitCounter = 0;
    CTDAttrib* pSelectedAttrib = NULL;
    CTDConcept* pSelectedConcept = NULL;
    while (m_pAttribMgr->pickSpecializeConcept(pSelectedAttrib, pSelectedConcept)) {
#ifdef _DEBUG_PRT_INFO
        cout << _T("* * * * * [Split Counter: ") << splitCounter++ << _T("] * * * * *") << endl;
#endif
        // Split the related partitions based on the selected concept.
        if (!splitPartitions(pSelectedAttrib, pSelectedConcept)) {
            m_leafPartitions.cleanup();
            return false;
        }

        // Compute informaiton gain of each concept in the cut.
        if (!m_pAttribMgr->computeInfoGain()) {
            m_leafPartitions.cleanup();
            return false;
        }

        // Compute anonymity counts of each concept in the cut for each VID.
        // Also, calculate the I/A ratio for each concept in the cut.
        // Filter out the non-candidates.
        if (!computeGR_ALratios()) {
            m_leafPartitions.cleanup();
            ASSERT(false);
            return false;
        }
    }
    
    cout << _T("Partitioning data succeeded.") << endl;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDPartition* CTDPartitioner::initRootPartition()
{
    CTDPartition* pPartition = new CTDPartition(gPartitionIndex++, m_pAttribMgr->getAttributes(), m_pVirtualMgr->getSensConcepts());
    if (!pPartition)
        return NULL;

    CTDRecords* pRecs = m_pDataMgr->getRecords();
    if (!pRecs) {
        delete pPartition;
        return NULL;
    }

    int nRecs = pRecs->GetSize();
    for (int i = 0; i < nRecs; ++i) {
        if (!pPartition->addRecord(pRecs->GetAt(i))) {
            delete pPartition;
            return NULL;
        }
    }

    if (pPartition->getNumRecords() <= 0) {
        cerr << _T("CTDPartitioner: Zero number of records in root partition.") << endl;
        delete pPartition;
        ASSERT(false);
        return NULL;
    }
    return pPartition;
}

//---------------------------------------------------------------------------
// For each related partitions of the split concept {
//      Update the VID path of the parent partitions.
//      Distribute records from parent paritition to child partitions.
//      Update the VID paths of the child partitions.
//      Update the related partition lists.
//      Update leaf partitions.
// }
//---------------------------------------------------------------------------
bool CTDPartitioner::splitPartitions(CTDAttrib* pSplitAttrib, CTDConcept* pSplitConcept)
{
    ASSERT(pSplitAttrib && pSplitConcept);

    // For each partition
    CTDPartitions childPartitions, allChildPartitions;
    CTDPartition* pParentPartition = NULL;
    CTDPartition* pChildPartition = NULL;
    CTDPartitions* pRelParts = pSplitConcept->getRelatedPartitions();
    for (POSITION partPos = pRelParts->GetHeadPosition(); partPos != NULL;) {
        pParentPartition = pRelParts->GetNext(partPos);
#ifdef _DEBUG_PRT_INFO                        
        cout << _T("----------------------[Splitting Parent Partition]------------------------") << endl;
        cout << *pParentPartition;
#endif
        // Deregister this parent partition from the related concepts.
        if (!pParentPartition->deregisterPartition())
            return false;

        // Update the VID path of the parent partitions.
        if (!updateVIDTrees(pSplitAttrib, pParentPartition, NULL))
            return false;

        // Distribute records from parent paritition to child partitions.
        if (!distributeRecords(pParentPartition, pSplitAttrib, pSplitConcept, childPartitions))
            return false;

        // Update the VID paths of the child partitions.
        if (!updateVIDTrees(pSplitAttrib, NULL, &childPartitions))
            return false;

        // Update the min AC of each VID.
        if (!updateVIDTreesMinAC()) 
            return false;

        for (POSITION childPos = childPartitions.GetHeadPosition(); childPos != NULL;) {
            pChildPartition = childPartitions.GetNext(childPos);

            // Register this child partition to the related concepts.
            if (!pChildPartition->registerPartition())
                return false;

            // Add child partitions to leaf partitions.
            pChildPartition->m_leafPos = m_leafPartitions.AddTail(pChildPartition);
            //cout << _T("# of leaf partitions: ") << m_leafPartitions.GetCount() << endl;
#ifdef _DEBUG_PRT_INFO
            cout << _T("------------------------[Splitted Child Partition]------------------------") << endl;
            cout << *pChildPartition;
#endif
        }

        // Remove parent partition from leaf partitions.
        m_leafPartitions.RemoveAt(pParentPartition->m_leafPos);
        delete pParentPartition;
        pParentPartition = NULL;

        // Keep track of all new child partitions.
        allChildPartitions.AddTail(&childPartitions);
    }

    // For each new child partition in this split, compute support matrix and gain ratios.
    for (POSITION childPos = allChildPartitions.GetHeadPosition(); childPos != NULL;) {
        pChildPartition = allChildPartitions.GetNext(childPos);

        // Construct raw counts of the child partition.
        if (!pChildPartition->constructSupportMatrix()) {
            ASSERT(false);
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
// Distribute records from parent paritition to child partitions.
//---------------------------------------------------------------------------
bool CTDPartitioner::distributeRecords(CTDPartition*  pParentPartition,
                                       CTDAttrib*     pSplitAttrib, 
                                       CTDConcept*    pSplitConcept, 
                                       CTDPartitions& childPartitions) 
{
    childPartitions.RemoveAll();

    // Construct a partition for each child concept. 
    for (int childIdx = 0; childIdx < pSplitConcept->getNumChildConcepts(); ++childIdx)
        childPartitions.AddTail(new CTDPartition(gPartitionIndex++, m_pAttribMgr->getAttributes(), m_pVirtualMgr->getSensConcepts()));

    // Scan through each record in the parent partition and
    // add records to the corresponding child partition based
    // on the child concept.
    CTDRecord* pRec = NULL;
    CTDValue* pSplitValue = NULL;
    POSITION childPartitionPos = NULL;
    int childConceptIdx = -1;
    int splitIdx = pSplitAttrib->m_attribIdx;
    int nRecs = pParentPartition->getNumRecords();
    ASSERT(nRecs > 0);
    for (int r = 0; r < nRecs; ++r) {
        pRec = pParentPartition->getRecord(r);
        pSplitValue = pRec->getValue(splitIdx);

        // Lower the concept by one level.
        if (!pSplitValue->lowerCurrentConcept()) {
            cerr << _T("CTDPartition: Should not specialize on this concept.");
            childPartitions.cleanup();
            ASSERT(false);
            return false;
        }

        // Get the child concept of the current concept in this record.
        childConceptIdx = pSplitValue->getCurrentConcept()->m_childIdx;
        ASSERT(childConceptIdx != -1);
        childPartitionPos = childPartitions.FindIndex(childConceptIdx);
        ASSERT(childPartitionPos);

        // Add the record to this child partition.
        if (!childPartitions.GetAt(childPartitionPos)->addRecord(pRec)) {
            childPartitions.cleanup();
            ASSERT(false);                
            return false;
        }
    }

    // Delete empty child partitions.
    childPartitions.deleteEmptyPartitions();
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartitioner::updateVIDTrees(CTDAttrib* pSplitAttrib, CTDPartition* pParentPartition, CTDPartitions* pChildPartitions)
{
    CTDVirtualIDs* pVIDs = m_pVirtualMgr->getVIDs();
    if (!pVIDs) {
        ASSERT(false);
        return false;
    }
    int nVIDs = pVIDs->GetSize();

    // Negate the sensitive sums.
    CTDIntArray negSensSums;
    if (pParentPartition)
        negateNumbers(*pParentPartition->getSensSums(), negSensSums);

    CTDIntArray vidPath;
    CTDVirtualID* pVID = NULL;
    CTDPartition* pChildPartition = NULL;
    for (int v = 0; v < nVIDs; ++v) {
        pVID = pVIDs->GetAt(v);

        // Is it the root partition?
        // Is the split attribute related to this VID?
        if (!pSplitAttrib || pSplitAttrib->isRelated(pVID)) {
            if (pParentPartition) {
                // ---------------------------------------------
                // Update the VID tree of the parent partition.
                // ---------------------------------------------

                // Form the VID path and update its count.
                // Decrement the # of records in parent parition.
                if (!CTDPartition::getPartitionVIDPath(pParentPartition, pVID, vidPath)) {
                    ASSERT(false);
                    return false;
                }
                if (!pVID->updateVPathCount(vidPath, pParentPartition->getNumRecords() * -1, negSensSums, false))
                    return false;

                ASSERT(pVID->getVPathCount(vidPath) >= 0);
            }

            if (pChildPartitions) {
                // ---------------------------------------------
                // Update the VID trees of the child partitions.
                // ---------------------------------------------

                for (POSITION pos = pChildPartitions->GetHeadPosition(); pos != NULL;) {
                    // Form the VID path and update its count.
                    // Increment the # of records in child parition.
                    pChildPartition = pChildPartitions->GetNext(pos);
                    if (!CTDPartition::getPartitionVIDPath(pChildPartition, pVID, vidPath)) {
                        ASSERT(false);
                        return false;
                    }
                    if (!pVID->updateVPathCount(vidPath, pChildPartition->getNumRecords(), *pChildPartition->getSensSums(), false))
                        return false;
                }
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------
// Update the min AC of each VID tree.
//---------------------------------------------------------------------------
bool CTDPartitioner::updateVIDTreesMinAC()
{
    CTDVirtualIDs* pVIDs = m_pVirtualMgr->getVIDs();
    ASSERT(pVIDs);
    int nVIDs = pVIDs->GetSize();
    for (int v = 0; v < nVIDs; ++v) {
        if (!pVIDs->GetAt(v)->updateMinAC_MaxCFNode())
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------
// For each attribute A {
//      For each concept in cut of A {
//          For each attribute of the related paritions of the concept {
//              For each VID {
//                  Test whether specialization is possible.
//                  Keep track of the minAC for this VID.
//              }
//          }
//          If specialization is possible then {
//              Compute the overall anonymity of all VIDs after specialization.
//              Compute the IG/AL ratio.
//          }
//      }
// }
//---------------------------------------------------------------------------
bool CTDPartitioner::computeGR_ALratios()
{
    CTDAttrib* pAttrib = NULL;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCutConcept = NULL;
    CTDPartition* pParentPartition = NULL;
    CTDPartitions* pRelatedPartitions = NULL;
    CTDVirtualID* pVID = NULL;
    CTDVirtualIDs* pVIDs = m_pVirtualMgr->getVIDs();
    int nVIDs = pVIDs->GetSize();
    int v = 0, sumACsDiffs = 0, diffAC = 0, minACafterS = 0;
    float avgACsDiffs = 0.0f, maxCFafterS = 0.0f;

#ifdef _DEBUG_PRT_INFO
    cout << endl;
#endif 

    // Record A(VIDv) before specialization.
    CTDIntArray minACsBeforeS;
    for (v = 0; v < nVIDs; ++v) {
        pVID = pVIDs->GetAt(v);
        minACsBeforeS.Add(pVID->m_pMinACNode->getCount());
#ifdef _DEBUG_PRT_INFO
        cout << _T("VID #") << v << _T(" AC_Before_Split=") << pVID->m_pMinACNode->getCount() << _T(" k=") << pVID->m_kThreshold << endl;
        cout << _T("VID #") << v << _T(" CF_Before_Split=") << pVID->m_pMaxCFNode->getMaxConf() << _T(" k=") << m_pVirtualMgr->getDefaultC() << endl;
#endif
    }    
    
    // For each attribute A 
    int nAttribs = m_pAttribMgr->getNumAttributes() - 1;
    for (int a = 0; a < nAttribs; ++a) {
        pAttrib = m_pAttribMgr->getAttribute(a);

        // Is this VID related to this A?
        CTDBoolArray relatedToVID;
        for (v = 0; v < nVIDs; ++v)
            relatedToVID.Add(pAttrib->isRelated(pVIDs->GetAt(v)));

        // For each concept in the cut of A
        for (POSITION posCut = pAttrib->m_cut.GetHeadPosition(); posCut != NULL;) {
            pCutConcept = pAttrib->m_cut.GetNext(posCut);
            if (!pCutConcept->m_bCutCandidate) {
                // Checked this concept before. It cannot be specialized.
                continue;
            }
           
            for (v = 0; v < nVIDs; ++v) {
                if (!relatedToVID[v]) {
                    // A(VIDv) - A(VIDv, S) must be 0.
                    continue;
                } 
                pVIDs->GetAt(v)->initRollback();
            }

#ifdef _DEBUG_PRT_INFO
            cout << _T("computeGR_ALratios: Attribute_Index=") << pAttrib->m_attribIdx; // << _T(", Name=") << pAttrib->m_attribName;
            cout << _T(", Concept_Index=") << pCutConcept->m_flattenIdx << _T(", Concept_Value=") << pCutConcept->m_conceptValue << endl;
#endif
            // For each partition attribute of the related paritions
            pRelatedPartitions = pCutConcept->getRelatedPartitions();
            for (POSITION posPart = pRelatedPartitions->GetHeadPosition(); posPart != NULL;) {
                pParentPartition = pRelatedPartitions->GetNext(posPart);
                pPartAttrib = pParentPartition->getPartAttribs()->GetAt(pParentPartition->getPartAttribs()->FindIndex(pAttrib->m_attribIdx));
                if (!pPartAttrib->m_bCandidate) {
                    // This partition attribute cannot be further specialized.
                    // Thus, this concept cannot be further specialized.
#ifdef _DEBUG_PRT_INFO
                    cout << _T("Partition #") << pParentPartition->getPartitionIdx();
                    cout << _T(" cannot be further specialized on Attribute_Index=") << pAttrib->m_attribIdx << endl;
#endif
                    pCutConcept->m_bCutCandidate = false;
                    break;
                }
                
                // For each VID
                for (v = 0; v < nVIDs; ++v) {                    
                    if (!relatedToVID[v]) {
                        // Not related VID, no need to perform specialization.
                        continue;
                    }

                    // This VID is related to this attribute.
                    // Then pretend performing the specialization on the VID trees.
                    pVID = pVIDs->GetAt(v);
                    if (!pretendUpdateVIDTree(pParentPartition, pPartAttrib, pAttrib, pVID))
                        return false;
                }
            }

            if (!pCutConcept->m_bCutCandidate) {
                // The concept cannot be a candidate, so rollback.
                for (v = 0; v < nVIDs; ++v) {
                    if (!relatedToVID[v])
                        continue;
                    if (!pVIDs->GetAt(v)->rollback()) {
                        ASSERT(false);
                        return false;
                    }
                }
                continue;
            }

            // All VID trees have been updated by the related partitions for this concept.
            // 1. Check whether k is violated in each VID.
            // 2. Check whether c is violated in each VID.
            // 3. Compute A(VIDv, S) and A(VIDv) - A(VIDv, S).
            // 4. Rollback VID trees.
            sumACsDiffs = 0;
            for (v = 0; v < nVIDs; ++v) {
                if (!relatedToVID[v]) {
                    // A(VIDv) - A(VIDv, S) must be 0.
                    continue;
                }                

                // 1 Check whether k is violated in each VID.
                pVID = pVIDs->GetAt(v);
                if (!pVID->searchMinAC_MaxCF(minACafterS, maxCFafterS))
                    return false;
                if (minACafterS < pVID->m_kThreshold || maxCFafterS > m_pVirtualMgr->getDefaultC()) {
                    // This VID is violated if specialize on this concept.
#ifdef _DEBUG_PRT_INFO
                    cout << _T("AC_After_Split=") << minACafterS << _T(" *Violated VID #") << v << _T("* ");
                    cout << _T("Attribute_Index=") << pAttrib->m_attribIdx; // << _T(", Name=") << pAttrib->m_attribName;
                    cout << _T(", Concept_Index=") << pCutConcept->m_flattenIdx << _T(", Concept_Value=") << pCutConcept->m_conceptValue << endl;
#endif
                    pCutConcept->m_bCutCandidate = false;
                }
                else {
                    // 3. Compute A(VIDv, S) and A(VIDv) - A(VIDv, S).
                    diffAC = minACsBeforeS[v] - minACafterS;
#ifdef _DEBUG_PRT_INFO
                    cout << _T("AC_After_Split=") << minACafterS << endl;
                    cout << _T("CF_After_Split=") << maxCFafterS << endl;
#endif
                    ASSERT(diffAC >= 0);       // related VID should have diff > 0.
                    sumACsDiffs += diffAC;    // Sum up the difference.
                }

                // 4. Rollback VID trees.
                if (!pVID->rollback()) {
                    ASSERT(false);
                    return false;
                }
#ifdef _DEBUG_PRT_INFO
                if (!pVID->searchMinAC_MaxCF(minACafterS, maxCFafterS))
                    return false;
                ASSERT(minACsBeforeS[v] == minACafterS);
                cout << _T("AC_After_Rollback=") << minACafterS << endl;
                cout << _T("CF_After_Rollback=") << maxCFafterS << endl;
#endif
            }

            if (!pCutConcept->m_bCutCandidate) {
                // Does not satisfy k, can't be a candidate.
                continue;
            }

            avgACsDiffs = float(sumACsDiffs) / nVIDs;
            avgACsDiffs += 1.0f;

            // Finally, we compute the GR ratio for this concept.
#if defined(_TD_SCORE_FUNCTION_DEFAULT) || defined(_TD_SCORE_FUNCTION_DISTORTION_BY_ANONYLOSS)
            pCutConcept->m_infoAnoyRatio = pCutConcept->m_infoGain / avgACsDiffs;
#else
            pCutConcept->m_infoAnoyRatio = pCutConcept->m_infoGain;
#endif

#ifdef _DEBUG_PRT_INFO
            cout << _T("computeGR_ALratios: IG =") << pCutConcept->m_infoGain;
            cout << _T(", Avg_AL=") << avgACsDiffs;
            cout << _T(", IG/AL=") << pCutConcept->m_infoAnoyRatio << endl;
#endif
        }
    }
    return true;
}

//---------------------------------------------------------------------------
// Update the VID tree, but we will rollback later.
//---------------------------------------------------------------------------
bool CTDPartitioner::pretendUpdateVIDTree(CTDPartition*  pParentPartition, 
                                          CTDPartAttrib* pPartAttrib, 
                                          CTDAttrib*     pAttrib,
                                          CTDVirtualID*  pVID)
{
#ifdef _DEBUG_PRT_INFO
    cout << _T("Pretend splitting on Partition #") << pParentPartition->getPartitionIdx() << _T(" Attribute_Index=") << pAttrib->m_attribIdx << endl;
#endif

    // Negate the sensitive sums.
    CTDIntArray negSensSums;
    if (pParentPartition)
        negateNumbers(*pParentPartition->getSensSums(), negSensSums);

    // Form the VID path of parent partition and update its VID count.
    CTDIntArray vidParentPath;
    if (!CTDPartition::getPartitionVIDPath(pParentPartition, pVID, vidParentPath)) {
        ASSERT(false);
        return false;
    }
    if (!pVID->updateVPathCount(vidParentPath, pParentPartition->getNumRecords() * -1, negSensSums, true)) {
        ASSERT(false);
        return false;
    }
    ASSERT(pVID->getVPathCount(vidParentPath) >= 0);

    // Form vid path for next level value.
    // First, form the vid path using the current partition VID path.
    // Then only update the vid code that matches the partAttrib.
    CTDAttribs* pVAttribs = pVID->getVirtualAttribs();
    int vCodePos = -1, nVAttribs = pVAttribs->GetSize();
    CTDIntArray vidPathNextLevel;
    vidPathNextLevel.Append(vidParentPath);    
    for (int a = 0; a < nVAttribs; ++a) {
        if (pAttrib->m_attribIdx == pVAttribs->GetAt(a)->m_attribIdx)
            vCodePos = a;
    }
    ASSERT(vCodePos != -1);

    // Get the first record.
    // Then compute the test count by first retrieving Anonymity Count from VID tree,
    // and retrieving the raw support count.
    CTDRecord* pFirstRec = pParentPartition->getRecord(0);
    if (!pFirstRec) {
        ASSERT(false);
        return false;
    }
    int rc = -1;    
    CTDConcept* pCurrConcept = pFirstRec->getValue(pAttrib->m_attribIdx)->getCurrentConcept();
    CTDConcept* pChildConcept = NULL;
    CTDMDIntArray* pSensMatrix = pPartAttrib->getSensMatrix();
    int nChildConcepts = pCurrConcept->getNumChildConcepts();
    for (int c = 0; c < nChildConcepts; ++c) {
        pChildConcept = pCurrConcept->getChildConcept(c);

        // Update only one of the VID codes in the VID path. 
        vidPathNextLevel.SetAt(vCodePos, pChildConcept->m_flattenIdx);

        // Obtain raw count.
        rc = (*pPartAttrib->getSupportSums())[pChildConcept->m_childIdx];

        // Obtain counts of sensitive concepts.
        CTDIntArray sensSums;
        for (int s = 0; s < negSensSums.GetSize(); ++s)     // For efficiency, we negSenSums because every array of sensitive concepts has the same size.
            sensSums.Add((*pSensMatrix)[pChildConcept->m_childIdx][s]);

        // Update the tree.
        if (!pVID->updateVPathCount(vidPathNextLevel, rc, sensSums, true)) {
            ASSERT(false);
            return false;
        }        
    }
    return true;
}
