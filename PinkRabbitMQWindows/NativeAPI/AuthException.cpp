
#include <exception>

struct AuthException : public std::exception {
	const char* what() const throw () {
		return "Wrong login, password or vhost";
	}
};