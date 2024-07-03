#include <addin/test/LinuxCTest.hpp>
#include <addin/test/Connection.hpp>

#include <map>
#include <string>
#include <memory>
#include <functional>
#include <list>
using namespace std;

#if defined(__linux__) || defined(__APPLE__)
#define EXPORT __attribute__ ((visibility ("default")))
#else
#define EXPORT __declspec(dllexport)
#endif

std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;


map<int, shared_ptr<Biterp::Test::Connection>> cons;
Biterp::Test::Connection* getcon(int id){
    if (!cons.count(id)){
        throw runtime_error("Connection not found " + to_string(id));
    }
    return cons[id].get();
}

struct Arguments{
    Arguments(){}
    Arguments(int con):con(con){}
    vector<tVariant> args;
    list<u16string> strings;
    int con = -1;
    tVariant* add(){ args.push_back(tVariant()); return &args[args.size()-1];}
    void addNull(){
        getcon(con)->nullParam(add());
    }
    void addInt(int val){
        getcon(con)->intParam(add(), val);
    }
    void addLong(long long val){
        getcon(con)->longParam(add(), val);
    }
    void addDouble(double val){
        getcon(con)->doubleParam(add(), val);
    }
    void addString(const char* val){
        strings.push_back(conv.from_bytes(val));
        getcon(con)->stringParam(add(), strings.back());
    }
    void addBool(bool val){
        getcon(con)->boolParam(add(), val);
    }
};

struct Result{
    Result():ret(){}
    Result(int con):con(con){}
    tVariant ret;
    int con = -1;
    int64_t intVal(){
        if (ret.vt==VTYPE_INT || ret.vt==VTYPE_I1 || ret.vt==VTYPE_I2 || ret.vt==VTYPE_I4 || ret.vt==VTYPE_I8)
            return ret.i8Val;
        throw runtime_error("Result is not int");
    }
    uint64_t uintVal(){
        if (ret.vt==VTYPE_UINT || ret.vt==VTYPE_UI1 || ret.vt==VTYPE_UI2 || ret.vt==VTYPE_UI4 || ret.vt==VTYPE_UI8)
            return ret.ui8Val;
        throw runtime_error("Result is not int");
    }
    bool boolVal(){
        if (ret.vt!=VTYPE_BOOL)
            throw runtime_error("Result is not bool");
        return ret.bVal;
    }
    double doubleVal(){
        if (ret.vt==VTYPE_R4)
            return ret.fltVal;
        if (ret.vt==VTYPE_R8)
            return ret.dblVal;
        if (ret.vt==VTYPE_DATE)
            return ret.date;
        throw runtime_error("Result is not double");
    }
};

map<int, Arguments> args;
map<int, Result> results;

string last_ex;

bool checked(std::function<void()> proc){
    try{
        proc();
        last_ex = "";
        return true;
    }catch(std::exception& ex){
        last_ex = ex.what();
    }catch(...){
        last_ex = "Unknown exception catched";
    }
    return false;
}

Arguments& getargs(int id){
    if (!args.count(id)){
        throw runtime_error("Arguments not found " + to_string(id));
    }
    return args[id];
}

Result& getres(int id){
    if (!results.count(id)){
        throw runtime_error("Result not found " + to_string(id));
    }
    return results[id];
}

int string_cpy(char* buf, int size, const string& data){
    int len = data.length() < size-1 ? data.length() : size-1;
    memcpy(buf, data.c_str(), len);
    buf[len]=0;
    return data.length()+1;
}

