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

class SliceIslandRegion
{
public:
    SliceRegionType type; //TODO: Use this
    const Color *color; //TODO: Use this
    Polygons outline;
    Polygons combBoundary;
    vector<Polygons> insets;
    Polygons skinOutline;
    Polygons sparseOutline;
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
