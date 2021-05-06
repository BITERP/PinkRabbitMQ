
#include "RabbitMQClientNative.h"

#pragma warning( disable : 4267)
#pragma warning( disable : 4311)
#pragma warning( disable : 4302)

#if defined(__ANDROID__)

#include <addin/jni/jnienv.h>

#endif //__ANDROID__

#if defined(__APPLE__) && !defined(BUILD_DYNAMIC_LIBRARY)

namespace ADD_IN_NATIVE
{

#endif //__APPLE__ && !BUILD_DYNAMIC_LIBRARY

static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;

//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T *wsName, IComponentBase **pInterface) {
    if (!*pInterface) {
        *pInterface = new RabbitMQClientNative();
        return (long) (*pInterface);
    }
    return 0;
}

//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities) {
    g_capabilities = capabilities;
    return eAppCapabilitiesLast;
}

//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase **pInterface) {
    if (!*pInterface)
        return -1;

    delete *pInterface;
    *pInterface = nullptr;
    return 0;
}

//---------------------------------------------------------------------------//
const WCHAR_T *GetClassNames() {
    return RabbitMQClientNative::componentName;
}

#if defined(__ANDROID__)
JavaVM *JNI::sJavaVM = nullptr;

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *aJavaVM, void *aReserved) {
    JNI::setVM(aJavaVM);
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *aJavaVM, void *aReserved) {
    JNI::setVM(nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_biterp_util_Log_logNative(JNIEnv *env, jclass clazz, jint level, jstring text) {
    const char *chars = env->GetStringUTFChars(text, nullptr);
    Biterp::Logger::log((int) level, chars);
    env->ReleaseStringUTFChars(text, chars);
}

#endif


#if defined(__APPLE__) && !defined(BUILD_DYNAMIC_LIBRARY)

};

namespace ADD_IN_NATIVE
{

    static LPCVOID addin_exports[] =
    {
        "GetClassObject", (LPCVOID)GetClassObject,
        "DestroyObject", (LPCVOID)DestroyObject,
        "GetClassNames", (LPCVOID)GetClassNames,
        "SetPlatformCapabilities", (LPCVOID)SetPlatformCapabilities,
        NULL
    };

    DECLARE_DLL((const char*)g_ComponentName, addin_exports);
}

#endif //__APPLE__ && !BUILD_DYNAMIC_LIBRARY
