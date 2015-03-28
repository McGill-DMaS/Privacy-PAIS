// TDMain.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#if !defined(TDMAIN_H)
    #include "TDMain.h"
#endif

#if !defined(TDCONTROLLER_H)
    #include "TDController.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

//using namespace std;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool parseArgs(int      nArgs, 
               TCHAR*   argv[], 
               CString& dataSetName,
               bool&    bRemoveUnknownOnly,
               bool&    bIncludeNonVA, 
               int&     defaultL,
               int&     defaultK,
               float&   defaultC,
               int&     nInputRecs,
               int&     nTraining)
{
    if (nArgs != 9 || !argv) {
        cout << _T("Usage: PrivacyTD <dataSetName> <bRemoveUnknownOnly> <bIncludeNonVA> <defaultL> <defaultK> <defaultC> <nInputRecs> <nTraining>") << endl;
        return false;
    }

    dataSetName = argv[1];

	if (_tcsicmp(argv[2], _T("TRUE")) == 0)
	    bRemoveUnknownOnly = true;
	else if (_tcsicmp(argv[2], _T("FALSE")) == 0)
		bRemoveUnknownOnly = false;
	else
        return false;

	if (_tcsicmp(argv[3], _T("TRUE")) == 0)
	    bIncludeNonVA = true;
	else if (_tcsicmp(argv[3], _T("FALSE")) == 0)
		bIncludeNonVA = false;
	else
        return false;

    defaultL = StrToInt(argv[4]);
    defaultK = StrToInt(argv[5]);
    defaultC = (float) CBFStrHelper::strToFloat(argv[6]);
    nInputRecs = CBFStrHelper::strToInt(argv[7]);
    nTraining = CBFStrHelper::strToInt(argv[8]);
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void debugPrint(LPCTSTR str)
{
    cout << str << endl;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void printTime()
{
    //CTime t = CTime::GetCurrentTime();
    //CString s = t.Format(_T("%H:%M:%S"));
    //DEBUGPrint(_T("Current System Time = %s\n"), s);

    time_t ltime;
    time(&ltime);
    cout << _T("Current System Time = ") << ltime << endl;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
long getTime()
{
    time_t ltime;
    time(&ltime);
    return (long) ltime;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
long get_runtime(void)
{
  clock_t start;
  start = clock();
  return((long)((double)start*100.0/(double)CLOCKS_PER_SEC));
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDIntArray& intAry)
{
#ifdef _DEBUG_PRT_INFO
    os << _T("[");
    for (int i = 0; i < intAry.GetSize(); ++i) {
        os << intAry.GetAt(i);
        if (i != intAry.GetSize() - 1)
            os << _T(" ");
    }
    os << _T("]");
#endif
    return os;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
		CString strHello;
		strHello.LoadString(IDS_HELLO);
		//cout << (LPCTSTR)strHello << endl;

        CString dataSetName;
        int defaultK = 0, defaultL = 0, nTraining = 0, nInputRecs = 0;
        float defaultC = 0.0f;
        bool bRemoveUnknownOnly = false, bIncludeNonVA = false;

        if (!parseArgs(argc, argv, dataSetName, bRemoveUnknownOnly, bIncludeNonVA, defaultL, defaultK, defaultC, nInputRecs, nTraining)) {
		    cerr << _T("Input Error: invalid arguments") << endl;
		    return 1;
        }
        
        // Construct the filenames
        CString rawDataFile, attributesFile, nameFile, supFile, transformedDataFile, transformedTestFile, transformedSVMDataFile, transformedSVMTestFile;
        rawDataFile = dataSetName;
        rawDataFile += _T(".");
        rawDataFile += TD_RAWDATAFILE_EXT;
        attributesFile = dataSetName;
        attributesFile += _T(".");
        attributesFile += TD_ATTRBFILE_EXT;
        nameFile = dataSetName;
        nameFile += _T(".");
        nameFile += TD_NAMEFILE_EXT;
        supFile = dataSetName;
        supFile += _T(".");
        supFile += TD_SUPFILE_EXT;
        transformedDataFile = dataSetName;
        transformedDataFile += _T(".");
        transformedDataFile += TD_TRANSFORM_DATAFILE_EXT;
        transformedTestFile = dataSetName;
        transformedTestFile += _T(".");
        transformedTestFile += TD_TRANSFORM_TESTFILE_EXT;
        transformedSVMDataFile = dataSetName;
        transformedSVMDataFile += _T(".");
        transformedSVMDataFile += TD_TRANSFORM_SVM_DATAFILE_EXT;
        transformedSVMTestFile = dataSetName;
        transformedSVMTestFile += _T(".");
        transformedSVMTestFile += TD_TRANSFORM_SVM_TESTFILE_EXT;

        CTDController controller(rawDataFile, 
                                 attributesFile,
                                 nameFile,
                                 supFile,
                                 transformedDataFile, 
                                 transformedTestFile,
                                 transformedSVMDataFile, 
                                 transformedSVMTestFile,
                                 bIncludeNonVA, 
                                 defaultL,
                                 defaultK,
                                 defaultC,
                                 nInputRecs,
                                 nTraining);

        if (bRemoveUnknownOnly) {
            if (!controller.removeUnknowns()) {
                cerr << _T("Error occured.") << endl;
                return 1;
            }
        }
        else {
            if (!controller.runTopDown()) {
                cerr << _T("Error occured.") << endl;
                return 1;
            }
        }
	}

    cout << _T("Bye!") << endl;
    //DEBUGPrint(_T("Bye!\n"));
#ifdef _DEBUG
    _getch();
#endif
	return nRetCode;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
float calEntropy(CTDIntArray* pArray)
{    
    int c = 0;
    int total = 0;
    int sz = pArray->GetSize();
    for (c = 0; c < sz; ++c)
        total += pArray->GetAt(c);

    //cout << "***" << endl;
    float r = 0.0f;
    float entropy = 0.0f;
    for (c = 0; c < sz; ++c) {
        r = float(pArray->GetAt(c)) / total;
        //cout << pArray->GetAt(c) << endl;
        if (r > 0.0f)
            entropy += (r * log2f(r)) * -1;
    }
    return entropy;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void orderNumbers(float& a, float& b, float& c)
{
    if (a > b)
        swapNumbers(a, b);
    if (a > c)
        swapNumbers(a, c);
    if (b > c)
        swapNumbers(b, c);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void swapNumbers(float& a, float& b)
{
    float temp = b;
    b = a;
    a = temp;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void negateNumbers(const CTDIntArray& ary, CTDIntArray& res)
{
    res.RemoveAll();
    for (int s = 0; s < ary.GetSize(); ++s)
        res.Add(ary[s] * -1);
}