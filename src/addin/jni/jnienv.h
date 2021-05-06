#ifndef __JNIENV_H__
#define __JNIENV_H__

///////////////////////////////////////////////////////////////////////////////

#include <jni.h>
#include <android/log.h>

class JNI {
public:
    static void trace(const char *format, ...) {
#if !defined(NDEBUG)
        va_list args;
        va_start(args, format);
        __android_log_vprint(ANDROID_LOG_DEBUG, "AddInNative", format, args);
        va_end(args);
#endif
    }

    static JNIEnv *getEnv() {

        JNIEnv *env = NULL;

        switch (sJavaVM->GetEnv((void **) &env, JNI_VERSION_1_6)) {
            case JNI_OK:
                return env;

            case JNI_EDETACHED: {
                JavaVMAttachArgs args;
                args.name = NULL;
                args.group = NULL;
                args.version = JNI_VERSION_1_6;

                if (!sJavaVM->AttachCurrentThreadAsDaemon(&env, &args)) {
                    trace("AttachCurrentThreadAsDaemon(), env = %08X", env);
                    return env;
                }
                break;
            }

        }
        return NULL;
    };

    inline static void setVM(JavaVM *vm) { sJavaVM = vm; }

private:
    static JavaVM *sJavaVM;
};

///////////////////////////////////////////////////////////////////////////////

#endif //__JNIENV_H__