/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include <assert.h>
#include <math.h>
#include <sstream>
#include <iostream>
#include "color.h"
#include "utils/intpoint.h"

using std::string;
using std::ostringstream;
using std::endl;
//using std::cout;

namespace cura {

void flatColorCallback(ClipperLib::IntPoint& e1bot, ClipperLib::IntPoint& e1top,
                       ClipperLib::IntPoint& e2bot, ClipperLib::IntPoint& e2top,
                       ClipperLib::IntPoint& pt) {
    //TODO: This is totally wrong.
    pt.Z = e1bot.Z;
//    assert(z2.Z);
//    float z1dist = sqrt(vSize2f(pt-z1));
//    float zdist = sqrt(vSize2f(z2-z1));
//    ColorExtentsRef ptExtents = ExtentsManager::inst().create();
//    ColorExtentsRef z2Extents(z2.Z);
//    float scale = z2Extents.getLength() / zdist;
////    cout << "----------------- Transfer " << z1dist << " of " << zdist << " (" << scale << ") from ------------------" << endl;
////    cout << z2Extents.toString();
////    ptExtents.copyExtents(z2Extents);
//    z2Extents.transferFront(z1dist * scale, ptExtents);
//    assert(ptExtents.size() > 0);
// //   cout << "to" << endl;
// //   cout << ptExtents.toString() << z2Extents.toString();
//    pt.Z = ptExtents.toClipperInt();
}

void flatColorOffsetCallback(int step, int steps, ClipperLib::IntPoint& source, ClipperLib::IntPoint& dest) {
    dest.Z = source.Z;
//    ColorExtentsRef newRef = ExtentsManager::inst().create();
//    if (step > 0) {
//        const Color *color = std::prev(ColorExtentsRef(source.Z).end())->color;
//        newRef.addExtent(color, 1.0f);
//    } else {
//        ColorExtentsRef baseRef(source.Z);
//        newRef.copyExtents(baseRef);
//    }
//    dest.Z = newRef.toClipperInt();
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
void ColorExtentsRef::premoveExtents(std::list<ColorExtent> &additions) {
    soul->premoveExtents(additions);
}
void ColorExtentsRef::premoveExtents(ColorExtents *other) {
    soul->premoveExtents(other);
}
void ColorExtentsRef::premoveExtents(ColorExtentsRef &other) {
    soul->premoveExtents(other.soul);
}
void ColorExtentsRef::copyExtents(ColorExtentsRef &other) {
    soul->copyExtents(other.soul);
}
void ColorExtentsRef::reverse() {
    soul->reverse();
}
//void ColorExtentsRef::resize(float distance) {
//    soul->resize(distance);
//}
float ColorExtentsRef::getLength() {
    return soul->getLength();
}
void ColorExtentsRef::transferFront(float distance, ColorExtentsRef &other) {
    soul->transferFront(distance, other);
}
ColorExtents::iterator ColorExtentsRef::begin() {
    return soul->begin();
}
ColorExtents::iterator ColorExtentsRef::end() {
    return soul->end();
}
ColorExtents::const_iterator ColorExtentsRef::begin() const {
    return soul->begin();
}
ColorExtents::const_iterator ColorExtentsRef::end() const {
    return soul->end();
}
unsigned int ColorExtentsRef::size() const {
    return soul->size();
}
string ColorExtentsRef::toString() const {
    return soul->toString();
}

// ---------------- class ColorExtents ------------------

//TODO: combine identical colors for efficiency
void ColorExtents::addExtent(const Color *color, float dx, float dy) {
    addExtent(color, sqrt(dx*dx + dy*dy));
}
void ColorExtents::addExtent(const Color *color, float length) {
    totalLength += length;
    extents.push_back(ColorExtent(color, length));
}
void ColorExtents::moveExtents(std::list<ColorExtent> &additions) {
    for (ColorExtent &ext : additions)
        totalLength += ext.length;
    extents.splice(extents.end(), additions);
}
void ColorExtents::moveExtents(ColorExtents *other) {
    totalLength += other->totalLength;
    extents.splice(extents.end(), other->extents);
}
void ColorExtents::moveExtents(ColorExtentsRef &other) {
    totalLength += other.getReference()->totalLength;
    extents.splice(extents.end(), other.getReference()->extents);
}
void ColorExtents::premoveExtents(std::list<ColorExtent> &additions) {
    for (ColorExtent &ext : additions)
        totalLength += ext.length;
    extents.splice(extents.begin(), additions);
}
void ColorExtents::premoveExtents(ColorExtents *other) {
    totalLength += other->totalLength;
    extents.splice(extents.begin(), other->extents);
}
void ColorExtents::premoveExtents(ColorExtentsRef &other) {
    totalLength += other.getReference()->totalLength;
    extents.splice(extents.begin(), other.getReference()->extents);
}
void ColorExtents::copyExtents(ColorExtents *other) {
    totalLength += other->totalLength;
    extents.insert(extents.end(), other->begin(), other->end());
}
void ColorExtents::reverse() {
    extents.reverse();
}
//void ColorExtents::resize(float length) {
//    float factor = length/totalLength;
//    for (ColorExtent &ext : extents) {
//        ext.length *= factor;
//    }
//    totalLength = length;
//}
void ColorExtents::transferFront(float distance, ColorExtentsRef &other) {
    // in
    // |--------------------------------|                   distance
    // |-----|------|--------------|-------------|--------| pre-transfer
    // out
    //                                  |--------|--------| post-transfer
    // |-----|------|--------------|----|                   other
    ColorExtents *o = other.getReference();
    // find the extent on which distance ends
    iterator transferPoint = extents.begin();
    assert(transferPoint != extents.end());
    while(distance >= transferPoint->length) {
        distance -= transferPoint->length;
        ++transferPoint;
        assert(transferPoint != extents.end());
    }
    // transfer preceding elements
    o->extents.splice(o->extents.end(), extents, extents.begin(), transferPoint);
    // add subextent on other side
    if (distance > 0.0f)
        o->addExtent(transferPoint->color, distance);
    // update this side
    transferPoint->length -= distance;
    // update lengths
    totalLength -= distance;
    o->totalLength = distance;
}
string ColorExtents::toString() const {
    ostringstream out;
    out << "Extents(" << totalLength << ") {" << endl;
    for (const ColorExtent &ext : extents) {
        out << "    " << ext.length << " (" << ext.color->r << ", " << ext.color->g << ", " << ext.color->b << ")" << endl;
    }
    out << "}" << endl;
    return out.str();
}

// ---------------- class ExtentsManager ----------------

ExtentsManager ExtentsManager::instance;

ExtentsManager& ExtentsManager::inst() {
    return instance;
}

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