extern "C" {

    EXPORT int last_exception(char* buf, int size){
        return string_cpy(buf, size, last_ex);
    }

    EXPORT int create_connection(const char* name){
        int ret = -1;
        checked([&ret, name]{
            u16string uname = conv.from_bytes(name);
            int i = 1;
            while (cons.count(i)) i++;
            cons[i] = shared_ptr<Biterp::Test::Connection>(new Biterp::Test::Connection(uname));
            ret = i;
        });
        return ret;
    }

    EXPORT void delete_connection(int con){
        if (cons.count(con)){
            cons.erase(con);
        }
    }

    EXPORT void set_raise(int con, bool raise){
        checked([con, raise]{getcon(con)->raiseErrors = raise;});
    }

    EXPORT int create_arguments(int con){
        int ret = -1;
        checked([&ret, con]{
            getcon(con);
            int i = 1;
            while (args.count(i)) i++;
            args[i] = Arguments(con);
            ret = i;
        });
        return ret;
    }

    EXPORT int create_argument_result(int arg, int argid){
        int ret = -1;
        checked([&ret, arg, argid]{
            Arguments& a = getargs(arg);
            if (argid >= a.args.size()) {
                throw runtime_error("Argument id exceeds: " + to_string(argid));
            }
            int i = 1;
            while (results.count(i)) i++;
            results[i] = Result(a.con);
            results[i].ret = a.args[argid];
            ret = i;
        });
        return ret;
    }

    EXPORT void delete_arguments(int arg){
        if (args.count(arg)){
            args.erase(arg);
        }
    }

    EXPORT bool call_proc(int con, const char* proc, int arg){
        bool ret = false;
        checked([&ret, con, proc, arg]{
            u16string uproc = conv.from_bytes(proc);
            Arguments& a = getargs(arg);
            ret = getcon(con)->callAsProc(uproc, a.args.data(), a.args.size());
        });
        return ret;
    }

    EXPORT bool call_func(int con, const char* proc, int arg, int* result){
        bool ret = false;
        checked([&ret, con, proc, arg, result]{
            u16string uproc = conv.from_bytes(proc);
            Arguments& a = getargs(arg);
            int i = 1;
            while (results.count(i)) i++;
            results[i] = Result(con);
            *result = i;
            ret = getcon(con)->callAsFunc(uproc, &results[i].ret, a.args.data(), a.args.size());
        });
        return ret;
    }

    EXPORT bool set_prop(int con, const char* prop, int arg){
        bool ret = false;
        checked([&ret, con, prop, arg]{
            u16string uprop = conv.from_bytes(prop);
            Arguments& a = getargs(arg);
            ret = getcon(con)->setPropVal(uprop, &a.args[0]);
        });
        return ret;
    }

    EXPORT bool get_prop(int con, const char* prop, int* result){
        bool ret = false;
        checked([&ret, con, prop, result]{
            u16string uprop = conv.from_bytes(prop);
            int i = 1;
            while (results.count(i)) i++;
            results[i] = Result(con);
            *result = i;
            ret = getcon(con)->getPropVal(uprop, &results[i].ret);
        });
        return ret;
    }

    EXPORT void delete_result(int res){
        if (!results.count(res)){
            return;
        }
        checked([res]{
            Result& r = getres(res);
            getcon(r.con)->freeResult(&r.ret);
            results.erase(res);
        });
    }

    EXPORT void add_argument_int(int arg, int val){
        checked([arg, val]{getargs(arg).addInt(val);});
    }
    EXPORT void add_argument_long(int arg, long long val){
        checked([arg, val]{getargs(arg).addLong(val);});
    }
    EXPORT void add_argument_double(int arg, double val){
        checked([arg, val]{getargs(arg).addDouble(val);});
    }
    EXPORT void add_argument_str(int arg, const char* val){
        checked([arg, val]{getargs(arg).addString(val);});
    }
    EXPORT void add_argument_bool(int arg, bool val){
        checked([arg, val]{getargs(arg).addBool(val);});
    }
    EXPORT void add_argument_null(int arg){
        checked([arg]{getargs(arg).addNull();});
    }

    EXPORT int errornum(int con){
        int ret = 0;
        checked([con, &ret]{ret = getcon(con)->errors.size();});
        return ret;
    }

    EXPORT int get_error(int con, int error, char* data, int size){
        int ret = 0;
        checked([&ret, con, error, data, size]{
            string str = getcon(con)->errors[error];
            ret = string_cpy(data, size, str);
        });
        return ret;
    }

    EXPORT int get_result_type(int res){
        int ret = 0;
        checked([&ret, res]{
            ret = getres(res).ret.vt;
        });
        return ret;
    }

    EXPORT int64_t get_result_int(int res){
        int64_t ret = 0;
        checked([&ret, res]{
            ret = getres(res).intVal();
        });
        return ret;
    }

    EXPORT uint64_t get_result_uint(int res){
        uint64_t ret = 0;
        checked([&ret, res]{
            ret = getres(res).uintVal();
        });
        return ret;
    }

    EXPORT bool get_result_bool(int res){
        bool ret = false;
        checked([&ret, res]{
            ret = getres(res).boolVal();
        });        
        return ret;
    }

    EXPORT double get_result_double(int res){
        double ret = false;
        checked([&ret, res]{
            ret = getres(res).doubleVal();
        });        
        return ret;
    }

    EXPORT int get_result_str(int res, char* data, int size){
        int ret = 0;
        checked([&ret, res, data, size]{
            Result& r = getres(res);
            string str = getcon(r.con)->retStringUtf8(&r.ret, false);
            ret = string_cpy(data, size, str);
        });
        return ret;
    }

    EXPORT int eventnum(int con){
        int ret = 0;
        checked([con, &ret]{ret = getcon(con)->events.size();});
        return ret;
    }

    EXPORT int get_event(int con, int event, char* data, int size){
        int ret = 0;
        checked([&ret, con, event, data, size]{
            string str = getcon(con)->events[event];
            ret = string_cpy(data, size, str);
        });
        return ret;
    }

    EXPORT void clear_events(int con, int amount){
        checked([con, amount]{
            auto &ev = getcon(con)->events;
            for (int i=0; i<amount; ++i){
                if (ev.size()){
                    ev.erase(ev.begin());
                }
            }
        });
    }

    EXPORT void test_default_params(int con, const char* name, int count){
        checked([con, name, count]{
            u16string nm = conv.from_bytes(name);
            getcon(con)->testDefaultParams(nm, count);
        });
    }

};