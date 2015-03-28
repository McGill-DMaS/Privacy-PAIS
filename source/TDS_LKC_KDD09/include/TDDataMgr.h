// TDDataMgr.h: interface for the CTDDataMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDDATAMGR_H)
#define TDDATAMGR_H

#if !defined(TDATTRIBMGR_H)
    #include "TDAttribMgr.h"
#endif

#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif

class CTDDataMgr  
{
public:
    CTDDataMgr(LPCTSTR rawDataFile, LPCTSTR transformedDataFile, LPCTSTR transformedTestFile, LPCTSTR transformedSVMDataFile, LPCTSTR transformedSVMTestFile, bool bIncludeNonVA, int nInputRecs, int nTraining);
    virtual ~CTDDataMgr();

// operations
    bool initialize(CTDAttribMgr* pAttribMgr);
    bool readRecords();
    bool writeRecords(bool bRawValue);
    bool writeRecordsSVM();
    CTDRecords* getRecords() { return &m_records; };
    
protected:
    bool addRecord(CTDRecord* pRecord);

// attributes
    CString       m_rawDataFile;
    CString       m_transformedDataFile;
    CString       m_transformedTestFile;
    CString       m_transformedSVMDataFile;
    CString       m_transformedSVMTestFile;
    CTDRecords    m_records;
    CTDAttribMgr* m_pAttribMgr;
    int           m_nInputRecs;
    int           m_nTraining;
    bool          m_bIncludeNonVA;
};

#endif
