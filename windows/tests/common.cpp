#include "pch.h"
#include "common.h"
#include <nlohmann/json.hpp>
#include <codecvt>
#include <locale>
#include <fstream>
#include <sys/stat.h>

using json = nlohmann::json;

static string confFile;
static u16string _qname;

static string getConfFile() {
    if (confFile.empty()) {
        string fname = "test.conf";
        string path1[] = { "", "../", "../../" };
        string path2[] = { "", "tests/", "windows/tests/" };
        for (auto p1 : path1) {
            for (auto p2 : path2) {
                string p = p1 + p2 + fname;
                struct stat st;
                if (stat(p.c_str(), &st) == 0) {
                    confFile = p;
                    break;
                }
            }
            if (!confFile.empty()) {
                break;
            }
        }
    }
    return confFile;
}

static json loadConfig(bool amqps) {
    ifstream ifs(getConfFile());
    json val = json::parse(ifs);
    if (amqps && val.contains("amqps")) {
        return val["amqps"];
    }
    return val["amqp"];
}

u16string qname() {
    return _qname;
}


bool connect(Connection& conn, bool ssl, u16string password) {
    tVariant paParams[8];
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    json conf = loadConfig(ssl);
    _qname = conv.from_bytes(conf["queue"]);
    u16string host = conv.from_bytes(conf["host"]);
    u16string user = conv.from_bytes(conf["user"]);
    u16string pswd = password.empty() ?  conv.from_bytes(conf["pwd"]) : password;
    u16string vhost = conv.from_bytes(conf["vhost"]);
    conn.stringParam(&paParams[0], host);
    conn.intParam(&paParams[1], conf["port"]);
    conn.stringParam(&paParams[2], user);
    conn.stringParam(&paParams[3], pswd);
    conn.stringParam(&paParams[4], vhost);
    conn.intParam(&paParams[5], 0);
    conn.boolParam(&paParams[6], conf["secure"]);
    conn.intParam(&paParams[7], 5);
    return conn.callAsProc(u"Connect", paParams, 8);
}

bool makeQueue(Connection& conn, u16string name, u16string props) {
    delQueue(conn, name);
    tVariant paParams[7];
    conn.stringParam(paParams, name);
    conn.boolParam(&paParams[1], false);
    conn.boolParam(&paParams[2], true);
    conn.boolParam(&paParams[3], false);
    conn.boolParam(&paParams[4], false);
    conn.intParam(&paParams[5], 0);
    props.length() ? conn.stringParam(&paParams[6], props) : conn.nullParam(&paParams[6]);
    tVariant ret;
    bool res =  conn.callAsFunc(u"DeclareQueue", &ret, paParams, 7);
    if (res) {
        ASSERT(ret.vt == VTYPE_PWSTR);
        conn.retString(&ret);
    }
    return res;
}

bool delQueue(Connection& conn, u16string name) {
    tVariant paParams[3];
    conn.stringParam(paParams, name);
    conn.boolParam(&paParams[1], false);
    conn.boolParam(&paParams[2], false);
    return conn.callAsProc(u"DeleteQueue", paParams, 3);
}

u16string lastError(Connection& conn) {
    tVariant err;
    conn.callAsFunc(u"GetLastError", &err, nullptr, 0);
    u16string ret = conn.retString(&err);
    return ret;
}

bool makeExchange(Connection& conn, u16string ename, u16string props) {
    delExchange(conn, ename);
    tVariant paParams[6];
    u16string type = u"topic";
    conn.stringParam(paParams, ename);
    conn.stringParam(&paParams[1], type);
    conn.boolParam(&paParams[2], false);
    conn.boolParam(&paParams[3], true);
    conn.boolParam(&paParams[4], false);
    props.length() ? conn.stringParam(&paParams[5], props) : conn.nullParam(&paParams[5]);
    return conn.callAsProc(u"DeclareExchange", paParams, 6);
}

bool delExchange(Connection& conn, u16string qname) {
    tVariant paParams[2];
    conn.stringParam(paParams, qname);
    conn.boolParam(&paParams[1], false);
    return conn.callAsProc(u"DeleteExchange", paParams, 2);
}

bool bindQueue(Connection& conn, u16string name, u16string props) {
    ASSERT(makeQueue(conn, name));
    ASSERT(makeExchange(conn, name));
    tVariant paParams[4];
    u16string rkey = u"#";
    conn.stringParam(paParams, name);
    conn.stringParam(&paParams[1], name);
    conn.stringParam(&paParams[2], rkey);
    props.length() ? conn.stringParam(&paParams[3], props) : conn.nullParam(&paParams[3]);
    return conn.callAsProc(u"BindQueue", paParams, 4);
}

bool publish(Connection& conn, u16string qname, u16string msg, u16string props, bool noBind) {
    if (!noBind) {
        ASSERT(bindQueue(conn, qname));
    }
    tVariant paParams[6];
    conn.stringParam(&paParams[0], qname);
    conn.stringParam(&paParams[1], qname);
    conn.stringParam(&paParams[2], msg);
    conn.intParam(&paParams[3], 0);
    conn.boolParam(&paParams[4], false);
    props.length() ? conn.stringParam(&paParams[5], props) : conn.nullParam(&paParams[5]);
    return conn.callAsProc(u"BasicPublish", paParams, 6);
}

u16string basicConsume(Connection& conn, u16string queue, int size) {
    tVariant paParams[6];
    conn.stringParam(paParams, queue);
    u16string tag;
    conn.stringParam(&paParams[1], tag);
    conn.boolParam(&paParams[2], false);
    conn.boolParam(&paParams[3], false);
    conn.intParam(&paParams[4], size);
    conn.nullParam(&paParams[5]);
    tVariant ret;
    bool res = conn.callAsFunc(u"BasicConsume", &ret, paParams, 6);
    if (!res) {
        return u"";
    }
    return conn.retString(&ret);
}


u16string receiveUntil(Connection& conn, u16string qname, u16string msg, long* msgTag) {
    u16string tag = basicConsume(conn, qname);
    tVariant args[4];
    tVariant status;
    conn.stringParam(args, tag);
    conn.intParam(&args[3], 10000);
    u16string ret;
    while (conn.callAsFunc(u"BasicConsumeMessage", &status, args, 4) && status.bVal) {
        ret = conn.retString(&args[1]);
        if (ret == msg) {
            if (msgTag) {
                *msgTag = static_cast<long>(args[2].dblVal);
            }
            return ret;
        }
        conn.callAsProc(u"BasicAck", &args[2], 1);
    }
    return u"";
}

bool nSleep(Connection& conn, int timeout) {
    tVariant paParams[1];
    conn.intParam(paParams, timeout);
    return conn.callAsProc(u"SleepNative", paParams, 1);
}