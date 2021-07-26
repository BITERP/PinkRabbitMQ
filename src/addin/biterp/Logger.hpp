#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <thread>
#include <sys/stat.h>

#if defined(__ANDROID__)

#include <android/log.h>
#include <addin/IAndroidComponentHelper.h>
#include "../jni/jnienv.h"

#define UNLINK  unlink
#define LOCALTIME(V, T) tm V = *localtime(T)

#endif

#if defined(__linux__)

#include <unistd.h>
#include <pwd.h>

#define UNLINK  unlink
#define LOCALTIME(V, T) tm V = *localtime(T)

#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <Shlobj.h>

#define UNLINK  _unlink
#define LOCALTIME(V, T) tm V; localtime_s(&V, T)

#endif


using namespace std;

namespace Biterp {

    /**
     * Log to file and logcat. Rotate logs with debug filtering.
     */
    class Logger {
    public:
        enum Level {
            LDEBUG = 0,
            LINFO,
            LWARNING,
            LERROR,
            LFATAL
        };

    public:
        inline static void init(const u16string &name, IAddInDefBase *addin) {
            instance()._init(name, addin);
        }

        inline static void log(int level, const string &text) {
            instance()._log(level, text);
        }

        inline static void debug(const string &text) { log(Level::LDEBUG, text); }

        inline static void info(const string &text) { log(Level::LINFO, text); }

        inline static void warning(const string &text) { log(Level::LWARNING, text); }

        inline static void error(const string &text) { log(Level::LERROR, text); }

    private:
        // rotation size 2Mb
        const int ROTATE_SIZE = 2 * 1024 * 1024;
        const char *LEVELS = "DIWEF";

        /**
         * Set logs filename, open current logfile.
         * @param name
         * @param addin
         */
        void _init(const u16string &name, IAddInDefBase *addin) {
            if (filename.length()) {
                // already inited
                return;
            }
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
            this->name = getName(conv.to_bytes(name));
            filename = getFileName(addin);
            if (!filename.length()) {
                return;
            }
            file.open(filename, ios::out | ios::app | ios::ate);
        }

        /**
         * Save log to file. Multithread.
         * @param level
         * @param text
         */
        void _log(int level, const string &text) {
            logNative(level, text);
            std::lock_guard<std::mutex> lock(_mutex);
            if (!file) {
                return;
            }
            time_t t = time(nullptr);
            LOCALTIME(tm, &t);
            file << "[" << LEVELS[level] << std::put_time(&tm, " %Y-%m-%d %H:%M:%S ") 
                << std::time(nullptr) << " " << std::this_thread::get_id() << "]"
                << text << endl;
            file.flush();
            if (file.tellp() > ROTATE_SIZE) {
                rotate();
            }
        }

        /**
         * Rotate logs (current -> .1 -> .2 -> .3 -> .4). Filter debug messages from log.2
         */
        void rotate() {
            if (fileExists(4)) {
                UNLINK((filename + ".4").c_str());
            }
            for (int i = 3; i > 0; i--) {
                if (fileExists(i)) {
                    moveFile(i);
                }
            }
            file.close();
            moveFile(0);
            file.open(filename, ios::out);
            if (fileExists(2)) {
                thread th(filterLogs, filename + ".2");
                th.detach();
            }
        }

        /**
         * Copy file removeing D(ebug) lines.
         * @param filename
         */
        static void filterLogs(string filename) {
            string bak = filename + ".bak";
            std::rename(filename.c_str(), bak.c_str());
            ifstream in(bak);
            ofstream out(filename, ios::out);
            bool keep = true;
            string line;
            while (getline(in, line)) {
                if (line[0] == '[' && line[7] == '-') {
                    keep = line[1] != 'D';
                }
                if (keep) {
                    out << line << endl;
                }
            }
            in.close();
            out.close();
            UNLINK(bak.c_str());
        }


