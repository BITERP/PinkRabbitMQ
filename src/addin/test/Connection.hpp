#pragma once

#include <addin/types.h>
#include <addin/IMemoryManager.h>
#include <addin/AddInDefBase.h>
#include <addin/ComponentBase.h>
#include <addin/biterp/Error.hpp>
#include <addin/IAndroidComponentHelper.h>
#include <string>
#include <exception>
#include <stdexcept>
#include <vector>
#include <codecvt>
#include <locale>

#if !defined(ASSERT)
#error Include AndroidTest.hpp or WindowsCppUnit.hpp before Connection.hpp
#endif

#if !defined(ANDROID_ANDROIDTEST_HPP)
class AndroidHelper {
};
#endif

using namespace std;

namespace Biterp {
    namespace Test {


        class MemManager : public IMemoryManager {
        public:
            virtual bool ADDIN_API AllocMemory(void **pMemory, unsigned long ulCountByte) override {
                *pMemory = new uint8_t[ulCountByte];
                return true;
            }

            virtual void ADDIN_API FreeMemory(void **pMemory) override {
                if (!*pMemory) {
                    return;
                }
                uint8_t *buf = static_cast<uint8_t *>(*pMemory);
                delete[] buf;
                *pMemory = nullptr;
            }

        };


        class Connection : public IAddInDefBaseEx {
        public:

            class TestError : public Biterp::Error {
                using Biterp::Error::Error;
            };

            Connection(u16string className = u"") : raiseErrors(false), addin(nullptr) {
                init(className);
            }

            Connection(Connection &) = delete;

            Connection(Connection &&) = delete;

            virtual ~Connection() {
                done();
            }

            bool ADDIN_API
            AddError(unsigned short wcode, const WCHAR_T *source, const WCHAR_T *descr,
                     long scode) override {
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
                errors.push_back(conv.to_bytes((char16_t *) descr));
                if (raiseErrors) {
                    throw TestError(conv.to_bytes((char16_t *) descr));
                }
                return true;
            }

            virtual bool ADDIN_API
            Read(WCHAR_T *wszPropName, tVariant *pVal, long *pErrCode,
                 WCHAR_T **errDescriptor) override {
                return false;
            }

            virtual bool ADDIN_API
            Write(WCHAR_T *wszPropName, tVariant *pVar) override {
                return false;
            };

            virtual bool ADDIN_API
            RegisterProfileAs(WCHAR_T *wszProfileName) override {
                return false;
            };

            virtual bool ADDIN_API
            SetEventBufferDepth(long lDepth) override { return false; };

            virtual long ADDIN_API
            GetEventBufferDepth() override { return 0; };

            virtual bool ADDIN_API
            ExternalEvent(WCHAR_T *wszSource, WCHAR_T *wszMessage, WCHAR_T *wszData) override {
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
                std::string ev = conv.to_bytes(wszSource);
                ev += ":" + conv.to_bytes(wszMessage);
                ev += ":" + conv.to_bytes(wszData);
                events.push_back(ev);
                return true;
            };

            virtual void ADDIN_API
            CleanEventBuffer() override {};

            virtual bool ADDIN_API
            SetStatusLine(WCHAR_T *wszStatusLine) override {
                return false;
            };

            virtual void ADDIN_API
            ResetStatusLine() override {};

            virtual IInterface *ADDIN_API
            GetInterface(Interfaces iface) override {
                if (iface == eIAndroidComponentHelper) {
                    static AndroidHelper helper;
                    return (IInterface *) &helper;
                }
                return nullptr;
            }

            void init(u16string className) {
                GetClassObject(className.c_str(), &addin);
                addin->Init(this);
                addin->setMemManager(&memManager);
            }

            void done() {
                addin->Done();
                DestroyObject(&addin);
                addin = nullptr;
            }

            //-- native addin checkers

            long methodNum(std::u16string name, const long argsCnt, bool isFunc) {
                long ret = addin->FindMethod((WCHAR_T *)
                                                     name.c_str());
                const WCHAR_T *wnameBuf = addin->GetMethodName(ret, 0);
                u16string wname = (char16_t *) wnameBuf;
                memManager.FreeMemory((void **) &wnameBuf);
                //check that ru methodname does not crashes
                wnameBuf = addin->GetMethodName(ret, 1);
                memManager.FreeMemory((void **) &wnameBuf);

                ASSERT_EQ(name, wname);
                ASSERT_EQ(argsCnt, addin->GetNParams(ret));
                ASSERT_EQ(isFunc, addin->HasRetVal(ret));
                return ret;
            }

