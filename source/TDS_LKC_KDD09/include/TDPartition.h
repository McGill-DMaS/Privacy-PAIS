// TDPartition.h: interface for the CTDPartition class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDPARTITION_H)
#define TDPARTITION_H

#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif

#if !defined(TDPARTATTRIB_H)
    #include "TDPartAttrib.h"
#endif

#if !defined(TDVIRTUALID_H)
    #include "TDVirtualID.h"
#endif

class CTDPartition  
{
public:
    CTDPartition(int partitionIdx, CTDAttribs* pAttribs, const CTDConcepts* pSensConcepts);
    virtual ~CTDPartition();

// operations
    int getPartitionIdx() { return m_partitionIdx; };
    CTDPartAttribs* getPartAttribs() { return &m_partAttribs; };

    bool addRecord(CTDRecord* pRecord);
    int getNumRecords() { return m_partRecords.GetSize(); };
    CTDRecord* getRecord(int idx) { return m_partRecords.GetAt(idx); };
    CTDRecords* getAllRecords() { return &m_partRecords; };
    CTDIntArray* getSensSums() { return &m_sensSums; };
    
    bool deregisterPartition();
    bool registerPartition();
    bool constructSupportMatrix();

    static bool getPartitionVIDPath(CTDPartition* pPartition,CTDVirtualID* pVID, CTDIntArray& vidPath);
    friend ostream& operator<<(ostream& os, const CTDPartition& partition);

    POSITION m_leafPos;             // Position in the leaf list.

protected:
// attributes
    int m_partitionIdx;
    CTDPartAttribs m_partAttribs;   // Pointers to attributes of this partition.
    CTDRecords m_partRecords;       // Pointers to records of this partition.    
    int m_nClasses;                 // Number of classes.

    const CTDConcepts* m_pSensConcepts; // Sensitive concepts
    CTDIntArray m_sensSums;             // sum of sensitive concepts
};


typedef CTypedPtrList<CPtrList, CTDPartition*> CTDPartitionList;
class CTDPartitions : public CTDPartitionList
{
public:
    CTDPartitions();
    virtual ~CTDPartitions();
    void cleanup();

    void deleteEmptyPartitions();
    bool joinPartitions(CTDRecords& jointRecs);

    friend ostream& operator<<(ostream& os, const CTDPartitions& partitions);
};

#endif
