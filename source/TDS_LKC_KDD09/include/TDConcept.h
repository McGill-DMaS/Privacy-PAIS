// TDConcept.h: interface for the CTDConcept class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDCONCEPT_H)
#define TDCONCEPT_H

class CTDConcept;
class CTDAttribs;

typedef CTypedPtrArray<CPtrArray, CTDConcept*> CTDConceptPtrArray;
class CTDConcepts : public CTDConceptPtrArray
{
public:
    CTDConcepts();
    virtual ~CTDConcepts();
    void cleanup();

// operations
    CTDConcept* findConcept(LPCTSTR target);
    bool initSensConcepts(LPCTSTR senStr, CTDAttribs* pAttribs, float defaultC);
    static bool parseFirstSensitive(float& cThreshold, CString& firstSAttrib, CString& firstSConcept, CString& restStr);
    static bool parseThresholdAttribConcept(CString firstSens, float& cThreshold, CString& firstSAttrib, CString& firstSConcept);
};

class CTDPartition;
class CTDPartitions;
class CTDAttrib;
class CTDRecords;

class CTDConcept
{
public:
    CTDConcept(CTDAttrib* pAttrib);
    virtual ~CTDConcept();

    virtual bool isContinuous() = 0;
    virtual bool initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxChildren) = 0;
    virtual bool divideConcept(int nClasses) = 0;
    virtual CString toString() = 0;
    
    bool addChildConcept(CTDConcept* pConceptNode);
    int getNumChildConcepts() const;
    CTDConcept* getChildConcept(int idx) const;
    CTDConcept* getParentConcept();
    CTDAttrib* getAttrib() { return m_pAttrib; };

    CTDPartitions* getRelatedPartitions();
    POSITION registerPartition(CTDPartition* pPartition);
    void deregisterPartition(POSITION pos);

    bool computeInfoGain(int nClasses);
    static bool computeInfoGainHelper(float entropy, 
                                      const CTDIntArray& supSums, 
                                      const CTDIntArray& classSums, 
                                      CTDMDIntArray& supMatrix,
                                      float& infoGainDiff);

    static float computeEntropy(CTDIntArray* pClassSums);
    static bool parseFirstConcept(CString& firstConcept, CString& restStr);
        
// attributes
    CString        m_conceptValue;          // Actual value in string format.
    int            m_depth;                 // Depth of this concept.
    int            m_childIdx;              // Child index in concept hierarchy.
    int            m_flattenIdx;            // Flattened index in concept hierarchy.
    float          m_infoGain;              // Information gain
    float          m_infoAnoyRatio;         // IG/AL ratio of this concept.
    float          m_cThreshold;            // Confidence threshold.
    bool           m_bSensConcept;          // Is it a sensitive concept?
    bool           m_bCutCandidate;         // Can it be a cut candidate?
    POSITION       m_cutPos;                // Position of this concept in the cut.    

protected:
// operations
    bool initSplitMatrix(int nConcepts, int nClasses);
    virtual bool findOptimalSplitPoint(CTDRecords& recs, int nClasses) = 0;
    virtual bool makeChildConcepts() = 0;

// attributes
    CTDAttrib*     m_pAttrib;               // Pointer to this attribute.
    CTDConcept*    m_pParentConcept;
    CTDConcepts    m_childConcepts;
    CTDPartitions* m_pRelatedPartitions;    // related leaf partitions.

    CTDMDIntArray* m_pSplitSupMatrix;   // raw count of supports
    CTDIntArray    m_splitSupSums;      // sum of supports of each concept
    CTDIntArray    m_splitClassSums;    // sum of classes
};


class CTDDiscConcept : public CTDConcept
{
public:
    CTDDiscConcept(CTDAttrib* pAttrib);
    virtual ~CTDDiscConcept();

    virtual bool isContinuous() { return false; };
    virtual bool initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches);
    virtual bool divideConcept(int nClasses);
    virtual CString toString();

    static bool parseConceptValue(LPCTSTR str, CString& conceptVal, CString& restStr);

protected:
    virtual bool findOptimalSplitPoint(CTDRecords& recs, int nClasses);
    virtual bool makeChildConcepts();

// attributes
    CTDConcept* m_pSplitConcept;            // pointer to the winner concept.
};


class CTDContConcept : public CTDConcept
{
public:
    CTDContConcept(CTDAttrib* pAttrib);
    virtual ~CTDContConcept();
    virtual bool isContinuous() { return true; };
    virtual bool initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches);
    virtual bool divideConcept(int nClasses);
    virtual CString toString();
    
    float getSplitPoint() { return m_splitPoint; };

    static bool makeRange(float lowerB, float upperB, CString& range);
    static bool parseConceptValue(LPCTSTR  str, 
                                  CString& conceptVal, 
                                  CString& restStr,
                                  float&   lowerBound,
                                  float&   upperBound);
    static bool parseLowerUpperBound(const CString& str, 
                                     float& lowerB, 
                                     float& upperB);

// attributes
    float m_lowerBound;     // inclusive
    float m_upperBound;     // exclusive

protected:
// operations
    virtual bool findOptimalSplitPoint(CTDRecords& recs, int nClasses);
    virtual bool makeChildConcepts();
    bool computeSplitEntropy(float& entropy);

// attributes
    float m_splitPoint;     // Split point of this concept.
};

#endif
