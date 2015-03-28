// TDRollbackRec.h: interface for the CTDRollbackRec class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDROLLBACKREC_H)
#define TDROLLBACKREC_H

#if !defined(TDVIRTUALTREE_H)
    #include "TDVirtualTree.h"
#endif

class CTDRollbackRec;

typedef CTypedPtrArray<CPtrArray, CTDRollbackRec*> CTDRollbackRecArray;
class CTDRollbackRecs : public CTDRollbackRecArray
{
public:
    CTDRollbackRecs() {};
    virtual ~CTDRollbackRecs() { cleanup(); };
    void cleanup();
};

class CTDRollbackRec
{
public:
    CTDRollbackRec(CTDVirtualTreeNode* pNode, int diff, const CTDIntArray& sensCounts);
    virtual ~CTDRollbackRec() {};

// attributes
    CTDVirtualTreeNode* m_pVTreeNode;
    int m_diff;
    CTDIntArray m_sensCounts;
};

#endif
