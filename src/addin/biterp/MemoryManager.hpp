//
// Created by ipalenov on 18.12.2020.
//

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include "../types.h"
#include "../IMemoryManager.h"
#include <string>

namespace Biterp {

    /**
     * Allocs memory for 1c
     */
    class MemoryManager {
    public:
        MemoryManager() : handle(nullptr) {}

        void setHandle(IMemoryManager *manager) { handle = manager; }

        /**
        * Alloc memory for 1c.
        * @param pointer
        * @param size
        * @return
        */
        bool alloc(void **pointer, size_t size) {
            return (handle && pointer && size) ? handle->AllocMemory(pointer, (unsigned long) size)
                                               : false;
        }

        /**
         * Alloc memory and copy string
         * @param str - string to copy or null
         * @return new string pointer or null
         */
        char16_t *allocString(const char16_t *str) {
            char16_t *out = nullptr;
            return str && copyString(&out, str) ? out : nullptr;
        }

        /**
         * Copy string to memory pointer
         * @param pointer
         * @param str
         * @return
         */
        bool copyString(char16_t **pointer, std::u16string str) {
            size_t len = (str.length() + 1) * sizeof(char16_t);
            if (!alloc((void **) pointer, len)) {
                return false;
            }
            memcpy(*pointer, str.c_str(), len);
            return true;
        }

        /**
         * Copy string to variant pwstr.
         * @param vOut
         * @param str
         * @return
         */
        bool variantFromString(tVariant *vOut, std::u16string &str) {
            vOut->vt = VTYPE_EMPTY;
            size_t len = (str.length() + 1) * sizeof(char16_t);
            if (!alloc((void **) &vOut->pwstrVal, len)) {
                return false;
            }
            memcpy(vOut->pwstrVal, str.c_str(), len);
            vOut->wstrLen = (uint32_t) str.length();
            vOut->vt = VTYPE_PWSTR;
            return true;
        }

    private:
        IMemoryManager *handle;
    };

}

#endif //MEMORYMANAGER_H
