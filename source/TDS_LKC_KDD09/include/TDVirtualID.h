// TDVirtualID.h: interface for the CTDVirtualID class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDVIRTUALID_H)
#define TDVIRTUALID_H

#if !defined(TDATTRIBUTE_H)
    #include "TDAttribute.h"
#endif

#if !defined(TDVIRTUALTREE_H)
    #include "TDVirtualTree.h"
#endif

#if !defined(TDROLLBACKREC_H)
    #include "TDRollbackRec.h"
#endif

class CTDVirtualIDs;

class CTDVirtualID
{
public:
    CTDVirtualID(int defaultK);
    virtual ~CTDVirtualID();

// operations
    bool initVID(LPCTSTR vidStr, CTDAttribs* pAttribs);    
    static bool parseVIDAttrib(CString& firstAttrib, CString& restStr);

    CTDAttribs* getVirtualAttribs() { return &m_virtualAttribs; };

    int getVPathCount(const CTDIntArray& vPath);
    bool updateVPathCount(const CTDIntArray& vPath, int count, const CTDIntArray& sensCounts, bool bRollback);

    bool updateMinAC_MaxCFNode();
    bool searchMinAC_MaxCF(int& minAC, float& maxCF);

    void initRollback() { m_rollbackRecs.cleanup(); };
    bool rollback();    

    bool combinations(int req_len, CTDVirtualIDs& vids);
    CString toString() const;

    int m_kThreshold;
    CTDVirtualTreeNode* m_pMinACNode;   // The node of the minimum anonymity count of this VID.
    CTDVirtualTreeNode* m_pMaxCFNode;   // The node of the maximum confidence of this VID.

protected:
    bool combinationsHelper(int req_len, CTDIntArray& pos, int depth, int margin, CTDVirtualIDs& vids);

// attributes    
    CTDAttribs m_virtualAttribs;        // array of virtual attributes.
    CTDVirtualTreeNode m_vTreeNodeRoot; // Virtual tree root.
    CTDRollbackRecs m_rollbackRecs;     // Rollback records.
};


typedef CTypedPtrArray<CPtrArray, CTDVirtualID*> CTDVirtualIDArray;
class CTDVirtualIDs : public CTDVirtualIDArray
{
public:
    CTDVirtualIDs();
    virtual ~CTDVirtualIDs();
    void cleanup();

// operations
    bool initVIDs(LPCTSTR vidStr, CTDAttribs* pAttribs, int defaultK);
    CString toString() const;
    static bool parseFirstVID(CString& firstVID, CString& restStr);
};

#endif
