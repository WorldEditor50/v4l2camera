#ifndef STRINGS_HPP
#define STRINGS_HPP
#include <string>
#include <cstring>
#include <vector>
#include <stdarg.h>
struct Strings {

    static std::string format(int totalsize, const char *formatText, ...)
    {
        std::string text(totalsize, 0);
        va_list ap;
        va_start(ap, formatText);
        ::vsprintf((char*)text.c_str(), formatText, ap);
        va_end(ap);
        return text;
    }

    static std::string lower(const std::string &s_)
    {
        std::string s(s_);
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] >= 97 && s[i] <= 122) {
                s[i] -= 32;
            }
        }
        return s;
    }

    static std::string upper(const std::string &s_)
    {
        std::string s(s_);
        for (std::size_t i = 0; i < s.size(); i++) {
            if (s[i] >= 65 && s[i] <= 90) {
                s[i] += 32;
            }
        }
        return s;
    }

    static std::vector<std::string> split(const std::string& src, const std::string& delim)
    {
        std::vector<std::string> elems;
        std::size_t pos = 0;
        std::size_t len = src.length();
        std::size_t delim_len = delim.length();
        if (delim_len == 0) {
            return elems;
        }
        while (pos < len) {
            int find_pos = src.find(delim, pos);
            if (find_pos < 0) {
                elems.push_back(src.substr(pos, len - pos));
                break;
            }
            elems.push_back(src.substr(pos, find_pos - pos));
            pos = find_pos + delim_len;
        }
        return elems;
    }

    static int stringToInt(const std::string &text)
    {
        return std::atoi(text.c_str());
    }

    static char hexCharToInt4(char c)
    {
        char x = 0;
        if (c >= 'A' && c <= 'Z') {
            x = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'z') {
            x = c - 'a' + 10;
        } else if (c >= '0' && c <= '9') {
            x = c - '0';
        }
        return x;
    }

    static unsigned char hexStringToInt8(const char* hex)
    {
        unsigned char x0 = hexCharToInt4(hex[1]);
        unsigned char x1 = hexCharToInt4(hex[0]);
        return (x1<<4) + x0;
    }

    static unsigned short hexStringToInt16(const std::string &hex)
    {
        unsigned char x0 = hexCharToInt4(hex[3]);
        unsigned char x1 = hexCharToInt4(hex[2]);
        unsigned char x2 = hexCharToInt4(hex[1]);
        unsigned char x3 = hexCharToInt4(hex[0]);
        return (x3<<12) + (x2<<8) + (x1<<4) + x0;
    }


};
#endif // STRINGS_HPP
