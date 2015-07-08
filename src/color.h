/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef COLOR_H
#define COLOR_H

#include <set>
#include <clipper/clipper.hpp>

namespace cura {
extern ClipperLib::FollowingZFill colorFill;

class ColorCache;

class Color
{
public:
    float r;
    float g;
    float b;

private:
    Color() : r(0), g(0), b(0) {}
    Color(float red, float green, float blue) : r(red), g(green), b(blue) {}

    friend class ColorCache;
};

struct ColorComparator {
    bool operator() (const Color& self, const Color& other) {
        if (self.r != other.r)
            return self.r < other.r;
        if (self.g != other.g)
            return self.g < other.g;
        if (self.b != other.b)
            return self.b < other.b;
        return false;
    }
};

class ColorCache
{
public:
    static ColorCache& inst();
    static const Color* badColor;
    const Color* getColor(const float r, const float g, const float b);

private:
    typedef std::set<Color, ColorComparator> ColorSet;
    typedef ColorSet::iterator ColorIterator;
    
    static ColorCache instance;

    ColorSet cache;
    const ColorIterator createColor(const Color &c);
};

void flatColorCallback(ClipperLib::IntPoint& z1, ClipperLib::IntPoint& z2, ClipperLib::IntPoint& pt);
void flatColorOffsetCallback(int step, int steps, ClipperLib::IntPoint& source, ClipperLib::IntPoint& dest);

}//namespace cura

#endif//COLOR_H
