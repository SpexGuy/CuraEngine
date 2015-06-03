/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include <assert.h>
#include <math.h>
#include "color.h"

namespace cura {

void flatColorCallback(ClipperLib::IntPoint& z1, ClipperLib::IntPoint& z2, ClipperLib::IntPoint& pt) {
    pt.Z = z1.Z;
}

void flatColorOffsetCallback(int step, int steps, ClipperLib::IntPoint& source, ClipperLib::IntPoint& dest) {
    dest.Z = source.Z;
}

// ---------------- class ColorExtentsRef ---------------

void ColorExtentsRef::addExtent(const Color *color, float dx, float dy) {
    soul->addExtent(color, dx, dy);
}
void ColorExtentsRef::addExtent(const Color *color, float length) {
    soul->addExtent(color, length);
}
void ColorExtentsRef::moveExtents(std::list<ColorExtent> &additions) {
    soul->moveExtents(additions);
}
void ColorExtentsRef::moveExtents(ColorExtents *other) {
    soul->moveExtents(other);
}
void ColorExtentsRef::moveExtents(ColorExtentsRef &other) {
    soul->moveExtents(other.soul);
}
void ColorExtentsRef::reverse() {
    soul->reverse();
}

// ---------------- class ColorExtents ------------------

void ColorExtents::addExtent(const Color *color, float dx, float dy) {
    extents.push_back(ColorExtent(color, sqrt(dx*dx + dy*dy)));
}
void ColorExtents::addExtent(const Color *color, float length) {
    extents.push_back(ColorExtent(color, length));
}
void ColorExtents::moveExtents(std::list<ColorExtent> &additions) {
    extents.splice(extents.end(), additions);
}
void ColorExtents::moveExtents(ColorExtents *other) {
    extents.splice(extents.end(), other->extents);
}
void ColorExtents::moveExtents(ColorExtentsRef &other) {
    extents.splice(extents.end(), other.getReference()->extents);
}
void ColorExtents::reverse() {
    extents.reverse();
}

// ---------------- class ExtentsManager ----------------

ColorExtentsRef ExtentsManager::create() {
    ColorExtents *newExtents = new ColorExtents();
    allocatedExtents.push_back(newExtents);
    return ColorExtentsRef(newExtents);
}

ExtentsManager::~ExtentsManager() {
    for (ColorExtents *extent : allocatedExtents) {
        delete extent;
    }
}

// ---------------- class ColorCache --------------------
ColorCache ColorCache::instance;

const Color* ColorCache::badColor = ColorCache::inst().getColor(-1.0f, -1.0f, -1.0f);

ColorCache& ColorCache::inst() {
    return instance;
}

const Color* ColorCache::getColor(const float r, const float g, const float b) {
    Color target(r,g,b);
    ColorIterator elem = cache.find(target);
    if (elem == cache.end())
        elem = createColor(target);
    return &(*elem);
}

const ColorCache::ColorIterator ColorCache::createColor(const Color& c) {
    std::pair<ColorIterator, bool> result = cache.insert(c);
    assert(result.second);
    return result.first;
}

}
