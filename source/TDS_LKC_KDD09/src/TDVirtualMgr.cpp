// TDVirtualMgr.cpp: implementation of the CTDVirtualMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDVIRTUALMGR_H)
    #include "TDVirtualMgr.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDVirtualMgr::CTDVirtualMgr(LPCTSTR vidsFile, int defaultL, int defaultK, float defaultC) 
    : m_vidsFile(vidsFile), m_defaultL(defaultL), m_defaultK(defaultK),  m_defaultC(defaultC)
{
    //ASSERT(m_pAttribMgr);
}

CTDVirtualMgr::~CTDVirtualMgr() 
{
    m_vids.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDVirtualMgr::initialize(CTDAttribMgr* pAttribMgr)
{
    if (!pAttribMgr) {
        ASSERT(false);
        return false;
    }
    m_pAttribMgr = pAttribMgr;
    return true;
}

//---------------------------------------------------------------------------
// Read VID attributes from the VIDs file.
//---------------------------------------------------------------------------
bool CTDVirtualMgr::readVIDandSensitive()
{
    cout << _T("Reading virtual identifiers...") << endl;
    m_vids.RemoveAll();
    m_sensConcepts.RemoveAll();
    try {
        CStdioFile vidFile;
        if (!vidFile.Open(m_vidsFile, CFile::modeRead)) {
            cerr << _T("CTDVirtualMgr: Failed to open file ") << m_vidsFile << endl;
            return false;
        }

        // Parse each line.        
        CString lineStr, attribName, attribType, valuesStr;
        int commentCharPos = -1, semiColonPos = -1;
        while (vidFile.ReadString(lineStr)) {
            CBFStrHelper::trim(lineStr);
            if (lineStr.IsEmpty())
                continue;

            // Remove comments.
            commentCharPos = lineStr.Find(TD_CONHCHY_COMMENT);
            if (commentCharPos != -1) {
                lineStr = lineStr.Left(commentCharPos);
                CBFStrHelper::trim(lineStr);
                if (lineStr.IsEmpty())
                    continue;
            }

            // Find semicolon.
            semiColonPos = lineStr.Find(TCHAR(':'));
            if (semiColonPos == -1) {
                cerr << _T("CTDVirtualMgr: Unknown line: ") << lineStr << endl;
                ASSERT(false);
                return false;
            }

            // Extract attribute name.
            attribName = lineStr.Left(semiColonPos);
            CBFStrHelper::trim(attribName);
            if (attribName.IsEmpty()) {
                cerr << _T("CTDVirtualMgr: Invalid attribute: ") << lineStr << endl;
                ASSERT(false);
                return false;
            }

            // Extract attribute type.
            attribType = lineStr.Mid(semiColonPos + 1);
            CBFStrHelper::trim(attribType);
            if (attribType.IsEmpty())
                attribType = TD_DISCRETE_ATTRIB;

            // Read the next line which contains the VID or Sensitive Concepts.
            if (!vidFile.ReadString(valuesStr)) {
                cerr << _T("CTDVirtualMgr: Invalid attribute: ") << attribName << endl;
                ASSERT(false);
                return false;
            }
            CBFStrHelper::trim(valuesStr);
            if (valuesStr.IsEmpty()) {
                cerr << _T("CTDVirtualMgr: Invalid attribute: ") << attribName << endl;
                ASSERT(false);
                return false;
            }

            // Remove comments.
            commentCharPos = valuesStr.Find(TD_CONHCHY_COMMENT);
            if (commentCharPos != -1) {
                valuesStr = valuesStr.Left(commentCharPos);
                CBFStrHelper::trim(valuesStr);
                if (valuesStr.IsEmpty()) {
                    cerr << _T("CTDVirtualMgr: Invalid attribute: ") << attribName << endl;
                    ASSERT(false);
                    return false;
                }
            }

            if (attribName.CompareNoCase(TD_VID_ATTRIB_NAME) == 0) {
                if (!m_vids.initVIDs(valuesStr, m_pAttribMgr->getAttributes(), m_defaultK)) {
                    cerr << _T("CTDVirtualMgr: Invalid VID: ") << valuesStr << endl;
                    return false;
                }
            }
            else if (attribName.CompareNoCase(TD_SENSITIVE_NAME) == 0) {
                if (!m_sensConcepts.initSensConcepts(valuesStr, m_pAttribMgr->getAttributes(), m_defaultC)) {
                    cerr << _T("CTDVirtualMgr: Invalid sensitive concepts: ") << valuesStr << endl;
                    return false;
                }
            }
        }
        vidFile.Close();
    }
    catch (CFileException&) {
        cerr << _T("Failed to read VIDs file: ") << m_vidsFile << endl;
        ASSERT(false);
        return false;
    }
    cout << _T("Reading virtual identifiers succeeded.") << endl;
    return true;
}

//---------------------------------------------------------------------------
// Break a single VID into subset combinations with length L.
// e.g., If L=3, {A B C D} becomes:
// {A,B,C} {A,B,D}, {A,C,D}, {B,C,D}
//---------------------------------------------------------------------------
bool CTDVirtualMgr::breakIntoCombinations()
{
    if (m_defaultL > 0) {
        cout << _T("Breaking virtual identifiers into subsets...") << endl;
        if (m_vids.GetSize() != 1) {
            cerr << _T("CTDVirtualMgr: breakIntoCombinations. If L>0, then one VID only. Now, there are ") << m_vids.GetSize() << endl;
            ASSERT(false);
            return false;
        }

        CTDVirtualIDs brokenVIDs;
        if (!m_vids[0]->combinations(m_defaultL, brokenVIDs))
            return false;

        m_vids.cleanup();
        m_vids.Append(brokenVIDs);
        cout << m_vids.toString();
        cout << _T("Breaking virtual identifiers into subsets succeeded.") << endl;
    }
    return true;
}