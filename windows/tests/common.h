#pragma once

#include <string>
#include <addin/types.h>
#include <addin/test/WindowsCppUnit.hpp>
#include <addin/test/Connection.hpp>

using namespace std;
using Addin = Biterp::Test::Connection;

bool connect(Addin& conn, bool ssl = false, u16string password = u"");

u16string qname();

bool makeQueue(Addin& conn, u16string name, u16string props = u"");

bool delQueue(Addin& conn, u16string name);

bool delExchange(Addin& conn, u16string qname);

u16string lastError(Addin& conn);

bool makeExchange(Addin& conn, u16string ename, u16string props = u"");

bool bindQueue(Addin& conn, u16string name, u16string props = u"");

bool publish(Addin& conn, u16string qname, u16string msg, u16string props = u"", bool noBind = false);

u16string basicConsume(Addin& conn, u16string queue, int size=10, u16string args=u"");

bool nSleep(Connection& conn, int timeout);

u16string receiveUntil(Addin& conn, u16string qname, u16string msg, long* msgTag = nullptr);

