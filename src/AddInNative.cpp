
#include "RabbitMQClientNative.h"
#include <addin/ComponentBase.h>

#pragma warning( disable : 4267)
#pragma warning( disable : 4311)
#pragma warning( disable : 4302)

#if defined(__ANDROID__)

#include <addin/jni/jnienv.h>

#endif //__ANDROID__

#if defined(__linux__)
#define EXPORT __attribute__ ((visibility ("default")))
#else
#define EXPORT
#endif


#if defined(__APPLE__) && !defined(BUILD_DYNAMIC_LIBRARY)

namespace ADD_IN_NATIVE
{

#endif //__APPLE__ && !BUILD_DYNAMIC_LIBRARY

static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;

//---------------------------------------------------------------------------//
EXPORT long GetClassObject(const WCHAR_T *wsName, IComponentBase **pInterface) {
    if (!*pInterface) {
        *pInterface = new RabbitMQClientNative();
        return (long) (*pInterface);
    }
    return 0;
}

//---------------------------------------------------------------------------//
EXPORT AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities) {
    g_capabilities = capabilities;
    return eAppCapabilitiesLast;
}

//---------------------------------------------------------------------------//
EXPORT long DestroyObject(IComponentBase **pInterface) {
    if (!*pInterface)
        return -1;

    delete *pInterface;
    *pInterface = nullptr;
    return 0;
}

//---------------------------------------------------------------------------//
EXPORT const WCHAR_T *GetClassNames() {
    return RabbitMQClientNative::componentName;
}

EXPORT AttachType GetAttachType(){
    return AttachType::eCanAttachAny;
}

#if defined(__ANDROID__)

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
        "GetAttachType", (LPCVOID)GetAttachType,
        "GetClassNames", (LPCVOID)GetClassNames,
        "SetPlatformCapabilities", (LPCVOID)SetPlatformCapabilities,
        NULL
    };

    DECLARE_DLL((const char*)g_ComponentName, addin_exports);
}

#endif //__APPLE__ && !BUILD_DYNAMIC_LIBRARY