            long propNum(std::u16string name, bool readable = true, bool checkWriteable = false) {
                long ret = addin->FindProp((WCHAR_T *)
                                                   name.c_str());
                const WCHAR_T *wnameBuf = addin->GetPropName(ret, 0);
                u16string wname = (char16_t *) wnameBuf;
                memManager.FreeMemory((void **) &wnameBuf);
                // check ru propname
                wnameBuf = addin->GetPropName(ret, 1);
                memManager.FreeMemory((void **) &wnameBuf);

                ASSERT_EQ(name, wname);
                ASSERT_EQ(addin->IsPropReadable(ret), readable);
                if (checkWriteable) {
                    ASSERT_EQ(addin->IsPropWritable(ret), true);
                }
                return ret;
            }


            bool callAsProc(std::u16string name, tVariant *params, long pCnt) {
                long meth = methodNum(name, pCnt, false);
                ASSERT(meth >= 0);
                return addin->CallAsProc(meth, params, pCnt);
            }

            bool callAsFunc(std::u16string name, tVariant *ret, tVariant *params, long pCnt) {
                long meth = methodNum(name, pCnt, true);
                ASSERT(meth >= 0);
                return addin->CallAsFunc(meth, ret, params, pCnt);
            }

            bool getPropVal(std::u16string name, tVariant *ret) {
                long pNum = propNum(name);
                ASSERT(pNum >= 0);
                return addin->GetPropVal(pNum, ret);
            }

            bool setPropVal(std::u16string name, tVariant* val) {
                long pNum = propNum(name);
                ASSERT(pNum >= 0);
                return addin->SetPropVal(pNum, val);
            }

            //-- params filling

            void stringParam(tVariant *p, u16string &val) {
                p->vt = VTYPE_PWSTR;
                p->pwstrVal = const_cast<WCHAR_T *>((WCHAR_T *)
                        val.c_str());
                p->wstrLen = (uint32_t) val.length();
            }

            void boolParam(tVariant *p, bool val) {
                p->vt = VTYPE_BOOL;
                p->bVal = val;
            }

            void intParam(tVariant *p, int val) {
                p->vt = VTYPE_I4;
                p->lVal = val;
            }

            void longParam(tVariant *p, long long val) {
                p->vt = VTYPE_I8;
                p->llVal = val;
            }

            void nullParam(tVariant *pVariant) {
                pVariant->vt = VTYPE_EMPTY;
            }

            u16string retString(tVariant *variant) {
                u16string ret;
                ASSERT_EQ((int) variant->vt, (int) VTYPE_PWSTR);
                ret = u16string((char16_t *) variant->pwstrVal, variant->wstrLen);
                memManager.FreeMemory((void **) &variant->pwstrVal);
                return ret;
            }

            string retStringUtf8(tVariant *variant) {
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
                return conv.to_bytes(retString(variant));
            }

            bool hasError(string text, bool throwOnFail = true) {
                for (auto s : errors) {
                    if (s.find(text) != string::npos) {
                        return true;
                    }
                }
                if (throwOnFail) {
                    throw TestError("No error with text: " + text);
                }
                return false;
            }

            bool testDefaultParams(std::u16string name, int nondefaultParamsCount) {
                int meth = addin->FindMethod(name.c_str());
                ASSERT(meth >= 0);
                int params = addin->GetNParams(meth);
                ASSERT(params >= nondefaultParamsCount);
                for (int i = 0; i < params; i++) {
                    tVariant p;
                    p.vt = VTYPE_ILLEGAL;
                    bool res = addin->GetParamDefValue(meth, i, &p);
                    ASSERT(p.vt != VTYPE_EMPTY);
                    if (res != (i >= nondefaultParamsCount)) {
                        throw TestError("Default param wrong retval. meth: ") << meth << " proc: "
                                                                              << i;
                    }
                    if ((p.vt != VTYPE_ILLEGAL) != res) {
                        throw TestError("Default param not changed. meth: ") << meth << " proc: "
                                                                             << i;
                    }
                }
                return true;
            }


        public:
            IComponentBase *addin;
            MemManager memManager;
            bool raiseErrors;
            vector<string> errors;
            vector<string> events;
        };

    }

}