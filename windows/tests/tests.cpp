#include "pch.h"
#include "CppUnitTest.h"
#include <addin/test/WindowsCppUnit.hpp>
#include <addin/test/Connection.hpp>
#include <chrono>
#include <iostream>
#include "common.h"
#include <thread>
#include <nlohmann/json.hpp>
#include <Utils.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using Addin = Biterp::Test::Connection;
using json = nlohmann::json;

namespace tests
{
	TEST_CLASS(tests)
	{
	public:
		
		TEST_METHOD(Connect)
		{
            Addin con;
			con.raiseErrors = true;
			Assert::IsTrue(connect(con));
        }

        TEST_METHOD(FailConnect)
        {
            Addin con;
            con.raiseErrors = false;
            Assert::IsFalse(connect(con ,false, u"password"));
            con.hasError("Wrong login, password or vhost");
        }


		TEST_METHOD(ConnectSsl)
		{
            Addin con;
			con.raiseErrors = true;
			Assert::IsTrue(connect(con, true));
        }

		TEST_METHOD(DefParams)
		{
            Addin con;
			con.testDefaultParams(u"Connect", 5);
			con.testDefaultParams(u"DeclareQueue", 5);
			con.testDefaultParams(u"DeclareExchange", 5);
			con.testDefaultParams(u"BasicPublish", 5);
			con.testDefaultParams(u"BindQueue", 3);
		}

        TEST_METHOD(Publish) {
            Addin conn;
            connect(conn);
            Assert::IsTrue(publish(conn, qname(), u"tset msg"));
        }


        TEST_METHOD(DeclareExchange) {
            Addin conn;
            connect(conn);
            Assert::IsTrue(makeExchange(conn, qname()));
        }

        TEST_METHOD(DeclareQueue) {
            Addin conn;
            connect(conn);
            Assert::IsTrue(makeQueue(conn, qname()));
        }

        TEST_METHOD(BindQueue) {
            Addin conn;
            connect(conn);
            conn.raiseErrors = true;
            Assert::IsTrue(bindQueue(conn, qname()));
            conn.raiseErrors = false;
        }

        TEST_METHOD(UnbindQueue) {
            Addin conn;
            connect(conn);
            tVariant paParams[3];
            u16string qn = qname();
            Assert::IsTrue(bindQueue(conn, qn));
            u16string rkey = u"#";
            conn.stringParam(paParams, qn);
            conn.stringParam(&paParams[1], qn);
            conn.stringParam(&paParams[2], rkey);
            Assert::IsTrue(conn.callAsProc(u"UnbindQueue", paParams, 3));
        }

        TEST_METHOD(DeleteQueue) {
            Addin conn;
            connect(conn);
            Assert::IsTrue(delQueue(conn, qname()));
        }

        TEST_METHOD(DeleteExchange) {
            Addin conn;
            connect(conn);
            Assert::IsTrue(delExchange(conn, qname()));
        }

        TEST_METHOD(BasicAck) {
            Addin conn;
            connect(conn);
            u16string msg = u"msgack test";
            Assert::IsTrue(publish(conn, qname(), msg));
            long tag;
            u16string got = receiveUntil(conn, qname(), msg, &tag);
            Assert::IsTrue(got == msg);
            tVariant paParams[1];
            conn.longParam(paParams, tag);
            Assert::IsTrue(conn.callAsProc(u"BasicAck", paParams, 1));
        }

        TEST_METHOD(BasicNack) {
            Addin conn;
            connect(conn);
            u16string msg = u"msgnack test";
            Assert::IsTrue(publish(conn, qname(), msg));
            long tag;
            u16string got = receiveUntil(conn, qname(), msg, &tag);
            Assert::IsTrue(got == msg);
            tVariant paParams[1];
            conn.longParam(paParams, 1);
            Assert::IsTrue(conn.callAsProc(u"BasicReject", paParams, 1));
        }

        TEST_METHOD(BasicConsume) {
            Addin conn;
            connect(conn);
            bindQueue(conn, qname());
            u16string tag = basicConsume(conn, qname());
            Assert::IsTrue(tag.length() > 0);
        }

        TEST_METHOD(BasicConsumeNoMessage) {
            Addin conn;
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
            Assert::IsTrue(res);
            Assert::IsTrue(ret.vt == VTYPE_BOOL);
            Assert::IsTrue(!ret.bVal);
            Assert::IsTrue(args[1].vt == VTYPE_EMPTY);
            Assert::IsTrue(args[2].vt == VTYPE_I4);
            Assert::IsTrue(args[2].lVal == 0);
        }

        TEST_METHOD(BasicCancel) {
            Addin conn;
            connect(conn);
            u16string tag = basicConsume(conn, qname());
            Assert::IsTrue(tag.length() > 0);
            tVariant arg[1];
            conn.stringParam(arg, tag);
            Assert::IsTrue(conn.callAsProc(u"BasicCancel", arg, 1));
        }

        TEST_METHOD(BasicConsumeReceive) {
            Publish();
            Addin conn;
            connect(conn);
            u16string tag = basicConsume(conn, qname());
            tVariant args[4];
            tVariant ret;
            conn.stringParam(args, tag);
            conn.intParam(&args[3], 10000);
            bool res = conn.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
            Assert::IsTrue(res);
            Assert::IsTrue(ret.vt == VTYPE_BOOL);
            Assert::IsTrue(ret.bVal);
            Assert::IsTrue(args[1].vt == VTYPE_PWSTR);
            u16string msg = conn.retString(&args[1]);
            Assert::IsTrue(msg.length() > 0);
        }

        TEST_METHOD(Version) {
            Addin conn;
            tVariant ver;
            Assert::IsTrue(conn.getPropVal(u"Version", &ver));
            Assert::IsTrue(ver.vt == VTYPE_PWSTR);
            conn.retString(&ver);
        }

        TEST_METHOD(SetMessageProp) {
            Addin conn;
            connect(conn);
            tVariant val;
            u16string value = u"MY_CORRELATION_ID";
            conn.stringParam(&val, value);
            Assert::IsTrue(conn.setPropVal(u"CorrelationId", &val));
            Assert::IsTrue(publish(conn, qname(), u"msgprop test"));
        }

        TEST_METHOD(GetMessageProp) {
            SetMessageProp();
            Addin conn;
            connect(conn);
            u16string text = receiveUntil(conn, qname(), u"msgprop test");
            Assert::IsTrue(text == u"msgprop test");
            tVariant ret;
            Assert::IsTrue(conn.getPropVal(u"CorrelationId", &ret));
            u16string res = conn.retString(&ret);
            Assert::IsTrue(res == u"MY_CORRELATION_ID");
        }

        TEST_METHOD(SetPriority) {
            Addin conn;
            connect(conn);
            tVariant args[1];
            conn.intParam(args, 13);
            Assert::IsTrue(conn.callAsProc(u"SetPriority", args, 1));
            Assert::IsTrue(publish(conn, qname(), u"msgpriority test"));
        }

        TEST_METHOD(GetPriority) {
            SetPriority();
            Addin conn;
            connect(conn);
            u16string text = receiveUntil(conn, qname(), u"msgpriority test");
            Assert::IsTrue(text == u"msgpriority test");
            tVariant ret;
            Assert::IsTrue(conn.callAsFunc(u"GetPriority", &ret, nullptr, 0));
            Assert::IsTrue(ret.vt == VTYPE_I4);
            Assert::IsTrue(ret.lVal == 13);
        }


        TEST_METHOD(ExtBadjson) {
            Addin con;
            con.raiseErrors = false;
            Assert::IsTrue(connect(con));
            Assert::IsTrue(!makeQueue(con, qname(), u"Not Json Object Param"));
            con.hasError("syntax error");
        }

        TEST_METHOD(ExtGoodParam) {
            Addin con;
            Assert::IsTrue(connect(con));
            Assert::IsTrue(makeQueue(con, qname(), u"{\"x-message-ttl\":60000}"));
        }

        TEST_METHOD(ExtExchange) {
            Addin con;
            Assert::IsTrue(connect(con));
            Assert::IsTrue(makeExchange(con, qname(), u"{\"alternate-exchange\":\"myae\"}"));
        }

        TEST_METHOD(ExtBind) {
            Addin con;
            Assert::IsTrue(connect(con));
            Assert::IsTrue(bindQueue(con, qname(), u"{\"x-message-ttl\":60000}"));
        }

        TEST_METHOD(ExtPublish) {
            Addin con;
            Assert::IsTrue(connect(con));
            Assert::IsTrue(publish(con, qname(), u"args message", u"{\"some-header\":13,\"yes\":true,\"no\":false}"));
            u16string text = receiveUntil(con, qname(), u"args message");
            tVariant ret;
            Assert::IsTrue(con.callAsFunc(u"GetHeaders", &ret, nullptr, 0));
            Assert::IsTrue(ret.vt == VTYPE_PWSTR);
            json obj = json::parse(con.retStringUtf8(&ret));
            Assert::AreEqual(13, (int)obj["some-header"]);
            Assert::AreEqual(true, (bool)obj["yes"]);
            Assert::AreEqual(false, (bool)obj["no"]);
        }

        TEST_METHOD(MultiConnectSsl) {
            Addin con;
            con.raiseErrors = true;
            for (int i = 0; i < 10; i++) {
                Assert::IsTrue(connect(con, true));
            }
        }

        TEST_METHOD(NoCancel) {
            Addin con;
            con.raiseErrors = true;
            for (int i = 0; i < 5; i++) {
                Assert::IsTrue(connect(con));
                for (int j = 0; j < i; j++) {
                    Assert::IsTrue(publish(con, qname(), u"args message", u"{\"some-header\":13}"));
                }
                u16string tag = basicConsume(con, qname());
                tVariant args[4];
                con.stringParam(args, tag);
                con.intParam(&args[3], 500);
                tVariant ret;
                bool res = con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
                Assert::IsTrue(res);
                if (ret.bVal) {
                    con.callAsProc(u"BasicAck", &args[2], 1);
                }
            }
        }

        TEST_METHOD(Select1) {
            Addin con;
            con.raiseErrors = true;
            Assert::IsTrue(connect(con));
            u16string q = qname();
            publish(con, q, u"select1");
            for (int i = 0; i < 2; i++) {
                u16string tag = basicConsume(con, q, 1);
                tVariant args[4];
                con.stringParam(args, tag);
                con.intParam(&args[3], 500);
                tVariant ret;
                bool res = con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
                Assert::AreEqual(i == 0, ret.bVal);
                while (res && ret.bVal) {
                    string data = con.retStringUtf8(&args[1]);
                    std::cout << "recv " << data << endl;
                    con.callAsProc(u"BasicAck", &args[2], 1);
                    res = con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4);
                }               
                Assert::IsTrue(con.callAsProc(u"BasicCancel", args, 1));
            }
        }

