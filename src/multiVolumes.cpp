#include "multiVolumes.h"

namespace cura {

void carveMultipleVolumes(vector<SliceVolumeStorage> &volumes)
{
    //Go trough all the volumes, and remove the previous volume outlines from our own outline, so we never have overlapped areas.
    for(unsigned int idx=0; idx < volumes.size(); idx++)
    {
        for(unsigned int idx2=0; idx2<idx; idx2++)
        {
            for(unsigned int layerNr=0; layerNr < volumes[idx].layers.size(); layerNr++)
            {
                SliceLayer* layer1 = &volumes[idx].layers[layerNr];
                SliceLayer* layer2 = &volumes[idx2].layers[layerNr];
                for(unsigned int p1 = 0; p1 < layer1->islands.size(); p1++)
                {
                    for(unsigned int p2 = 0; p2 < layer2->islands.size(); p2++)
                    {
                        layer1->islands[p1].outline = layer1->islands[p1].outline.difference(layer2->islands[p2].outline);
                    }
                }
            }
        }
    }
}

//Expand each layer a bit and then keep the extra overlapping parts that overlap with other volumes.
//This generates some overlap in dual extrusion, for better bonding in touching parts.
void generateMultipleVolumesOverlap(vector<SliceVolumeStorage> &volumes, int overlap)
{
    if (volumes.size() < 2 || overlap <= 0) return;
    
    for(unsigned int layerNr=0; layerNr < volumes[0].layers.size(); layerNr++)
    {
        Polygons fullLayer;
        for(SliceVolumeStorage &volume : volumes)
        {
            for(SliceLayerIsland &island : volume.layers[layerNr].islands)
            {
                fullLayer = fullLayer.unionPolygons(island.outline.offset(20));
            }
        }
        fullLayer = fullLayer.offset(-20);
        
        for(SliceVolumeStorage &volume : volumes)
        {
            for(SliceLayerIsland &island : volume.layers[layerNr].islands)
            {
                generateOverlap(fullLayer, island.outline, overlap);
            }
        }
    }
}

void generateOverlap(const Polygons &boundOutline, Polygons &partOutline, int overlap) {
    partOutline = boundOutline.intersection(partOutline.offset(overlap / 2));
}

}//namespace cura
