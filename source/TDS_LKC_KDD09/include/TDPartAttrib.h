// TDPartAttrib.h: interface for the CTDPartAttrib class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDPARTATTRIB_H)
#define TDPARTATTRIB_H

#if !defined(TDATTRIBUTE_H)
    #include "TDAttribute.h"
#endif

class CTDPartAttrib  
{
public:
    CTDPartAttrib(CTDAttrib* pActualAttrib);
    virtual ~CTDPartAttrib();

// operations
    bool initSupportMatrix(CTDConcept* pCurrCon, int nClasses, const CTDConcepts* pSensConcepts);
    
    CTDMDIntArray* getSupportMatrix() { return m_pSupportMatrix; };
    CTDMDIntArray* getSensMatrix() { return m_pSensMatrix; };
    CTDIntArray* getSupportSums() { return &m_supportSums; };
    CTDIntArray* getClassSums() { return &m_classSums; };    
    CTDAttrib* getActualAttrib() { return m_pActualAttrib; };

// attributes
    bool           m_bCandidate;
    POSITION       m_relatedPos;        // Position of the partition in the related list of concept.

protected:
    CTDAttrib*     m_pActualAttrib;    
    CTDMDIntArray* m_pSupportMatrix;    // supports of sensitive concepts on a class
    CTDMDIntArray* m_pSensMatrix;       // supports of sensitive concepts on a child value
    CTDIntArray    m_supportSums;       // sum of supports of each concept
    CTDIntArray    m_classSums;         // sum of classes    
};


typedef CTypedPtrList<CPtrList, CTDPartAttrib*> CTDPartAttribList;
class CTDPartAttribs : public CTDPartAttribList
{
public:
    CTDPartAttribs();
    virtual ~CTDPartAttribs();
};

#endif
