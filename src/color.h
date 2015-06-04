/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef COLOR_H
#define COLOR_H

#include <list>
#include <vector>
#include <set>
#include <assert.h>
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
    typedef std::list<ColorExtent>::iterator iterator;
    typedef std::list<ColorExtent>::const_iterator const_iterator;

    ColorExtentsRef(ColorExtents *ref)  : soul(ref) {}
    ColorExtentsRef(ClipperLib::cInt z) : soul(reinterpret_cast<ColorExtents *>(z)) {assert(soul);}
    void operator=(const ColorExtentsRef &other) {soul = other.soul;}
    void addExtent(const Color *color, float dx, float dy);
    void addExtent(const Color *color, float length);
    void moveExtents(std::list<ColorExtent> &additions);
    void moveExtents(ColorExtents *other);
    void moveExtents(ColorExtentsRef &other);
    void premoveExtents(std::list<ColorExtent> &additions);
    void premoveExtents(ColorExtents *other);
    void premoveExtents(ColorExtentsRef &other);
    void reverse();
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    unsigned int size() const;
    ColorExtents *getReference() const {return soul;}
    ClipperLib::cInt toClipperInt() const {return reinterpret_cast<ClipperLib::cInt>(soul);}

private:
    ColorExtents *soul;
};

class ColorExtents : public ColorExtentsRef
{
public:
    typedef std::list<ColorExtent>::iterator iterator;
    typedef std::list<ColorExtent>::const_iterator const_iterator;

    void addExtent(const Color *color, float dx, float dy);
    void addExtent(const Color *color, float length);
    void moveExtents(std::list<ColorExtent> &additions);
    void moveExtents(ColorExtents *other);
    void moveExtents(ColorExtentsRef &other);
    void premoveExtents(std::list<ColorExtent> &additions);
    void premoveExtents(ColorExtents *other);
    void premoveExtents(ColorExtentsRef &other);
    void reverse();
    iterator begin() {return extents.begin();}
    iterator end() {return extents.end();}
    const_iterator begin() const {return extents.begin();}
    const_iterator end() const {return extents.end();}
    unsigned int size() const {return extents.size();}

private:
    ColorExtents() : ColorExtentsRef(this) {}
    std::list<ColorExtent> extents;

    friend class ExtentsManager; // only an ExtentsManager can create a ColorExtents
};

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
