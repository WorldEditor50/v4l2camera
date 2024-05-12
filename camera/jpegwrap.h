#ifndef JPEGWRAP_H
#define JPEGWRAP_H
#include <cstdio>
#include <string.h>
#include <stdlib.h>
#include <jerror.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <memory>

/*
    wrapper of libjpeg examples
    origin:
            http://www.ijg.org/files/

*/


class Jpeg
{
public:
    struct Error {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };
    enum Align {
         ALIGN_0 = 0,
         ALIGN_4
    };
    enum Scale {
        SCALE_D1 = 1,
        SCALE_D2 = 2,
        SCALE_D4 = 4,
        SCALE_D8 = 8
    };
public:
    static void errorNotify(j_common_ptr cinfo);
    static inline int align4(int width, int channel) {return (width*channel+3)/4*4;}
    static int encode(uint8_t*& jpeg, std::size_t &totalsize,
               uint8_t* img, int w, int h, int rowstride, int quality=90);
    static int decode(uint8_t* &rgb, int &w, int &h,
               uint8_t *jpeg, std::size_t totalsize, int scale = SCALE_D1, int align=ALIGN_4);
    static int load(const char* filename, std::shared_ptr<uint8_t[]>& img, int &h, int &w, int &c);
    static int save(const char* filename, uint8_t* img, int h, int w, int c, int quality=90);
};

#endif // JPEGWRAP_H
