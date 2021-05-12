#pragma once

#include <string>
#include <addin/types.h>
#include <addin/test/LinuxCTest.hpp>
#include <addin/test/Connection.hpp>

using namespace std;
using namespace Biterp::Test;

bool connect(Connection& conn, bool ssl = false, u16string password = u"");

u16string qname();

bool makeQueue(Connection& conn, u16string name, u16string props = u"");

bool delQueue(Connection& conn, u16string name);

bool delExchange(Connection& conn, u16string qname);

u16string lastError(Connection& conn);

bool makeExchange(Connection& conn, u16string ename, u16string props = u"");

bool bindQueue(Connection& conn, u16string name, u16string props = u"");

bool publish(Connection& conn, u16string qname, u16string msg, u16string props = u"", bool noBind=false);

u16string basicConsume(Connection& conn, u16string queue, int size=10);

u16string receiveUntil(Connection& conn, u16string qname, u16string msg, long* msgTag = nullptr);