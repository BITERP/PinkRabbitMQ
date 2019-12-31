
#include "ConversionWchar.h"
#include <wchar.h>

//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
    if (!len)
        len = static_cast<uint32_t>(::wcslen(Source) + 1);

    if (!*Dest)
        *Dest = new WCHAR_T[len];

    WCHAR_T* tmpShort = *Dest;
    wchar_t* tmpWChar = (wchar_t*)Source;
    uint32_t res = 0;

    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpShort = (WCHAR_T)*tmpWChar;
    }

    return res;
}

//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
    if (!len)
        len = getLenShortWcharStr(Source) + 1;

    if (!*Dest)
        *Dest = new wchar_t[len];

    wchar_t* tmpWChar = *Dest;
    WCHAR_T* tmpShort = (WCHAR_T*)Source;
    uint32_t res = 0;

    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpWChar = (wchar_t)*tmpShort;
    }

    return res;
}

//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
    uint32_t res = 0;
    WCHAR_T *tmpShort = (WCHAR_T*)Source;

    while (*tmpShort++)
        ++res;

    return res;
}
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) :

m_str_WCHAR(NULL),

m_str_wchar(NULL)
{
    if (str)
    {
        int len = static_cast<int>(wcslen(str));
        m_str_wchar = new wchar_t[len + 1];
        memset(m_str_wchar, 0, sizeof(wchar_t)* (len + 1));
        memcpy(m_str_wchar, str, sizeof(wchar_t)* len);
        
        convToShortWchar(&m_str_WCHAR, m_str_wchar, len + 1);
       

    }
}

//---------------------------------------------------------------------------//

WcharWrapper::WcharWrapper(const WCHAR_T* str) : m_str_WCHAR(NULL),
m_str_wchar(NULL)
{
    if (str)
    {
        int len = getLenShortWcharStr(str);
        m_str_WCHAR = new WCHAR_T[len + 1];
        memset(m_str_WCHAR,   0, sizeof(WCHAR_T) * (len + 1));
        memcpy(m_str_WCHAR, str, sizeof(WCHAR_T) * len);
        convFromShortWchar(&m_str_wchar, m_str_WCHAR);
    }
}

//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
    if (m_str_WCHAR)
    {
        delete[] m_str_WCHAR;
        m_str_WCHAR = NULL;
    }

    
    if (m_str_wchar)
    {
        delete[] m_str_wchar;
        m_str_wchar = NULL;
    }
}
