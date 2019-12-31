
#ifndef __CONVERSIONWCHAR_H__
#define __CONVERSIONWCHAR_H__

#include "../include/types.h"

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);

class WcharWrapper
{
public:

    WcharWrapper(const WCHAR_T* str);
    WcharWrapper(const wchar_t* str);
    ~WcharWrapper();
    operator const WCHAR_T*() { return m_str_WCHAR; }
    operator WCHAR_T*() { return m_str_WCHAR; }
    operator const wchar_t*() { return m_str_wchar; }
    operator wchar_t*() { return m_str_wchar; }
private:
    WcharWrapper& operator = (const WcharWrapper& other) { return *this; };
    WcharWrapper(const WcharWrapper& other) {};
private:

    WCHAR_T* m_str_WCHAR;
    wchar_t* m_str_wchar;
};

#endif //__CONVERSIONWCHAR_H__
