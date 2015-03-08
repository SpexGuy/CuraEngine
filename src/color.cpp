/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include "color.h"

namespace cura {

void flatColorCallback(ClipperLib::IntPoint& z1, ClipperLib::IntPoint& z2, ClipperLib::IntPoint& pt) {
    pt.Z = z1.Z;
}


// ---------------- class ColorCache --------------------
ColorCache ColorCache::instance;

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

ColorIterator& ColorCache::createColor(const Color *c) {
	std::pair<ColorIterator, bool> result = cache.insert(c);
	assert(result.second);
	return result.first;
}

}
