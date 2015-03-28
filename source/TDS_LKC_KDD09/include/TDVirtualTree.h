// TDVirtualTree.h: interface for the CTDVirtualTree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDVIRTUALTREE_H)
#define TDVIRTUALTREE_H

class CTDRollbackRecs;
class CTDVirtualTreeNode;

typedef CTypedPtrList<CPtrList, CTDVirtualTreeNode*> CTDVirtualTreeNodeList;
class CTDVirtualTreeNodes : public CTDVirtualTreeNodeList
{
public:
    CTDVirtualTreeNodes();
    virtual ~CTDVirtualTreeNodes();
    void cleanup();   
};

class CTDVirtualTreeNode
{
public:
    CTDVirtualTreeNode(CTDVirtualTreeNode* pParentNode, int flattenConceptIdx);
    virtual ~CTDVirtualTreeNode();

    int getCount() { return m_count; };
    float getMaxConf() { return m_maxConf; };
    void updateCount(int c) { m_count += c; }; 
    void updateSensCounts(const CTDIntArray& sc);

    bool addChildNode(CTDVirtualTreeNode* pChildNode);
    void removeChildNode(POSITION childPos);
      
    CTDVirtualTreeNode* getVPathNode(const CTDIntArray& vPath); 
    bool updateVPathCount(const CTDIntArray& vPath, int diff, const CTDIntArray& sensCounts, CTDRollbackRecs* pRollbackLog);

    bool searchMinAC_MaxCFNode(CTDVirtualTreeNode*& pMinNode, CTDVirtualTreeNode*& pMaxNode);

protected:
    CTDVirtualTreeNode* getVPathNodeHelper(const CTDIntArray& vPath, int vIdx);
    bool updateVPathCountHelper(const CTDIntArray& vPath, int vIdx, int diff, const CTDIntArray& sensCounts, CTDRollbackRecs* pRollbackLog);
    bool searchMinAC_MaxCFNodeHelper(CTDVirtualTreeNode*& pMinNode, CTDVirtualTreeNode*& pMaxNode);
    void deleteZeroBranch();

// attributes
    CTDVirtualTreeNode* m_pParentNode;
    POSITION m_vTreeChildPos;

    CTDVirtualTreeNodes m_childNodes;
    int m_flattenConceptIdx;
    int m_count;
    float m_maxConf;
    CTDIntArray m_sensCounts;
};

#endif