        /**
         * Get log filename with OS specific path.
         * @param addin
         * @return
         */
        string getFileName(IAddInDefBase *addin) {
#if defined(__ANDROID__)
            // return activity.getExternalFilesDir(null).getAbsolutePath() + "/logs/<name>.log"
            string path;
            IAddInDefBaseEx *addinex = static_cast<IAddInDefBaseEx *>(addin);
            IAndroidComponentHelper *helper = (IAndroidComponentHelper *) addinex->GetInterface(
                    eIAndroidComponentHelper);
            jobject activity = helper->GetActivity();
            JNIEnv *env = JNI::getEnv();
            jclass cls = env->GetObjectClass(activity);
            jmethodID meth = env->GetMethodID(cls, "getExternalFilesDir",
                                              "(Ljava/lang/String;)Ljava/io/File;");
            jobject jfile = env->CallObjectMethod(activity, meth, nullptr);
            env->DeleteLocalRef(cls);
            if (!jfile) {
                logNative(LERROR, "ExternalFilesDir failed for logger");
                return "";
            }
            cls = env->GetObjectClass(jfile);
            meth = env->GetMethodID(cls, "getAbsolutePath", "()Ljava/lang/String;");
            jstring jpath = (jstring) env->CallObjectMethod(jfile, meth);
            env->DeleteLocalRef(jfile);
            env->DeleteLocalRef(cls);
            if (!jpath) {
                logNative(LERROR, "AbsolutePath failed for logger");
            }
            const char *chars = env->GetStringUTFChars(jpath, nullptr);
            path = chars;
            env->ReleaseStringUTFChars(jpath, chars);
            env->DeleteLocalRef(jpath);
            if (path.length()) {
                path += "/logs";
                if (!pathExists(path)) {
                    mkdir(path.c_str(), 0755);
                }
                return path + "/" + name + ".log";
            }
#elif (defined(_WIN32) || defined(_WIN64))
            // create logfile in LOCAL_APPDATA/biterp/logs/<name>.log
            char buf[MAX_PATH];
            SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf);
            string path = buf;
            path += "/biterp";
            if (!pathExists(path)) {
                CreateDirectoryA(path.c_str(), NULL);
            }
            path += "/logs";
            if (!pathExists(path)) {
                CreateDirectoryA(path.c_str(), NULL);
            }
            return path + "/" + name + ".log";
#elif defined(__linux__)
            string path = getpwuid(getuid())->pw_dir;
            path+="/.biterp";
            if (!pathExists(path)) {
                mkdir(path.c_str(), 0755);
            }
            path += "/logs";
            if (!pathExists(path)) {
                mkdir(path.c_str(), 0755);
            }
            return path + "/" + name + ".log";
#else
#error "Unsupported OS"
#endif
            return name + ".log";
        }

        /**
         * Get log name
         * @param name
         * @return
         */
        string getName(string name) {
            string cut = "./\\:";
            for (char c: cut) {
                size_t pos = name.rfind(c);
                if (pos != string::npos) {
                    name = name.substr(pos + 1);
                }
            }
            return name;
        }

        /**
         * Duplicate log to OS native (logcat).
         * @param level
         * @param text
         */
        void logNative(int level, const string &text) {
#if defined(__ANDROID__)
            level += ANDROID_LOG_DEBUG - Level::LDEBUG;
            __android_log_write(level, name.c_str(), text.c_str());
#endif
        }

        inline bool fileExists(int num) {
            return pathExists(filename + "." + to_string(num));
        }

        bool pathExists(string path) {
            struct stat buffer;
            return (stat(path.c_str(), &buffer) == 0);
        }

        void moveFile(int from) {
            string src = filename + (from == 0 ? "" : ("." + to_string(from)));
            string dest = filename + "." + to_string(from + 1);
            std::rename(src.c_str(), dest.c_str());
        }

    private:
        string filename;
        string name;
        ofstream file;
        std::mutex _mutex;

    private:
        // Singleton
        static Logger &instance() {
            static Logger _inst;
            return _inst;
        }

        Logger() {}

        ~Logger() {
            if (file) {
                file.close();
            }
        }

        Logger(const Logger &) = delete;

        Logger(const Logger &&) = delete;

        void operator=(const Logger &) = delete;

        void operator=(const Logger &&) = delete;

    };
}


#endif //LOGGER_HPP