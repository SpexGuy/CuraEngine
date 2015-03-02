/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef COLOR_H
#define COLOR_H

#include "../libs/clipper/clipper.hpp"

namespace cura {

class Color
{
public:
    float r;
    float g;
    float b;

    Color() : r(0), g(0), b(0) {}
    Color(float red, float green, float blue) : r(red), g(green), b(blue) {}

};

void flatColorCallback(ClipperLib::IntPoint& z1, ClipperLib::IntPoint& z2, ClipperLib::IntPoint& pt);

}//namespace cura

#endif//COLOR_H
