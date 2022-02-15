#include <addin/test/LinuxCTest.hpp>
#include <addin/test/Connection.hpp>
#include <chrono>
#include "common.h"
#include <nlohmann/json.hpp>
#include <ctime>
#inculde <time.h>
#include <Utils.h>


using namespace Biterp::Test;
using json = nlohmann::json;

CTEST(Connect)
{
	Connection con;
	con.raiseErrors = true;
	ASSERT(connect(con));
}

CTEST(FailConnect)
{
    Connection con;
    con.raiseErrors = false;
    ASSERT(!connect(con ,false, u"password"));
    con.hasError("Wrong login, password or vhost");
}


CTEST(ConnectSsl)
{
	Connection con;
	con.raiseErrors = true;
	ASSERT(connect(con, true));
}

CTEST(DefParams)
{
	Connection con;
	con.testDefaultParams(u"Connect", 5);
	con.testDefaultParams(u"DeclareQueue", 5);
	con.testDefaultParams(u"DeclareExchange", 5);
	con.testDefaultParams(u"BasicPublish", 5);
	con.testDefaultParams(u"BindQueue", 3);
}

CTEST(Publish) {
    Connection conn;
    connect(conn);
    ASSERT(publish(conn, qname(), u"tset msg"));
}


CTEST(DeclareExchange) {
    Connection conn;
    connect(conn);
    ASSERT(makeExchange(conn, qname()));
}

CTEST(DeclareQueue) {
    Connection conn;
    connect(conn);
    ASSERT(makeQueue(conn, qname()));
}

CTEST(BindQueue) {
    Connection conn;
    connect(conn);
    conn.raiseErrors = true;
    ASSERT(bindQueue(conn, qname()));
}

CTEST(UnbindQueue) {
    Connection conn;
    connect(conn);
    tVariant paParams[3];
    u16string qn = qname();
    ASSERT(bindQueue(conn, qn));
    u16string rkey = u"#";
    conn.stringParam(paParams, qn);
    conn.stringParam(&paParams[1], qn);
    conn.stringParam(&paParams[2], rkey);
    ASSERT(conn.callAsProc(u"UnbindQueue", paParams, 3));
}

CTEST(DeleteQueue) {
    Connection conn;
    connect(conn);
    ASSERT(delQueue(conn, qname()));
}

CTEST(DeleteExchange) {
    Connection conn;
    connect(conn);
    ASSERT(delExchange(conn, qname()));
}

CTEST(BasicAck) {
    Connection conn;
    connect(conn);
    u16string msg = u"msgack test";
    ASSERT(publish(conn, qname(), msg));
    long tag;
    u16string got = receiveUntil(conn, qname(), msg, &tag);
    ASSERT(got == msg);
    tVariant paParams[1];
    conn.longParam(paParams, tag);
    ASSERT(conn.callAsProc(u"BasicAck", paParams, 1));
}

CTEST(BasicNack) {
    Connection conn;
    connect(conn);
    u16string msg = u"msgnack test";
    ASSERT(publish(conn, qname(), msg));
    long tag;
    u16string got = receiveUntil(conn, qname(), msg, &tag);
    ASSERT(got == msg);
    tVariant paParams[1];
    conn.longParam(paParams, 1);
    ASSERT(conn.callAsProc(u"BasicReject", paParams, 1));
}

CTEST(BasicConsume) {
    Connection conn;
    connect(conn);
    bindQueue(conn, qname());
    u16string tag = basicConsume(conn, qname());
    ASSERT(tag.length() > 0);
}

CTEST(BasicConsumeNoMessage) {
    Connection conn;
    connect(conn);
    u16string qn = qname();
    bindQueue(conn, qn);
    u16string tag = basicConsume(conn, qn);
    tVariant args[4];
    tVariant ret;
    conn.stringParam(args, tag);
    conn.intParam(&args[3], 100);
    bool res = conn.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
    while (ret.bVal) {
        res = conn.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
    }
    ASSERT(res);
    ASSERT(ret.vt == VTYPE_BOOL);
    ASSERT(!ret.bVal);
    ASSERT(args[1].vt == VTYPE_EMPTY);
    ASSERT(args[2].vt == VTYPE_R8);
    ASSERT(args[2].dblVal == 0);
}

