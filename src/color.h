/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef COLOR_H
#define COLOR_H

#include <list>
#include <vector>
#include <set>
#include "../libs/clipper/clipper.hpp"

namespace cura {

class ColorCache;
class ColorExtents;

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


struct ColorExtent {
    const Color *color;
    float length;

    ColorExtent(const Color *color, float length) : color(color), length(length) {}
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

class ColorExtentsRef
{
public:
    ColorExtentsRef(ColorExtents *ref)  : soul(ref) {}
    ColorExtentsRef(ClipperLib::cInt z) : soul(reinterpret_cast<ColorExtents *>(z)) {}
    void operator=(const ColorExtentsRef &other) {soul = other.soul;}
    void addExtent(const Color *color, float dx, float dy);
    void addExtent(const Color *color, float length);
    void moveExtents(std::list<ColorExtent> &additions);
    void moveExtents(ColorExtents *other);
    void moveExtents(ColorExtentsRef &other);
    void reverse();
    ColorExtents *getReference()  const {return soul;}
    ClipperLib::cInt toClipperInt() const {return reinterpret_cast<ClipperLib::cInt>(soul);}

private:
    ColorExtents *soul;

//    static_assert(sizeof(ColorExtents *) <= sizeof(ClipperLib::cInt)); // ensure that a ColorExtents can fit in a cInt
//    char __pad[sizeof(ClipperLib::cInt) - sizeof(ColorExtents *)]; // pad the ColorExtentsRef to be the same size as a cInt, for conversion purposes
};

class ColorExtents : public ColorExtentsRef
{
public:
    void addExtent(const Color *color, float dx, float dy);
    void addExtent(const Color *color, float length);
    void moveExtents(std::list<ColorExtent> &additions);
    void moveExtents(ColorExtents *other);
    void moveExtents(ColorExtentsRef &other);
    void reverse();

private:
    ColorExtents() : ColorExtentsRef(this) {}
    std::list<ColorExtent> extents;

    friend class ExtentsManager; // only an ExtentsManager can create a ColorExtents
};

//static_assert(sizeof(ColorExtentsRef) == sizeof(ClipperLib::cInt), "ColorExtentsRef must be the same size as cInt because it is reinterpret_cast()ed into the Z member of ClipperLib::IntPoint");

class ExtentsManager
{
public:
    ExtentsManager() {}
    ColorExtentsRef create();
    ~ExtentsManager();

private:
    std::vector<ColorExtents *> allocatedExtents;
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
