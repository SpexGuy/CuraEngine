#include "polygon.h"
#include "../sliceDataStorage.h"

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

inline bool areAdjacent(const Paths &outline, const Point &p1, const Point &p2) {
    //TODO: This is not the fastest implementation, but it works
    return &lookupPrev(outline, p2) == &lookup(outline, p1);
}

inline int next(const Path &offset, int index) {
    return (index + 1) % offset.size();
}
inline int prev(const Path &offset, int index) {
    return index == 0 ? offset.size()-1 : index-1;
}

void makePolygons(vector<SliceIslandRegion> &regions, vector<Paths> &known, const Paths &outline, const Path &offset) {
    if (offset.size() < 2) return;
    // Find the first of this extent, inclusive
    int firstIndex = 0;
    while(areAdjacent(outline, offset[prev(offset, firstIndex)], offset[firstIndex])) {
        firstIndex = prev(offset, firstIndex);
        if (firstIndex == 0) break; // prevent infinite loop
    }

    // Iterate the extents of contiguous regions
    int extentStart = firstIndex;
    do {
        // Find the end and length of the extent which starts at extentStart, exclusive
        int extentEnd = extentStart;
        int extentLen = 0;
        do {
            extentEnd = next(offset, extentEnd);
            extentLen++;
        } while(areAdjacent(outline, offset[prev(offset, extentEnd)], offset[extentEnd]) && extentEnd != extentStart);

        // Setup the mask for known regions
        known.emplace_back(1);
        Path &whole = known.back()[0];
        whole.reserve(2*(extentLen+1)); // outside and inside part, plus first and last edge
        whole << lookupPrev(outline, offset[extentStart]);
        whole.back().Z = toClipperInt(ColorCache::badColor);

        // Build polygons along the extent
        int extentCurr = extentStart;
        do {
            //TODO: emplace_back first, then get poly to reference inside regions.back().outline
            Polygons polys;
            PolygonRef poly = polys.newPoly(); // eww... should be Polygon&

            // TODO: Combine same colors
            poly.add(lookupPrev(outline, offset[extentCurr]));
            poly.add(lookup(outline, offset[extentCurr]));
            poly.add(offset[extentCurr]);
            poly.add(offset[prev(offset, extentCurr)]);

            whole << poly[1];
            regions.emplace_back(polys, srtBorder, toColor(poly[1].Z));

            extentCurr = next(offset, extentCurr);
        } while(extentCurr != extentEnd);

        // Backtrack along the extent, adding the inside of the mask
        do {
            extentCurr = prev(offset, extentCurr);
            whole << offset[extentCurr];
            whole.back().Z = toClipperInt(ColorCache::badColor);
        } while(extentCurr != extentStart);

        // Finish off the mask
        whole << offset[prev(offset, extentStart)];
        whole.back().Z = toClipperInt(ColorCache::badColor);

        // Move on to the next extent
        extentStart = extentEnd;
    } while(extentStart != firstIndex);
}

inline cInt processUnoptimizedPolygon(PolygonRef poly) {
    const cInt badColor = toClipperInt(ColorCache::badColor);
    cInt firstColor = badColor;
    bool multipleColors = false;
    int prev = poly.size() - 1;
    for (unsigned int c = 0; c < poly.size(); prev = c++) {
        if (poly[c].Z == badColor) {
            int next = (c+1) % poly.size();
            if (poly[next].Z == badColor)
                poly[c].Z = poly[prev].Z;
            else
                poly[c].Z = poly[next].Z;
        } else {
            if (firstColor == badColor)
                firstColor = poly[c].Z;
            else if (poly[c].Z != firstColor)
                multipleColors = true;
        }
    }
    if (multipleColors)
        return badColor;
    return firstColor;
}

inline void createUnoptimizedRegions(vector<SliceIslandRegion> &regions, Polygons &polys) {
    const cInt badColor = toClipperInt(ColorCache::badColor);
    cInt firstColor = badColor;
    bool multipleColors = false;
    for (unsigned int c = 0; c < polys.size(); ++c) {
        cInt newColor = processUnoptimizedPolygon(polys[c]);
        if (newColor == badColor)
            multipleColors = true;
        else if (firstColor == badColor)
            firstColor = newColor;
        else if (firstColor != newColor)
            multipleColors = true;
    }
    if (multipleColors)
        regions.emplace_back(polys, srtUnoptimized, ColorCache::badColor);
    else
        regions.emplace_back(polys, srtBorder, toColor(firstColor));
}

void Polygons::splitIntoColors(vector<SliceIslandRegion> &regions, int distance) const {
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

    // Iterate the output, creating polygons of a single color from each path, and a mask in known
    for (const Path &path : offset) {
    	makePolygons(regions, known, polygons, path);
    }

    // Add the infill areas to known
    // TODO: Offset with a polyTree for efficiency
    for (Polygons &polygons : Polygons(offset).splitIntoParts()) {
    	known.emplace_back(polygons.polygons); // these areas should be completely covered by makePolygons, so setting their z values is unnecessary.
    	regions.emplace_back(polygons, srtInfill, ColorCache::badColor);
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
   	for (Polygons &poly : polys) {
        createUnoptimizedRegions(regions, poly);
   	}
}

}
