// TDPartitioner.h: interface for the CTDPartitioner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDPARTITIONER_H)
#define TDPARTITIONER_H

#if !defined(TDATTRIBMGR_H)
    #include "TDAttribMgr.h"
#endif

#if !defined(TDVIRTUALMGR_H)
    #include "TDVirtualMgr.h"
#endif

#if !defined(TDDATAMGR_H)
    #include "TDDataMgr.h"
#endif

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

class CTDPartitioner  
{
public:
    CTDPartitioner();
    virtual ~CTDPartitioner();

// operations
    bool initialize(CTDAttribMgr* pAttribMgr,
                    CTDVirtualMgr* pVirtualMgr, 
                    CTDDataMgr* pDataMgr);
    bool transformData();
    CTDPartitions* getLeafPartitions() { return &m_leafPartitions; };
    
protected:
    CTDPartition* initRootPartition();
    bool splitPartitions(CTDAttrib* pSplitAttrib, CTDConcept* pSplitConcept);
    bool distributeRecords(CTDPartition*  pParentPartition, 
                           CTDAttrib*     pSplitAttrib, 
                           CTDConcept*    pSplitConcept, 
                           CTDPartitions& childPartitions);
    bool updateVIDTrees(CTDAttrib*     pSplitAttrib, 
                        CTDPartition*  pParentPartition, 
                        CTDPartitions* pChildPartitions);
    bool updateVIDTreesMinAC();
    bool computeGR_ALratios();
    bool pretendUpdateVIDTree(CTDPartition*  pParentPartition, 
                              CTDPartAttrib* pPartAttrib, 
                              CTDAttrib*     pAttrib,
                              CTDVirtualID*  pVID);

// attributes
    CTDAttribMgr*  m_pAttribMgr;
    CTDVirtualMgr* m_pVirtualMgr;
    CTDDataMgr*    m_pDataMgr;
    CTDPartitions  m_leafPartitions;
};

#endif
