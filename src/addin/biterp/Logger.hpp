#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sys/stat.h>
#include <filesystem>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std::chrono;
namespace fs = std::filesystem;

#if defined(__ANDROID__)

#include <android/log.h>
#include <addin/IAndroidComponentHelper.h>
#include "../jni/jnienv.h"

#define LOCALTIME(V, T) tm V = *localtime(T)

#endif

#if defined(__linux__)

#include <unistd.h>
#include <pwd.h>

#define GETPID  getpid 
#define LOCALTIME(V, T) tm V = *localtime(T)

#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <Shlobj.h>
#include <process.h>

#define GETPID  _getpid 
#define LOCALTIME(V, T) tm V; localtime_s(&V, T)

#endif


namespace Biterp {

    /**
     * Log to file and logcat. Rotate logs with debug filtering.
     */
    class Logging {

    public:
        struct Logger{
            enum Level {
                LDEBUG = 0,
                LINFO,
                LWARNING,
                LERROR,
                LFATAL
            };
            virtual ~Logger(){}
            std::string subname;
            std::string version;
            std::string instance;
            inline void debug(const std::string &text) const {Logging::debug(text, *this);}
            inline void info(const std::string &text) const {Logging::info(text, *this);}
            inline void warning(const std::string &text) const {Logging::warning(text, *this);}
            inline void error(const std::string &text) const {Logging::error(text, *this);}
        };


    public:
        inline static Logger getLogger(const std::string &name, const std::string &version, IAddInDefBase *addin, void* compinst) {
            instance()._init(addin, name, version);
            Logger logger;
            logger.subname = name;
            logger.version = version;
            uintptr_t instance = reinterpret_cast<uintptr_t>(compinst);
            logger.instance = std::to_string(instance);
            return logger;
        }

        Logging& getInstance(){
            return instance();
        }

        inline static void log(int level, const std::string &text, const Logger& logger) {
            return;
            instance()._log(level, text, logger);
        }

        inline static void debug(const std::string &text, const Logger& logger=defaultLogger()) { log(Logger::Level::LDEBUG, text, logger); }
        inline static void info(const std::string &text, const Logger& logger=defaultLogger()) { log(Logger::Level::LINFO, text, logger); }
        inline static void warning(const std::string &text, const Logger& logger=defaultLogger()) { log(Logger::Level::LWARNING, text, logger); }
        inline static void error(const std::string &text, const Logger& logger=defaultLogger()) { log(Logger::Level::LERROR, text, logger); }

        Logging& setAppName(const std::string& appname){
            std::lock_guard<std::mutex> lock(_mutex);
            record["Appname"] = appname;
            return *this;
        }
        Logging& setDeviceid(const std::string& deviceid){
            std::lock_guard<std::mutex> lock(_mutex);
            record["Deviceid"] = deviceid;
            return *this;
        }

        Logging& setClientid(const std::string& clientid){
            std::lock_guard<std::mutex> lock(_mutex);
            record["Clientid"] = clientid;
            return *this;
        }

        Logging& setLoglevel(const std::string& loglevel){
            std::lock_guard<std::mutex> lock(_mutex);
            for (int i=0;i<=Logger::Level::LFATAL; i++){
                if (levels[i] == loglevel){
                    minlevel = i;
                }
            }
            return *this;
        }

    private:
        static constexpr int CLEAN_INTERVAL = 600;
        static constexpr int KEEP_TIME = 60 * 10;
        static constexpr char FILE_FMT[] = "%Y-%m-%d-%H-%M";
        static constexpr char ISO_FMT[] = "%FT%H:%M:%S";
        static constexpr char PREFIX[] = "comc1c";

        /**
         * Set logs filename, open current logfile.
         * @param name
         * @param addin
         */
        void _init(IAddInDefBase *addin, const std::string& name, const std::string& version) {
            if (!_fname.empty()) {
                // already inited
                return;
            }
            defaultLogger().subname = name;
            defaultLogger().version = version;
            uint32_t pid = (uint32_t)GETPID();
            _path = getFilePath(addin);
            _fname = _path + PREFIX + std::to_string(pid);
        }

        std::string formatTime(const char* fmt, const std::tm* tm, int ms=-1){
            std::ostringstream oss;
            oss << std::put_time(tm, fmt);
            if (ms>=0){
                oss << '.' << std::setfill('0') << std::setw(3) << ms;
            }
            return oss.str();
        }


