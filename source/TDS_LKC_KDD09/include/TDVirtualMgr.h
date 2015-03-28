// TDVirtualMgr.h: interface for the CTDVirtualMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDVIRTUALMGR_H)
#define TDVIRTUALMGR_H

#if !defined(TDVIRTUALID_H)
    #include "TDVirtualID.h"
#endif

#if !defined(TDATTRIBMGR_H)
    #include "TDAttribMgr.h"
#endif

class CTDVirtualMgr  
{
public:
    CTDVirtualMgr(LPCTSTR vidsFile, int defaultL, int defaultK, float defaultC);
    virtual ~CTDVirtualMgr();

// operations
    bool initialize(CTDAttribMgr* pAttribMgr);
    bool readVIDandSensitive();
    bool breakIntoCombinations();
    inline CTDVirtualIDs* getVIDs() { return &m_vids; };
    inline CTDConcepts* getSensConcepts() { return &m_sensConcepts; };
    float getDefaultC() { return m_defaultC; };
    
protected:
// attributes
    int           m_defaultL;
    int           m_defaultK;
    float         m_defaultC;    
    CString       m_vidsFile;
    CTDVirtualIDs m_vids;
    CTDConcepts   m_sensConcepts;
    CTDAttribMgr* m_pAttribMgr;
};

#endif
