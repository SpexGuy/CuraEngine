#include "polygon.h"

using namespace ClipperLib;

namespace cura {

struct int2 {
    int2(int poly, int pt) : poly(poly), pt(pt) {}
    int2(cInt value) {*this = *reinterpret_cast<int2*>(&value);}
    int poly;
    int pt;
};
inline cInt toClipperInt(const int2 &value) {
	return *reinterpret_cast<const cInt*>(&value);
}
static_assert(sizeof(int2) <= sizeof(cInt), "int2 must fit in cInt");

Paths copyReplaceZ(const Paths &polygons) {
	Paths copy;
    copy.resize(polygons.size());
    for (unsigned int poly = 0; poly < polygons.size(); ++poly) {
        copy[poly].reserve(polygons[poly].size());
        for (unsigned int pt = 0; pt < polygons[poly].size(); ++pt) {
            copy[poly].emplace_back(polygons[poly][pt].X, polygons[poly][pt].Y, toClipperInt(int2(poly, pt)));
        }
    }
    return copy;
}

inline Point &lookup(Paths &polygons, const int2 &coord) {
	return polygons[coord.poly][coord.pt];
}
inline Point &lookup(Paths &polygons, const Point &inner) {
	return lookup(polygons, int2(inner.Z));
}
inline const Point &lookup(const Paths &polygons, const int2 &coord) {
	return polygons[coord.poly][coord.pt];
}
inline const Point &lookup(const Paths &polygons, const Point &inner) {
	return lookup(polygons, int2(inner.Z));
}
inline const Point &lookupPrev(const Paths &polygons, const int2 &coord) {
	if (coord.pt == 0)
		return polygons[coord.poly].back();
	return polygons[coord.poly][coord.pt-1];
}
inline const Point &lookupPrev(const Paths &polygons, const Point &inner) {
	return lookupPrev(polygons, int2(inner.Z));
}

inline void copyZ(Path &poly, cInt value) {
	for (Point &pt : poly) {
		pt.Z = value;
	}
}

void makePolygons(vector<Paths> &ret, const Paths &outline, const Path &offset) {
	// TODO: Coalesce like colors
	int prevIndex = offset.size() - 1;
	for (unsigned int currIndex = 0; currIndex < offset.size(); prevIndex = currIndex, ++currIndex) {
		ret.emplace_back(1);
		ret.back()[0] << lookupPrev(outline, offset[currIndex])
		              << lookup(outline, offset[currIndex]) // this color will be used
		              << offset[currIndex]
		              << offset[prevIndex];
        copyZ(ret.back()[0], ret.back()[0][1].Z); // copy the color from the second point
	}
}

vector<Polygons> Polygons::splitIntoColors(int distance) const {
    // Set up a copy of the paths where the z value holds the indexes to the point
    Paths copyPaths = copyReplaceZ(polygons);
    // Do the offset
    Paths offset;
    ClipperOffset clipper(clipper_init);
    clipper.Callback(&colorFill);
    clipper.AddPaths(copyPaths, jtMiter, etClosedPolygon);
    clipper.MiterLimit = 2.0;
    clipper.Execute(offset, -distance);

    //TODO: The right way to split this up is with voronoi on lines, but
    // I can't find a library which preserves a metainformation value through
    // the voronoi process and I don't have time to make one right now.
    // So instead, if we can't find the associated polygon, we get a nonoptimized polygon.
    // For now.

    vector<Paths> known; // The areas of the object to which color has been assigned

    // Iterate the output, creating polygons of a single color from each path
    for (const Path &path : offset) {
    	makePolygons(known, polygons, path);
    }
    // Now that we've made polygons, change offset z values to be the boundaries of infill
    for (Path &path : offset) {
    	copyZ(path, reinterpret_cast<cInt>(ColorCache::badColor));
    }
    // Add the infill areas to known
    for (Path &path : offset) {
    	known.emplace_back(1, path);
    }

    // Last step: find the unoptimized parts which are neither infill nor colored edge.
    // These areas occur when the shape pinches to a width of less than twice the given distance, and the offset gets clipped
    // Find them by subtracting all the known parts from the original polygons
    PolyTree tree;
   	Clipper unknownFinder(clipper_init);
   	unknownFinder.Callback(&colorFill);
   	unknownFinder.AddPaths(polygons, ptSubject, true);
   	for (Paths &p : known) {
   		unknownFinder.AddPaths(p, ptClip, true);
   	}
   	unknownFinder.Execute(ctDifference, tree);

   	// Finally, convert everything to Polygons and return
   	vector<Polygons> polys;
   	_processPolyTreeNode(&tree, polys); // split into parts in the result, since it may contain complex (multiple contour) polygons
   	for (Paths &poly : known) {
   		polys.push_back(Polygons(poly));
   	}
   	return polys;
}

}
