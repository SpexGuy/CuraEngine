/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include <assert.h>
#include <math.h>
#include <sstream>
#include <iostream>
#include "color.h"
#include "utils/intpoint.h"
#include "utils/logoutput.h"

using std::string;
using std::ostringstream;
using std::endl;
using std::cout;
using ClipperLib::cInt;
using ClipperLib::Path;

namespace cura {
// ---------------- class ColorExtents ------------------

//TODO: combine identical colors for efficiency
void ColorExtents::addExtent(const Color *color, const Point &p1, const Point &p2) {
    cInt length = intDistance(p1, p2);
    totalLength += length;
    extents.push_back(ColorExtent(color, length));
}
void ColorExtents::preMoveExtents(ColorExtents &other) {
    assert(canCombine(other));
    totalLength += other.totalLength;
    extents.splice(extents.begin(), other.extents);
}
void ColorExtents::reverse() {
    extents.reverse();
}
void ColorExtents::resize(const Point &p1, const Point &p2) {
    cInt dx = std::abs(p1.X - p2.X);
    cInt dy = std::abs(p1.Y - p2.Y);
    bool newUseX = dx > dy; // stay consistent with constructor setting
    cInt newLen = newUseX ? dx : dy;
    cInt oldLen = getLength();
    if (newLen == oldLen) {
        useX = newUseX;
        return;
    }

    // Use total distance rather than extent lengths
    // to avoid accumulating rounding errors
    cInt oldTravelled = 0;
    cInt newTravelled = 0;
    for (auto iter = extents.begin(); iter != extents.end(); ++iter) {
        ColorExtent &ext = *iter;
        oldTravelled += ext.length;
        cInt newEnd = (oldTravelled * newLen) / oldLen; // TODO: 128-bit multiply/divide for safety
        ext.length = newEnd - newTravelled;
        newTravelled = newEnd;
        assert(ext.length >= 0);
        if (ext.length == 0) {
            iter = extents.erase(iter);
        }
    }
    useX = newUseX;
    totalLength = newLen;
    assert(oldTravelled == oldLen);
    assert(newTravelled == newLen);
    assert(!extents.empty());
}
void ColorExtents::transferFront(cInt distance, ColorExtents &other) {
    // in
    // |--------------------------------|                   distance
    // |-----|------|--------------|-------------|--------| pre-transfer
    // out
    //                                  |--------|--------| post-transfer
    // |-----|------|--------------|----|                   other
    assert(other.useX == useX); // Other and this must use the same distance metric
    log("distance=%lld len=%lld num=%d    ", distance, totalLength, extents.size());
    
    // update lengths
    totalLength -= distance;
    other.totalLength = distance;

    // find the extent on which distance ends
    iterator transferPoint = extents.begin();
    assert(transferPoint != extents.end());
    while(distance >= transferPoint->length) {
        distance -= transferPoint->length;
        ++transferPoint;
        assert(transferPoint != extents.end());
    }
    // transfer preceding elements
    other.extents.splice(other.extents.end(), extents, extents.begin(), transferPoint);
    // add subextent on other side
    if (distance > 0) {
        other.extents.push_back(ColorExtent(transferPoint->color, distance));
        transferPoint->length -= distance;
    }
}
string ColorExtents::toString() const {
    ostringstream out;
    out << "Extents(" << extents.size() << ":" << totalLength << ", useX=" << useX << ") {" << endl;
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

ColorExtentsRef ExtentsManager::create(const Point &p1, const Point &p2) {
    ColorExtents *newExtents = new ColorExtents(p1, p2);
    allocatedExtents.push_back(newExtents);
    return ColorExtentsRef(newExtents);
}
ColorExtentsRef ExtentsManager::clone(const ColorExtentsRef &other) {
    ColorExtents *newExtents = new ColorExtents(*other);
    allocatedExtents.push_back(newExtents);
    return ColorExtentsRef(newExtents);
}
ColorExtentsRef ExtentsManager::empty_like(const ColorExtentsRef &proto) {
    ColorExtents *newExtents = new ColorExtents(proto->useX);
    allocatedExtents.push_back(newExtents);
    return ColorExtentsRef(newExtents);
}

// Clipper::FollowingZFill overrides
void ExtentsManager::OnFinishOffset(Path &poly) {
    if (poly.size() < 2) return;
    for (int c = 1; c < poly.size(); c++) {
        if (poly[c].Z) {
            ColorExtentsRef(poly[c].Z)->resize(poly[c-1], poly[c]);
        }
    }
    if (poly[0].Z) {
        ColorExtentsRef(poly[0].Z)->resize(poly.back(), poly[0]);
    }
}
cInt ExtentsManager::Clone(cInt z) {
    if (!z) {
        return z;
    }
    return clone(ColorExtentsRef(z)).toClipperInt();
}
void ExtentsManager::ReverseZ(cInt z) {
    if (!z) {
        return;
    }
    ColorExtentsRef(z)->reverse();
}
cInt ExtentsManager::StripBegin(cInt z, const Point &from, const Point &to, const Point &pt) {
    if (!z) {
        logError("Strip NULL!!\n");
        return z;
    }
    log("Stripping...    ");
    ColorExtentsRef whole(z);
    ColorExtentsRef begin = empty_like(whole);
    cInt total = whole->intDistance(from, to);
    cInt distance = whole->intDistance(from, pt);
    cInt length = whole->getLength();
    if (total != length) {
        logError("Warning: Scaling edge from %lld to %lld\n", total, length);
        distance = (distance * length) / total;
    }
    if (distance == 0 || distance == length) {
        logError("Distances equal - (%lld,%lld) (%lld,%lld) (%lld,%lld)\n", from.X, from.Y, pt.X, pt.Y, to.X, to.Y);
    }
    whole->transferFront(distance, *begin);
    log("Done\n");
    return begin.toClipperInt();
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
