/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include <stdio.h>

#include "layerPart.h"
#include "settings.h"
#include "multiVolumes.h"

/*
The layer-part creation step is the first step in creating actual useful data for 3D printing.
It takes the result of the Slice step, which is an unordered list of polygons, and makes groups of polygons,
each of these groups is called a "part", which sometimes are also known as "islands". These parts represent
isolated areas in the 2D layer with possible holes.

Creating "parts" is an important step, as all elements in a single part should be printed before going to another part.
And all every bit inside a single part can be printed without the nozzle leaving the boundery of this part.

It's also the first step that stores the result in the "data storage" so all other steps can access it.

COLOR:
The color slicer uses LayerParts within an island, where each part is its own color.  This causes the extents
of single colors to be grouped together without creating visible artifacts in the print.
*/

namespace cura {

void createLayerWithParts(SliceLayer& storageLayer, SlicerLayer* layer, int colorDepth, int overlap, int unionAllType)
{
    storageLayer.openLines = layer->openPolygons;

    if (unionAllType & FIX_HORRIBLE_UNION_ALL_TYPE_B)
    {
        for(unsigned int i=0; i<layer->polygonList.size(); i++)
        {
            // TODO: Does reverse() screw us over?
            if (layer->polygonList[i].orientation())
                layer->polygonList[i].reverse();
        }
    }
    
    vector<Polygons> result;
    if (unionAllType & FIX_HORRIBLE_UNION_ALL_TYPE_C)
        result = layer->polygonList.offset(1000).splitIntoParts(unionAllType);
    else
        result = layer->polygonList.splitIntoParts(unionAllType);
    for(unsigned int i=0; i<result.size(); i++)
    {
        storageLayer.islands.emplace_back();
        SliceLayerIsland &part = storageLayer.islands.back();
        if (unionAllType & FIX_HORRIBLE_UNION_ALL_TYPE_C) {
            part.outline.add(result[i][0]);
            part.outline = part.outline.offset(-1000);
        } else {
            part.outline = result[i];
        }
        part.boundaryBox.calculate(part.outline);

        //TODO: This has to happen after generateMultipleVolumesOverlap
        vector<Polygons> colors = part.outline.splitIntoColors(colorDepth);
        for (Polygons &polys : colors) {
            part.regions.emplace_back();
            SliceIslandRegion &region = part.regions.back();
            region.outline = polys;
            generateOverlap(part.outline, region.outline, overlap);
            //TODO: Set part.type and part.color
        }
    }
}

void createLayerParts(SliceVolumeStorage& storage, Slicer* slicer, int printZOffset, int colorDepth, int overlap, int unionAllType)
{
    for(unsigned int layerNr = 0; layerNr < slicer->layers.size(); layerNr++)
    {
        storage.layers.push_back(SliceLayer());
        storage.layers[layerNr].sliceZ = slicer->layers[layerNr].z;
        storage.layers[layerNr].printZ = slicer->layers[layerNr].z + printZOffset;
        createLayerWithParts(storage.layers[layerNr], &slicer->layers[layerNr], colorDepth, overlap, unionAllType);
    }
}

void dumpLayerparts(SliceDataStorage& storage, const char* filename)
{
    FILE* out = fopen(filename, "w");
    fprintf(out, "<!DOCTYPE html><html><body>");
    Point3 modelSize = storage.modelSize;
    Point3 modelMin = storage.modelMin;
    
    for(SliceVolumeStorage &volume : storage.volumes)
    {
        for(SliceLayer &layer : volume.layers)
        {
            fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" style=\"width: 500px; height:500px\">\n");
            fprintf(out, "<marker id='MidMarker' viewBox='0 0 10 10' refX='5' refY='5' markerUnits='strokeWidth' markerWidth='10' markerHeight='10' stroke='lightblue' stroke-width='2' fill='none' orient='auto'>");
            fprintf(out, "<path d='M 0 0 L 10 5 M 0 10 L 10 5'/>");
            fprintf(out, "</marker>");
            fprintf(out, "<g fill-rule='evenodd' style=\"fill: gray;stroke-width:1\">\n");
            for(SliceLayerIsland &island : layer.islands)
            {
                for(SliceIslandRegion &region : island.regions)
                {
                    for(unsigned int j=0;j<region.outline.size();j++)
                    {
                        PolygonRef p = region.outline[j];
                        for(unsigned int k=0;k<p.size();k++) {
                            //TODO: print regions, not just outlines
                            const Color& color = *reinterpret_cast<const Color*>(p[k].Z);
                            fprintf(out, "<path marker-mid='url(#MidMarker)' stroke=\"#%02x%02x%02x\" d=\"", int(color.r*255), int(color.g*255), int(color.b*255));
                            fprintf(out, "M %f,%f L %f,%f ", float(p[k].X - modelMin.x)/modelSize.x*500, float(p[k].Y - modelMin.y)/modelSize.y*500, float(p[(k+1) % p.size()].X - modelMin.x)/modelSize.x*500, float(p[(k+1) % p.size()].Y - modelMin.y)/modelSize.y*500);
                            fprintf(out, "\"/>");
                        }
                        if (j == 0)
                            fprintf(out, "\" style=\"fill:gray; stroke:black;stroke-width:1\" />\n");
                        else
                            fprintf(out, "\" style=\"fill:red; stroke:black;stroke-width:1\" />\n");
                    }
                }
            }
            fprintf(out, "</g></svg>\n");
        }
    }
    fprintf(out, "</body></html>");
    fclose(out);
}

}//namespace cura
