#ifndef COMPONENT_ERROR_H
#define COMPONENT_ERROR_H

#include <string>
#include <sstream>
#include <stdexcept>
#include "../types.h"

#if defined(__ANDROID__)
#include <unistd.h>
#endif

namespace Biterp {

    class Error : public std::runtime_error {
    public:
        Error(const std::string &msg) : std::runtime_error(msg) {
            *this << msg;
#if defined(__ANDROID__)
            pthread_key_t key;
            int res = pthread_key_create(&key, NULL);
            if (res) {
                while (pthread_key_create(&key, NULL)) {
                    usleep(100);
                }
            }
            pthread_key_delete(key);
#endif
        }

        Error() : std::runtime_error("") {}

        virtual char const *what() const _NOEXCEPT override {
            return errorString.c_str();
        };

        template<typename T>
        Error &&operator<<(T value) {
            ss << value;
            errorString = ss.str();
            return std::move(*this);
        }

    private:
        std::stringstream ss;
        std::string errorString;
    };

    class TypeError : public Error {
    public:
        TypeError(int number, std::string typeExpected, int typeGot) : Error() {
            *this << "Parameter " << number << " expected type " << typeExpected << ", but got "
                  << typeGot;
        }
    };


}

#endif // COMPONENT_ERROR_H