// TDAttribMgr.h: interface for the CTDAttribMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDATTRIBMGR_H)
#define TDATTRIBMGR_H

#if !defined(TDATTRIBUTE_H)
    #include "TDAttribute.h"
#endif

class CTDAttribMgr  
{
public:
    CTDAttribMgr(LPCTSTR attributesFile, LPCTSTR nameFile, LPCTSTR supFile, bool bIncludeNonVA);
    virtual ~CTDAttribMgr();

// operations
    bool readAttributes();
    bool writeNameFile();
    bool writeSupFile();

    CTDAttribs* getAttributes() { return &m_attributes; };
    CTDAttrib* getAttribute(int idx) { return m_attributes.GetAt(idx); };
    int getNumAttributes() const { return m_attributes.GetSize(); };

    CTDAttrib* getClassAttrib() { return m_attributes.GetAt(m_attributes.GetSize() - 1); };
    int getNumClasses() { return getClassAttrib()->getConceptRoot()->getNumChildConcepts(); };

    bool pickSpecializeConcept(CTDAttrib*& pSelectedAttrib, CTDConcept*& pSelectedConcept);
    bool computeInfoGain();
    
protected:
// attributes
    CString    m_attributesFile;
    CString    m_nameFile;
    CString    m_supFile;
    CTDAttribs m_attributes;
    bool       m_bIncludeNonVA;
};

#endif
