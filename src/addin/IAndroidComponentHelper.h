/*
 *          Warning!!!
 *       DO NOT ALTER THIS FILE!
 */

#ifndef __IANDROIDCOMPONENTHELPER_H__
#define __IANDROIDCOMPONENTHELPER_H__

#include "types.h"

#if defined(__ANDROID__)
#include <jni.h>

struct IAndroidComponentHelper : 
    public IInterface
{
    virtual jobject ADDIN_API GetActivity() = 0;

    virtual jclass ADDIN_API FindClass(const WCHAR_T* className) = 0;
};

#endif //__ANDROID__

#endif //__IANDROIDCOMPONENTHELPER_H__
