//
// Created by ipalenov on 18.12.2020.
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <string>
#include <iostream>
#include "../AddInDefBase.h"
#include "MemoryManager.hpp"
#include "Error.hpp"
#include "CallContext.hpp"
#include <codecvt>
#include <locale>
#include "Logger.hpp"

#define NATIVE_ERROR    1006

#define DO_QUOTE(...) #__VA_ARGS__
#define QUOTE(X)  "" DO_QUOTE(X)

#define LOGD(M) getLogger().debug(M)
#define LOGI(M) getLogger().info(M)
#define LOGW(M) getLogger().warning(M)
#define LOGE(M) getLogger().error(M)


namespace Biterp {

    /**
    * Base class for component
    */
    class Component {
    public:
        Component(const char *className) : addin(nullptr), skipAddError(false) {
            this->className = u16Converter.from_bytes(className);
            this->version = QUOTE(VERSION);
        }

        virtual ~Component() {}

        /**
        * Initialize component..
        * @param addin
        * @return
        */
        virtual bool init(IAddInDefBase *addin) {
            logger = Biterp::Logging::getLogger(u16Converter.to_bytes(className), version, addin, this);
            LOGD("init");
            this->addin = addin;
            return true;
        }

        /**
        * Deinitialize component.
        */
        virtual void done() {
            LOGD("done");
        }

        inline void setMemoryManager(IMemoryManager *manager) { memManager.setHandle(manager); }

        inline MemoryManager &memoryManager() { return memManager; }

    public:
        //---- common methods

        /**
        * Return lastError field
        * @param pvarRetValue
        * @param paParams
        * @param lSizeArray
        * @return
        */
        inline bool
        getLastError(tVariant *pvarRetValue, tVariant *paParams, const long lSizeArray) {
            return memManager.variantFromString(pvarRetValue, lastError);
        }

        void setLastError(std::u16string error) { lastError = error; }

        /**
        * Return component version
        * @param pvarRetValue
        * @return
        */
        inline bool getVersion(tVariant *pvarRetValue) {
            std::u16string uver = u16Converter.from_bytes(version);
            return memManager.variantFromString(pvarRetValue, uver);
        }

        inline const Biterp::Logging::Logger& getLogger(){ return logger; }

    protected:
        //---- utils

        /**
        * Raise 1c exception utf16.
        * @param descr  - exception message.
        * @param source - exception source.
        * @param wcode - internal code.
        * @param scode - hresult.
        */
        void addError(const std::u16string &descr, std::u16string source = u"",
                      unsigned short wcode = NATIVE_ERROR,
                      long scode = E_FAIL) {
            setLastError(descr);
            if (skipAddError) {
                return;
            }
            if (!source.length()) {
                source = className;
            }
            if (addin) {
                addin->AddError(wcode, source.c_str(), descr.c_str(), scode);
            }
        }

        /**
        * Raise 1c exception utf8.
        * @param descr
        * @param source
        * @param wcode
        * @param scode
        */
        void addError(const std::string &descr, const std::string &source = "",
                      unsigned short wcode = NATIVE_ERROR,
                      long scode = E_FAIL) {
            std::u16string wdescr;
            std::u16string wsource;
            try {
                wdescr = u16Converter.from_bytes(descr);
                wsource = u16Converter.from_bytes(source);
            }
            catch (const std::exception &) {
                wdescr = u"Invalid error message";
            }
            addError(move(wdescr), move(wsource), wcode, scode);
        }

        /**
         * Do not call addin->addError on exceptions. Just set lastError
         * @param skip
         */
        void setSkipAddError(bool skip = true) { skipAddError = skip; }


    protected:
        /**
         * Template function to call implementation.
         * Rethrows java errors.
         * @tparam T - proxy object type
         * @tparam Proc - proxy object method type
         * @param obj - proxy object pointer
         * @param proc - method pointer
         * @param paParams - input params array
         * @param lSizeArray - size of input params array
         * @param pvarRetValue - return value or nullptr
         * @return
         */
        template<typename T, typename Proc>
        bool wrapCall(T *obj, Proc proc, tVariant *paParams, const long lSizeArray,
                      tVariant *pvarRetValue = nullptr) {
            bool result = false;
            try {
                skipAddError = false;
                lastError.clear();
                CallContext ctx(memManager, paParams, lSizeArray, pvarRetValue);
                (obj->*proc)(ctx);
                result = true;
            }
            catch (std::exception &e) {
                std::string who = typeid(e).name();
                std::string what = e.what();
                LOGE(who + ": " + what);
                addError(what, who);
            }
            return result;
        }


    protected:
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16Converter;
        std::u16string className;
        std::string version;
        IAddInDefBase *addin;
        std::u16string lastError;
        MemoryManager memManager;
        bool skipAddError;
        Biterp::Logging::Logger logger;
    };

}

#endif //COMPONENT_HPP
