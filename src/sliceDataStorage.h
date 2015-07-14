/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef SLICE_DATA_STORAGE_H
#define SLICE_DATA_STORAGE_H

#include "utils/intpoint.h"
#include "utils/polygon.h"

/*
SliceData
+ Layers[]
  + Islands[]
    + Outline[]
    + Regions[]
      + Outline[]
      + Insets[]
      + Polygons[]
      + SkinPolygons[]
*/
namespace cura {

enum SliceRegionType { srtInfill, srtBorder, srtUnoptimized };

class RegionColoring
{
public:
    RegionColoring(SliceRegionType type, const Color *color) : type(type), color(color) {}

    inline const Color *getColor(const Point &pt) const {
        switch(type) {
            case srtBorder:
                return color;
            case srtUnoptimized:
                return reinterpret_cast<const Color *>(pt.Z);
            case srtInfill: default:
                return nullptr;
        }
    }

    inline bool operator==(const RegionColoring &other) const {
        if (other.type != type) return false;
        return type != srtBorder || other.color == color;
    }
private:
    SliceRegionType type;
    const Color *color;
    RegionColoring();
};

const RegionColoring defaultColoring(srtInfill, ColorCache::badColor);
const RegionColoring unoptimizedColoring(srtUnoptimized, ColorCache::badColor);

class SliceIslandRegion
{
public:
    SliceIslandRegion(const Polygons &outline, SliceRegionType type, const Color *color)
      : coloring(type, color), outline(outline) {}
    RegionColoring coloring;
    Polygons outline;
    Polygons combBoundary;
    vector<Polygons> insets;
    Polygons skinOutline;
    Polygons sparseOutline;
private:
    SliceIslandRegion();
};

class SliceLayerIsland
{
public:
    AABB boundaryBox;
    Polygons outline;
    vector<SliceIslandRegion> regions;
};

class SliceLayer
{
public:
    int sliceZ;
    int printZ;
    vector<SliceLayerIsland> islands;
    Polygons openLines;
};

/******************/
class SupportPoint
{
public:
    int32_t z;
    double cosAngle;
    
    SupportPoint(int32_t z, double cosAngle) : z(z), cosAngle(cosAngle) {}
};
class SupportStorage
{
public:
    bool generated;
    int angle;
    bool everywhere;
    int XYDistance;
    int ZDistance;
    
    Point gridOffset;
    int32_t gridScale;
    int32_t gridWidth, gridHeight;
    vector<SupportPoint>* grid;
   	SupportStorage(){grid = nullptr;}
	  ~SupportStorage(){if(grid) delete [] grid;}
};
/******************/

class SliceVolumeStorage
{
public:
    vector<SliceLayer> layers;
};

class SliceDataStorage
{
public:
    Point3 modelSize, modelMin, modelMax;
    Polygons skirt;
    Polygons raftOutline;               //Storage for the outline of the raft. Will be filled with lines when the GCode is generated.
    vector<Polygons> oozeShield;        //oozeShield per layer
    vector<SliceVolumeStorage> volumes;
    
    SupportStorage support;
    Polygons wipeTower;
    Point wipePoint;
};

}//namespace cura

#endif//SLICE_DATA_STORAGE_H
