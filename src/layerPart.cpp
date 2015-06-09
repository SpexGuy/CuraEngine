/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include <stdio.h>

#include "layerPart.h"
#include "settings.h"

/*
The layer-part creation step is the first step in creating actual useful data for 3D printing.
It takes the result of the Slice step, which is an unordered list of polygons, and makes groups of polygons,
each of these groups is called a "part", which sometimes are also known as "islands". These parts represent
isolated areas in the 2D layer with possible holes.

Creating "parts" is an important step, as all elements in a single part should be printed before going to another part.
And all every bit inside a single part can be printed without the nozzle leaving the boundery of this part.

It's also the first step that stores the result in the "data storage" so all other steps can access it.
*/

namespace cura {

void createLayerWithParts(SliceLayer& storageLayer, SlicerLayer* layer, int unionAllType)
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
        storageLayer.parts.push_back(SliceLayerPart());
        if (unionAllType & FIX_HORRIBLE_UNION_ALL_TYPE_C)
        {
            storageLayer.parts[i].outline.add(result[i][0]);
            storageLayer.parts[i].outline = storageLayer.parts[i].outline.offset(-1000);
        }else
            storageLayer.parts[i].outline = result[i];
        storageLayer.parts[i].boundaryBox.calculate(storageLayer.parts[i].outline);
    }
}

void createLayerParts(SliceVolumeStorage& storage, Slicer* slicer, int unionAllType)
{
    for(unsigned int layerNr = 0; layerNr < slicer->layers.size(); layerNr++)
    {
        storage.layers.push_back(SliceLayer());
        storage.layers[layerNr].sliceZ = slicer->layers[layerNr].z;
        storage.layers[layerNr].printZ = slicer->layers[layerNr].z;
        createLayerWithParts(storage.layers[layerNr], &slicer->layers[layerNr], unionAllType);
    }
}

void writePolygons(FILE* out, Polygons &poly, Point3 &offset, Point3 &scale) {
    fprintf(out, "<g>\n");
    for(unsigned int j=0;j<poly.size();j++)
    {
        const PolygonRef& p = poly[j];
        unsigned int prev = p.size()-1;
        for(unsigned int k=0;k<p.size();k++) {
            Point from = p[prev];
            ColorExtentsRef extents(p[k].Z);
            float len = extents.getLength();
            float distance = 0;
            for (ColorExtent &ext : extents) {
                distance += ext.length;
                float z = distance / len;
                Point to = p[prev]*(1-z) + p[k]*z;
                fprintf(out, "<path marker-mid='url(#MidMarker)' stroke=\"#%02x%02x%02x\" d=\"", int(ext.color->r*255), int(ext.color->g*255), int(ext.color->b*255));
                fprintf(out, "M %f,%f L %f,%f \" style=\"fill:%s;\"/>", float(from.X - offset.x)/scale.x*500, float(from.Y - offset.y)/scale.y*500, float(to.X - offset.x)/scale.x*500, float(to.Y - offset.y)/scale.y*500, j == 0 ? "gray" : "red");
            }
            assert(distance <= len);
            prev = k;
        }
    }
    fprintf(out, "</g>\n");
}

void dumpLayerparts(SliceDataStorage& storage, const char* filename)
{
    FILE* out = fopen(filename, "w");
    fprintf(out, "<!DOCTYPE html><html><body>");
    
    for(unsigned int volumeIdx=0; volumeIdx<storage.volumes.size(); volumeIdx++)
    {
        for(unsigned int layerNr=0;layerNr<storage.volumes[volumeIdx].layers.size(); layerNr++)
        {
            SliceLayer* layer = &storage.volumes[volumeIdx].layers[layerNr];
            fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" style=\"width: 500px; height:500px\">\n");
            fprintf(out, "<marker id='MidMarker' viewBox='0 0 10 10' refX='5' refY='5' markerUnits='strokeWidth' markerWidth='10' markerHeight='10' stroke='lightblue' stroke-width='2' fill='none' orient='auto'>");
            fprintf(out, "<path d='M 0 0 L 10 5 M 0 10 L 10 5'/>");
            fprintf(out, "</marker>");
            fprintf(out, "<g fill-rule='evenodd' style=\"fill: gray;stroke-width:1\">\n");
            for(unsigned int i=0;i<layer->parts.size();i++)
            {
                SliceLayerPart* part = &layer->parts[i];
                writePolygons(out, part->outline, storage.modelMin, storage.modelSize);
                for (Polygons &inset : part->insets) {
                    writePolygons(out, inset, storage.modelMin, storage.modelSize);
                }
            }
            fprintf(out, "</g></svg>\n");
        }
    }
    fprintf(out, "</body></html>");
    fclose(out);
}

}//namespace cura
