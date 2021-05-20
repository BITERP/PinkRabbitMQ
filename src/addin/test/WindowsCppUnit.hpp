#pragma once


#include <string>
#include "CppUnitTest.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {
            template<> inline std::wstring ToString<std::u16string>(const std::u16string& t) { return std::wstring((wchar_t*)t.c_str()); }

        }
    }
}


#define ASSERT(X) Assert::IsTrue((X))
#define ASSERT_EQ(X,Y) Assert::AreEqual((X),(Y))
