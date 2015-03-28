//---------------------------------------------------------------------------
// File:
//      BFStrHelper.cpp BFStrHelper.hpp
//
// Module:
//      CBFStrHelper
//
// History:
//		Jan. 15, 2004		Created by Benjamin Fung
//---------------------------------------------------------------------------

#include "BFPch.h"

#if !defined(BFSTRHELPER_H)
	#include "BFStrHelper.h"
#endif

//--------------------------------------------------------------------
//--------------------------------------------------------------------
CBFStrHelper::CBFStrHelper()
{
}

CBFStrHelper::~CBFStrHelper()
{
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void CBFStrHelper::trim(CString& str)
{
    str.TrimLeft();
    str.TrimRight();
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
bool CBFStrHelper::isNumeric(const CString& str)
{
    int len = str.GetLength();
    for (int i = 0; i < len; ++i) {
        switch (str[i])
        {
        case '0': case '1': case '2': case '3': case '4': case '5': 
        case '6': case '7': case '8': case '9': case '.':
          //do nothing
          break;
        default:
          return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------
// 0x45F3D is a hex
// 0x45F3Dwarf is not a hex
// x45F3D is not a hex
// 45F3D is a hex
//--------------------------------------------------------------------
//bool CBFStrHelper::isHex(LPCTSTR str)
//{
//	DWORD_PTR dw = NULL;
//	TCHAR ch;
//	return (1 == swscanf_s(str, _T("%x%c"), &dw, &ch));
//}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void CBFStrHelper::intToStr(int i, CString& str)
{
	char buffer[64];
	_itoa_s(i, buffer, 10);
    if (!buffer)
        str = _T("");
    else
        str = buffer;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void CBFStrHelper::floatToStr(double d, int nDecimals, CString& str)
{
    char buffer[100];
    sprintf_s (buffer, "%.*f", nDecimals, d);
    if (!buffer)
        str = _T("");
    else
        str = buffer;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
int CBFStrHelper::strToInt(const TCHAR* string)
{
    return _tstoi(string);
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
double CBFStrHelper::strToFloat(const TCHAR* string)
{
    return _tstof(string);
}