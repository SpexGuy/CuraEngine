/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef COLOR_H
#define COLOR_H

#include <list>
#include <vector>
#include <set>
#include <string>
#include <assert.h>
#include "../libs/clipper/clipper.hpp"

using ClipperLib::cInt;

namespace cura {

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
    ClipperLib::cInt length;

    ColorExtent(const Color *color, ClipperLib::cInt length) : color(color), length(length) {}
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

class ColorExtents
{
public:
    typedef std::list<ColorExtent>::iterator iterator;
    typedef std::list<ColorExtent>::const_iterator const_iterator;

    void addExtent(const Color *color, const ClipperLib::IntPoint &p1, const ClipperLib::IntPoint &p2);
    void transferFront(ClipperLib::cInt distance, ColorExtents &other);
    void preMoveExtents(ColorExtents &other);
    void reverse();
    void resize(const ClipperLib::IntPoint &p1, const ClipperLib::IntPoint &p2);
    inline bool canCombine(const ColorExtents &other) {return other.useX == useX;}
    inline ClipperLib::cInt getLength() const {return totalLength;}
    inline ClipperLib::cInt intDistance(const ClipperLib::IntPoint &p1, const ClipperLib::IntPoint &p2) const {
        return abs(useX ? p1.X - p2.X : p1.Y - p2.Y);
    }
    iterator begin() {return extents.begin();}
    iterator end() {return extents.end();}
    const_iterator begin() const {return extents.begin();}
    const_iterator end() const {return extents.end();}
    unsigned int size() const {return extents.size();}
    std::string toString() const;

private:
    ColorExtents(const ClipperLib::IntPoint &from, const ClipperLib::IntPoint &to)
      : totalLength(0),
        useX(abs(to.X - from.X) > abs(to.Y - from.Y)) {}
    ColorExtents(const ColorExtents &other)
      : extents(other.extents),
        totalLength(other.totalLength),
        useX(other.useX) {}
    ColorExtents(bool useX)
      : totalLength(0),
        useX(useX) {}
    std::list<ColorExtent> extents;
    ClipperLib::cInt totalLength;
    bool useX;

    friend class ExtentsManager; // only an ExtentsManager can create a ColorExtents
};

class ColorExtentsRef
{
public:
    ColorExtentsRef(ColorExtents *ref)  : soul(ref) {}
    ColorExtentsRef(ClipperLib::cInt z) : soul(reinterpret_cast<ColorExtents *>(z)) {assert(soul);}
    ClipperLib::cInt toClipperInt() const {return reinterpret_cast<ClipperLib::cInt>(soul);}
    ColorExtents       &operator*()       {return *soul;}
    ColorExtents const &operator*() const {return *soul;}
    ColorExtents       *operator->()       {return soul;}
    ColorExtents const *operator->() const {return soul;}

private:
    ColorExtents *soul;
};

class ExtentsManager : public ClipperLib::FollowingZFill {
public:
    static ExtentsManager& inst();
    ColorExtentsRef create(const ClipperLib::IntPoint &p1, const ClipperLib::IntPoint &p2);
    ColorExtentsRef clone(const ColorExtentsRef &other);
    ColorExtentsRef empty_like(const ColorExtentsRef &proto);
    ~ExtentsManager();

    virtual void OnFinishOffset(ClipperLib::Path &poly) override;
    virtual ClipperLib::cInt Clone(ClipperLib::cInt z) override;
    virtual void ReverseZ(ClipperLib::cInt z) override;
    virtual ClipperLib::cInt StripBegin(ClipperLib::cInt z, const ClipperLib::IntPoint &from, const ClipperLib::IntPoint &to, const ClipperLib::IntPoint &pt);

private:
    static ExtentsManager instance;

    std::vector<ColorExtents *> allocatedExtents;
    ExtentsManager() {}
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

}//namespace cura

#endif//COLOR_H
