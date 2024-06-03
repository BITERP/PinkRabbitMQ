#ifndef NAMES_HPP
#define NAMES_HPP

#include <string>
#include <map>
#include <codecvt>
#include <locale>
#include <utility>


namespace Biterp{

    struct Name{
        std::u16string en;
        std::u16string ru;
        std::string utf8;
        inline const std::u16string& nameEn() const {return en;}
        std::u16string& nameRu() {
            if (ru.empty()){
                ru = en;
            }
            return ru;
        }
        const std::string& nameUtf8(){
            if (utf8.empty()){
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
                utf8 = conv.to_bytes(en);
            }
            return utf8;
        }
    };

    struct Names{
        std::map<int, Name> names;
        
        const std::u16string& empty(){
            static std::u16string _empty;
            return _empty;
        }
        const std::string& emptyUtf8(){
            static std::string _emptyutf;
            return _emptyutf;
        }

        size_t size() const {
            return names.size();
        }

        const std::u16string& name(int id, int alias) {
            if (names.count(id) == 0 || alias > 1){
                return empty();
            }
            return alias == 1 ? names[id].nameRu() : names[id].nameEn();
        }

        const std::string& utf8(int id) {
            if (names.count(id) == 0){
                return emptyUtf8();
            }
            return names[id].nameUtf8();
        }

        long find(std::u16string name) {
            for (auto& pair: names){
                if (pair.second.nameEn() == name || pair.second.nameRu() == name){
                    return pair.first;
                }
            }
            return -1;
        }
    };

}

#endif //NAMES_HPP