// TDRecord.cpp: implementation of the CTDRecord class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif

//************
// CTDRecord *
//************

CTDRecord::CTDRecord()
    : m_recordID(0)
{
}

CTDRecord::~CTDRecord() 
{
    m_values.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDRecord::addValue(CTDValue* pValue)
{
    try {
        m_values.Add(pValue);
        return true;
    }
    catch (CMemoryException&) {
        ASSERT(false);
        return false;
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDRecord::toString(bool bIncludeNonVA, CTDAttribs* pAttribs) const
{
    CString str;
    CTDValue* pValue = NULL;
    int nValues = m_values.GetSize();
    for (int v = 0; v < nValues; ++v) {
        pValue = m_values.GetAt(v);
        if (bIncludeNonVA) {
            // Allow specialize on non virtual attribute.
            // Thus, write the generalized value.
            str += pValue->toString(false);
        }
        else {
            if (pAttribs) {
                // Need to check whether this attribute is a virtual attribute.
                str += pValue->toString(!pAttribs->GetAt(v)->m_bVirtualAttrib);
            }
            else {
                str += pValue->toString(true);
            }
        }

        if (v == nValues - 1)
            str += TD_RAWDATA_TERMINATOR;
        else
            str += TD_RAWDATA_DELIMETER;
    }
    return str;
}

//---------------------------------------------------------------------------
// An SVM record format is:
// <class> a1:v1 a2:v2, e.g.,
// +1 1:30.2 2:3.4
//---------------------------------------------------------------------------
CString CTDRecord::toSVMString(CTDAttribs* pAttribs) const
{
    // Construct <class>.    
    CTDValue* pValue = NULL;
    int nValues = m_values.GetSize();
    pValue = m_values.GetAt(nValues - 1);
    CString str = pValue->toSVMClassString();

    // Construct each value
    CString valStr;
    LPTSTR tmpStr = NULL;
    for (int v = 0; v < nValues - 1; ++v) {
        pValue = m_values.GetAt(v);
        if (pAttribs->GetAt(v)->isContinuous()) {
            // At this moment, continuous attribute cannot be virtual attribute.
            ASSERT(!pAttribs->GetAt(v)->m_bVirtualAttrib);
            valStr = pValue->toSVMString(true);
        }
        else {
#ifndef _TD_C45_SVM_DATA_CONVERTER
            // Need to check whether this attribute is a virtual attribute.
            valStr = pValue->toSVMString(!pAttribs->GetAt(v)->m_bVirtualAttrib);
#else
            valStr = pValue->toSVMString(true);
#endif
        }

        if (valStr == TD_SVM_RAWDATA_SKIPVALUE)
            continue;

        CString tmpStr;
        CBFStrHelper::intToStr(v + 1, tmpStr);
        str += TD_SVM_RAWDATA_DELIMETER;
        str += tmpStr;
        str += TD_SVM_RAWDATA_ATTVAL_SEPARATOR;
        str += valStr;
    }
    return str;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDRecord& record)
{
#ifdef _DEBUG_PRT_INFO
    os << _T("[") << record.m_recordID << _T("]\t") << record.toString(true, NULL);
#endif
    return os;
}

//*************
// CTDRecords *
//*************
CTDRecords::CTDRecords()
{
}

CTDRecords::~CTDRecords()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDRecords::cleanup()
{
    int nRecs = GetSize();
    for (int i = 0; i < nRecs; ++i)
        delete GetAt(i);

    RemoveAll();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDRecords::sortByAttrib(int attribIdx)
{
    bool ret = quickSort(attribIdx, 0, GetSize() - 1);
#if 0 //def _DEBUG_PRT_INFO
    for (int r = 0; r < GetSize(); ++r) {
        cout << _T("RecordID #") << GetAt(r)->getRecordID() << _T(" ") << getNumericValue(r, attribIdx) << endl;
    }
#endif
    return ret;
}

//---------------------------------------------------------------------------
// Quick sort records by attrib index.
//---------------------------------------------------------------------------
bool CTDRecords::quickSort(int attribIdx, int left, int right)
{
    int p = 0;
    float pivot = 0.0f;
    if (findPivot(attribIdx, left, right, pivot)) {
        p = partition(attribIdx, left, right, pivot);
        if (!quickSort(attribIdx, left, p - 1)) {
            ASSERT(false);
            return false;
        }
        if (!quickSort(attribIdx, p, right)) {
            ASSERT(false);
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
float CTDRecords::getNumericValue(int recIdx, int attribIdx)
{
    return (static_cast<CTDNumericValue*> (GetAt(recIdx)->getValue(attribIdx)))->getRawValue();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDRecords::findPivot(int attribIdx, int left, int right, float& pivot)
{
    pivot = 0.0f;
    // left value
    float a = getNumericValue(left, attribIdx);
    // middle value
    float b = getNumericValue(left + (right - left) / 2, attribIdx);
    // right value
    float c = getNumericValue(right, attribIdx);
    // order these 3 values
    orderNumbers(a, b, c);

    if (a < b) {
        pivot = b;
        return true;
    }

    if (b < c) {
        pivot = c;
        return true;
    }

    float pValue = 0.0f, leftValue = 0.0f;
    int p = left + 1;
    while (p <= right) {
        pValue = getNumericValue(p, attribIdx);
        leftValue = getNumericValue(left, attribIdx);
        if (pValue != leftValue) {
            if (pValue < leftValue)
                pivot = leftValue;
            else
                pivot = pValue;
            return true;
        }
        ++p;
    }
    return false;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int CTDRecords::partition(int attribIdx, int left, int right, float pivot) 
{
    while (left <= right) {
        while (getNumericValue(left, attribIdx) < pivot)
            ++left;

        while (getNumericValue(right, attribIdx) >= pivot)
            --right;

        if (left < right) {
            swapRecord(left, right);
            ++left;
            --right;
        }
    }
    return left;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDRecords::swapRecord(int recIdxA, int recIdxB)
{
    CTDRecord* pTemp = GetAt(recIdxA);
    SetAt(recIdxA, GetAt(recIdxB));
    SetAt(recIdxB, pTemp);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDRecords& records)
{
#ifdef _DEBUG_PRT_INFO
    CTDRecord* pRecord = NULL;
    os << _T("# of records = ") << records.GetSize() << endl;
    if (records.GetSize() > 0) {
        os << *(records.GetAt(0)) << endl;
    }
    /*
    for (int r = 0; r < records.GetSize(); ++r) {        
        pRecord = records.GetAt(r);
        os << *pRecord << endl;
    }
    */
#endif
    return os;
}