CTEST(BasicCancel) {
    Connection conn;
    connect(conn);
    u16string tag = basicConsume(conn, qname());
    ASSERT(tag.length() > 0);
    tVariant arg[1];
    conn.stringParam(arg, tag);
    ASSERT(conn.callAsProc(u"BasicCancel", arg, 1));
}

CTEST(BasicConsumeReceive) {
    Publish_impl();
    Connection conn;
    connect(conn);
    u16string tag = basicConsume(conn, qname());
    tVariant args[4];
    tVariant ret;
    conn.stringParam(args, tag);
    conn.intParam(&args[3], 10000);
    bool res = conn.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
    ASSERT(res);
    ASSERT(ret.vt == VTYPE_BOOL);
    ASSERT(ret.bVal);
    ASSERT(args[1].vt == VTYPE_PWSTR);
    u16string msg = conn.retString(&args[1]);
    ASSERT(msg.length() > 0);
}

CTEST(Version) {
    Connection conn;
    tVariant ver;
    ASSERT(conn.getPropVal(u"Version", &ver));
    ASSERT(ver.vt == VTYPE_PWSTR);
    string v = conn.retStringUtf8(&ver);
    cout << " " << v << " ";
}

CTEST(SetMessageProp) {
    Connection conn;
    connect(conn);
    tVariant val;
    u16string value = u"MY_CORRELATION_ID";
    conn.stringParam(&val, value);
    ASSERT(conn.setPropVal(u"CorrelationId", &val));
    ASSERT(publish(conn, qname(), u"msgprop test"));
}

CTEST(GetMessageProp) {
    SetMessageProp_impl();
    Connection conn;
    connect(conn);
    u16string text = receiveUntil(conn, qname(), u"msgprop test");
    ASSERT(text == u"msgprop test");
    tVariant ret;
    ASSERT(conn.getPropVal(u"CorrelationId", &ret));
    u16string res = conn.retString(&ret);
    ASSERT(res == u"MY_CORRELATION_ID");
}

CTEST(SetPriority) {
    Connection conn;
    connect(conn);
    tVariant args[1];
    conn.intParam(args, 13);
    ASSERT(conn.callAsProc(u"SetPriority", args, 1));
    ASSERT(publish(conn, qname(), u"msgpriority test"));
}

CTEST(GetPriority) {
    SetPriority_impl();
    Connection conn;
    connect(conn);
    u16string text = receiveUntil(conn, qname(), u"msgpriority test");
    ASSERT(text == u"msgpriority test");
    tVariant ret;
    ASSERT(conn.callAsFunc(u"GetPriority", &ret, nullptr, 0));
    ASSERT(ret.vt == VTYPE_I4);
    ASSERT(ret.lVal == 13);
}


CTEST(ExtBadjson) {
    Connection con;
    con.raiseErrors = false;
    ASSERT(connect(con));
    ASSERT(!makeQueue(con, qname(), u"Not Json Object Param"));
    con.hasError("syntax error");
}

CTEST(ExtGoodParam) {
    Connection con;
    ASSERT(connect(con));
    ASSERT(makeQueue(con, qname(), u"{\"x-message-ttl\":60000}"));
}

CTEST(ExtExchange) {
    Connection con;
    ASSERT(connect(con));
    ASSERT(makeExchange(con, qname(), u"{\"alternate-exchange\":\"myae\"}"));
}

CTEST(ExtBind) {
    Connection con;
    ASSERT(connect(con));
    ASSERT(bindQueue(con, qname(), u"{\"x-message-ttl\":60000}"));
}

