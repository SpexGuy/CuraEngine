/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include "inset.h"
#include "polygonOptimizer.h"

namespace cura {

void generateInsets(SliceIslandRegion* part, int offset, int insetCount)
{
    part->combBoundary = part->outline.offset(-offset);
    if (insetCount == 0)
    {
        part->insets.push_back(part->outline);
        return;
    }
    
    for(int i=0; i<insetCount; i++)
    {
        part->insets.push_back(part->outline.offset(-offset * i - offset/2));
        optimizePolygons(part->insets[i]);
        if (part->insets[i].size() < 1)
        {
            part->insets.pop_back();
            break;
        }
    }
}

void generateInsets(SliceLayer* layer, int offset, int insetCount)
{
    for (SliceLayerIsland &island : layer->islands) {
        for (SliceIslandRegion &region : island.regions) {
            generateInsets(&region, offset, insetCount);
        }
    }
    
    //Remove the parts which did not generate an inset. As these parts are too small to print,
    // and later code can now assume that there is always minimal 1 inset line.
    for(unsigned int islandNr = 0; islandNr < layer->islands.size(); islandNr++)
    {
        for (unsigned int regionNr = 0; regionNr < layer->islands[islandNr].regions.size(); regionNr++)
        {
            if (layer->islands[islandNr].regions[regionNr].insets.size() < 1)
            {
                layer->islands[islandNr].regions.erase(layer->islands[islandNr].regions.begin() + regionNr);
                regionNr -= 1;
            }
        }
        if (layer->islands[islandNr].regions.size() < 1)
        {
            layer->islands.erase(layer->islands.begin() + islandNr);
            islandNr -= 1;
        }
    }
}

}//namespace cura