       TEST_METHOD(MultiAck) {
           Addin con;
           con.raiseErrors = true;
           Assert::IsTrue(connect(con));
           u16string q = qname();
           for (int i = 0; i < 100; i++) {
               publish(con, q, u"multiack", u"", i != 0);
           }
           for (int i = 0; i < 2; i++) {
               u16string tag = basicConsume(con, q, 100);
               tVariant args[4];
               con.stringParam(args, tag);
               con.intParam(&args[3], 1000);
               tVariant ret;
               for (int j = 0; j < 100; j++) {
                   Assert::IsTrue(con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4));
                   Assert::AreEqual(i == 0, ret.bVal);
                   if (i != 0) {
                       break;
                   }
                   string data = con.retStringUtf8(&args[1]);
                   Assert::IsTrue(con.callAsProc(u"BasicAck", &args[2], 1));
               }
               Assert::IsTrue(con.callAsProc(u"BasicCancel", args, 1));
           }
       }

       TEST_METHOD(EmptyHost) {
           Addin con;
           tVariant paParams[8];
           u16string host = u"";
           u16string user = u"user";
           con.stringParam(&paParams[0], host);
           con.intParam(&paParams[1], 1234);
           con.stringParam(&paParams[2], user);
           con.stringParam(&paParams[3], user);
           con.stringParam(&paParams[4], user);
           con.intParam(&paParams[5], 0);
           con.boolParam(&paParams[6], false);
           con.intParam(&paParams[7], 5);
           Assert::IsFalse(con.callAsProc(u"Connect", paParams, 8));
           con.hasError("Empty hostname not allowed");
       }

       static void thread_proc(Addin& con) {
           u16string tag = basicConsume(con, qname(), 100);
           tVariant args[4];
           con.stringParam(args, tag);
           con.intParam(&args[3], 500);
           tVariant ret;
           while (con.callAsFunc(u"BasicConsumeMessage", &ret, args, 4)) {
               if (ret.bVal) {
                   string data = con.retStringUtf8(&args[1]);
               }
           }
       }

       TEST_METHOD(KillWhileConsume) {
           std::thread t;
           {
               Addin con;
               Assert::IsTrue(connect(con));
               t = std::thread(thread_proc, std::ref(con));
               this_thread::sleep_for(chrono::seconds(1));
           }           
           this_thread::sleep_for(chrono::seconds(1));
           t.join();
       }

       TEST_METHOD(NativeSleep) {
           Addin conn; 
           Assert::IsTrue(nSleep(conn, 1000));
       }
       TEST_METHOD(ConsumeWithOffset) {
           Addin conn;
           connect(conn);
           basicConsume(conn, qname(), 10, u"{\"x-stream-offset\":\"2022-01-02T12:13:14+03:00\"}");
       }


       TEST_METHOD(TimeConvertion) {
           time_t time = Utils::parseDateTime("2022-01-02T12:13:14+03:00");
           tm ut;
           gmtime_s(&ut, &time);
           Assert::AreEqual(2022, ut.tm_year+1900);
           Assert::AreEqual(1, ut.tm_mon+1);
           Assert::AreEqual(2, ut.tm_mday);
           Assert::AreEqual(9, ut.tm_hour);
           Assert::AreEqual(13, ut.tm_min);
           Assert::AreEqual(14, ut.tm_sec);
           try {
               time = Utils::parseDateTime("2022-01-02T12131403:00");
               throw exception("Must not be here");
           }
           catch (std::runtime_error& e) {
           }
       }

	};
}