CTEST(ExtPublish) {
    Connection con;
    ASSERT(connect(con));
    ASSERT(publish(con, qname(), u"args message", u"{\"some-header\":13,\"yes\":true,\"no\":false}"));
    u16string text = receiveUntil(con, qname(), u"args message");
    tVariant ret;
    ASSERT(con.callAsFunc(u"GetHeaders", &ret, nullptr, 0));
    ASSERT(ret.vt == VTYPE_PWSTR);
    json obj = json::parse(con.retStringUtf8(&ret));
    ASSERT_EQ(13, (int)obj["some-header"]);
    ASSERT_EQ(true, (bool)obj["yes"]);
    ASSERT_EQ(false, (bool)obj["no"]);
}

CTEST(MultiConnectSsl) {
    Connection con;
    con.raiseErrors = true;
    for (int i = 0; i < 10; i++) {
        ASSERT(connect(con, true));
    }
}

CTEST(NoCancel) {
    Connection con;
    con.raiseErrors = true;
    for (int i = 0; i < 5; i++) {
        ASSERT(connect(con));
        for (int j = 0; j < i; j++) {
            ASSERT(publish(con, qname(), u"args message", u"{\"some-header\":13}"));
        }
        u16string tag = basicConsume(con, qname());
        tVariant args[4];
        tVariant status;
        con.stringParam(args, tag);
        con.intParam(&args[3], 10000);
        tVariant ret;
        bool res = con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
        ASSERT(res);
        if (ret.bVal) {
            con.callAsProc(u"BasicAck", &args[2], 1);
        }
    }
}


CTEST(Select1) {
    Connection con;
    con.raiseErrors = true;
    ASSERT(connect(con));
    u16string q = qname();
    publish(con, q, u"select1");
    for (int i = 0; i < 2; i++) {
        u16string tag = basicConsume(con, q, 1);
        tVariant args[4];
        tVariant status;
        con.stringParam(args, tag);
        con.intParam(&args[3], 1000);
        tVariant ret;
        bool res = con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
        ASSERT_EQ(i == 0, ret.bVal);
        while (res && ret.bVal) {
            string data = con.retStringUtf8(&args[1]);
            std::cout << "recv " << data << endl;
            con.callAsProc(u"BasicAck", &args[2], 1);
            res = con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
        }               
        ASSERT(con.callAsProc(u"BasicCancel", args, 1));
    }
}

CTEST(MultiAck) {
   Connection con;
   con.raiseErrors = true;
   ASSERT(connect(con));
   u16string q = qname();
   for (int i = 0; i < 100; i++) {
       publish(con, q, u"multiack", u"", i != 0);
   }
   for (int i = 0; i < 2; i++) {
       u16string tag = basicConsume(con, q, 100);
       tVariant args[4];
       tVariant status;
       con.stringParam(args, tag);
       con.intParam(&args[3], 1000);
       tVariant ret;
       for (int j = 0; j < 100; j++) {
           ASSERT(con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4));
           ASSERT_EQ(i == 0, ret.bVal);
           if (i != 0) {
               break;
           }
           string data = con.retStringUtf8(&args[1]);
           ASSERT(con.callAsProc(u"BasicAck", &args[2], 1));
       }
       ASSERT(con.callAsProc(u"BasicCancel", args, 1));
   }
}

CTEST(TimeConvertion) {
    time_t time = Utils::parseDateTime("2022-01-02T12:13:14+03:00");
    tm *ut = gmtime(&time);
    ASSERT_EQ(2022, ut.tm_year+1900);
    ASSERT_EQ(1, ut.tm_mon+1);
    ASSERT_EQ(2, ut.tm_mday);
    ASSERT_EQ(9, ut.tm_hour);
    ASSERT_EQ(13, ut.tm_min);
    ASSERT_EQ(14, ut.tm_sec);
    try {
        time = Utils::parseDateTime("2022-01-02T12131403:00");
        throw exception("Must not be here");
    }
    catch (std::runtime_error& e) {
    }
}


CTEST_RUN();