        /**
         * Save log to file. Multithread.
         * @param level
         * @param text
         */
        void _log(int level, const std::string &text, const Logger& logger) {
            logNative(level, text);
            if (level < minlevel || _fname.empty()){
                return;
            }

            auto now = system_clock::now();
            auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            time_t timer = system_clock::to_time_t(now);
            LOCALTIME(tm, &timer);
            std::lock_guard<std::mutex> lock(_mutex);
            std::ofstream& file = getFile(formatTime(FILE_FMT, &tm));
            if (!file) {
                return;
            }
            std::string line = buildRecord(logger, text, level, formatTime(ISO_FMT, &tm, (int)ms.count()));
            file << line << std::endl;
            file.flush();
            if ((int)duration_cast<seconds>(now - cleanTime).count() < CLEAN_INTERVAL){
                return;
            }
            cleanTime = now;
            cleanOld(_path);
        }

        std::string buildRecord(const Logger& logger, const std::string message, int level, const std::string time){
            record["Subsystemname"] = logger.subname;
            record["Version"] = logger.version;
            record["Instance"] = logger.instance;
            record["Description"] = message;
            record["Loglevel"] = levels[level];
            record["Date"] = time;
            return record.dump();
        }

        void cleanOld(std::string path){
            try{
                auto now = system_clock::now();
                std::tm tm = {};
                std::error_code err;
                if (path.empty() || !fs::exists(path, err)){
                    return;
                }
                for (const auto & entry : fs::directory_iterator(path)){
                    if (!entry.is_regular_file() || entry.path().extension() != ".txt"){
                        continue;
                    }
                    std::string nm = entry.path().stem().string();
                    if (nm.find(Logging::PREFIX) != 0){
                        continue;
                    }
                    size_t pos = nm.find("-");
                    if (pos == std::string::npos){
                        continue;
                    }
                    std::string dt = nm.substr(pos + 1);
                    if (dt.length() != 16 || dt[4]!='-' || dt[7]!='-' || dt[10]!='-' || dt[13]!='-'){
                        continue;
                    }
                    std::istringstream ss(dt);
                    ss >> std::get_time(&tm, Logging::FILE_FMT);
                    if (ss.fail()){
                        continue;
                    }
                    auto diff = now - system_clock::from_time_t(mktime(&tm));
                    if (duration_cast<seconds>(diff).count() > KEEP_TIME){
                        fs::remove(entry.path(), err);
                    }
                }
            }catch(...){
            }
        }

        /**
         * Get log filename with OS specific path.
         * @param addin
         * @return
         */
        std::string getFilePath(IAddInDefBase *addin) {
            std::string path;
#if defined(__ANDROID__)
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
#elif (defined(_WIN32) || defined(_WIN64))
            // create logfile in LOCAL_APPDATA/biterp/logs/<name>.log
            char buf[MAX_PATH];
            SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf);
            path = buf;
            if (path.empty()){
                return "./";
            }
            path += "/biterp";
#elif defined(__linux__)
            path = getpwuid(getuid())->pw_dir;
            if (path.empty()){
                return "./";
            }
            path += "/.biterp";
#else
#error "Unsupported OS"
#endif
            if (path.empty()){
                return "./";
            }
            path += "/logs";
            fs::create_directories(path);
            return path + "/";
        }

        /**
         * Duplicate log to OS native (logcat).
         * @param level
         * @param text
         */
        void logNative(int level, const std::string &text) {
#if defined(__ANDROID__)
            level += ANDROID_LOG_DEBUG - Level::LDEBUG;
            __android_log_write(level, name.c_str(), text.c_str());
#endif
        }

        std::ofstream& getFile(const std::string& date){
            time_t tm = time(nullptr);
            std::string fname = _fname + "-" + date + ".txt";
            if (fname == _current_file && _file){
                return _file;
            }
            if (_file){
                _file.close();
            }
            _file.open(fname, std::ios::out | std::ios::app | std::ios::ate);
            _current_file = fname;
            return _file;
        }

    private:
        std::string _path;
        std::string _fname;
        std::string _current_file;
        std::ofstream _file;
        std::mutex _mutex;
        system_clock::time_point cleanTime;
        json record;
        int minlevel;
        std::vector<std::string> levels;

    private:
        // Singleton
        inline static Logging &instance() {
            static Logging _inst;
            return _inst;
        }

        inline static Logger &defaultLogger() {
            static Logger _defaultLogger;
            return _defaultLogger;
        }

        Logging(): minlevel(0) , levels{"D","I","W","E","F"}{
            record = json({
                {"Appname", ""},
                {"Subsystemname", ""},
                {"Version", ""},
                {"Description", ""},
                {"Loglevel", ""},
                {"Date", ""},
                {"Deviceid", ""},
                {"Clientid", ""},
                {"Instance", ""},
            });
        }

        ~Logging() {
            if (_file) {
                _file.close();
            }
        }

        Logging(const Logging &) = delete;

        Logging(const Logging &&) = delete;

        void operator=(const Logging &) = delete;

        void operator=(const Logging &&) = delete;

    };

}


#endif //LOGGER_HPP