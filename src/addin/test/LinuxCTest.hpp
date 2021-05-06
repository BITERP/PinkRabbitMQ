#ifndef LINUX_CTEST_HPP
#define LINUX_CTEST_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <typeinfo>
#include <addin/biterp/Error.hpp>

using namespace std;

#define CTEST(NAME) void NAME##_impl();\
	class NAME##_test: public CTest { public: \
		NAME##_test(): CTest(#NAME){}\
		virtual void test() override {NAME##_impl();}\
	};\
	NAME##_test NAME##_inst;\
	void NAME##_impl()

#define CTEST_RUN(...) int main(const int argc, const char* argv[]) { return CTest::run(argc, argv); }

#define ASSERT(X) do{ if(!(X)) { throw AssertError(#X, __func__, __FILE__, __LINE__);} } while(0)
#define ASSERT_EQ(X, Y) ASSERT((X) == (Y))



class AssertError : public Biterp::Error {
public:
    AssertError(string msg, string func, string file, int line) : Biterp::Error(
            formatMsg(msg, func, file, line)) {}

    static string formatMsg(string msg, string func, string file, int line) {
        stringstream ss("Assertion failed: ");
        ss << msg << endl << "Function: " << func << endl << "File: " << file << ":" << line;
        return ss.str();
    }
};

class CTest {
public:
	CTest(const string& name) {
		tests()[name] = this;
	}

	virtual void test() { throw runtime_error("Not Implemented");}

	static int run(const int argc, const char** argv) {
		string test;
		if (argc>1){
			test = argv[1];
		}
		if (test=="all"){
			test = "";
		}
		int result = EXIT_SUCCESS;
		for (auto it:tests()){
			if (!test.length() || test==it.first){
				cout << it.first << "\t\t";
				try{
					it.second->test();
					cout << "OK" << endl;
				}catch(exception& e){
					result = EXIT_FAILURE;
					cout << "FAIL" << endl;
					cerr << typeid(e).name() << ": " << e.what() << endl << endl; 
				}
			}
		}

		return result;
	}

private:
	static map<string, CTest*>& tests(){
		static map<string, CTest*> _tests;
		return _tests;
	}

};


#endif
