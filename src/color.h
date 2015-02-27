/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef COLOR_H
#define COLOR_H

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

}//namespace cura

#endif//COLOR_H
