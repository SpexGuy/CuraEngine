/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include "skin.h"

namespace cura {

void generateSkins(int layerNr, SliceVolumeStorage& storage, int extrusionWidth, int downSkinCount, int upSkinCount, int infillOverlap)
{
    SliceLayer& layer = storage.layers[layerNr];

    for(SliceLayerIsland &island : layer.islands)
    {
        for (SliceIslandRegion &region : island.regions)
        {            
            Polygons upskin = region.insets.back().offset(-extrusionWidth/2);
            Polygons downskin = upskin;
            
            if (region.insets.size() > 1)
            {
                //Add thin wall filling by taking the area between the insets.
                Polygons thinWalls = region.insets[0].offset(-extrusionWidth / 2 - extrusionWidth * infillOverlap / 100).difference(region.insets[1].offset(extrusionWidth * 6 / 10));
                upskin.add(thinWalls);
                downskin.add(thinWalls);
            }
            if (static_cast<int>(layerNr - downSkinCount) >= 0)
            {
                for(SliceLayerIsland &island2 : storage.layers[layerNr - downSkinCount].islands)
                {
                    if (island.boundaryBox.hit(island2.boundaryBox))
                    {
                        for (SliceIslandRegion &region2 : island2.regions)
                        {
                            downskin = downskin.difference(region2.insets.back());
                        }
                    }
                }
            }
            if (static_cast<int>(layerNr + upSkinCount) < static_cast<int>(storage.layers.size()))
            {
                for(SliceLayerIsland &island2 : storage.layers[layerNr + upSkinCount].islands)
                {
                    if (island.boundaryBox.hit(island2.boundaryBox))
                    {
                        for (SliceIslandRegion &region2 : island2.regions)
                        {
                            upskin = upskin.difference(region2.insets.back());
                        }
                    }
                }
            }
            
            region.skinOutline = upskin.unionPolygons(downskin);

            double minAreaSize = (2 * M_PI * INT2MM(extrusionWidth) * INT2MM(extrusionWidth)) * 0.3;
            for(unsigned int i=0; i<region.skinOutline.size(); i++)
            {
                double area = INT2MM(INT2MM(fabs(region.skinOutline[i].area())));
                if (area < minAreaSize) // Only create an up/down skin if the area is large enough. So you do not create tiny blobs of "trying to fill"
                {
                    region.skinOutline.remove(i);
                    i -= 1;
                }
            }
        }
    }
}

void generateSparse(int layerNr, SliceVolumeStorage& storage, int extrusionWidth, int downSkinCount, int upSkinCount)
{
    SliceLayer &layer = storage.layers[layerNr];

    for(SliceLayerIsland &island : layer.islands)
    {
        for (SliceIslandRegion &region : island.regions)
        {
            Polygons sparse = region.insets.back().offset(-extrusionWidth/2);
            Polygons downskin = sparse;
            Polygons upskin = sparse;
            
            if (static_cast<int>(layerNr - downSkinCount) >= 0)
            {
                for(SliceLayerIsland &island2 : storage.layers[layerNr - downSkinCount].islands)
                {
                    if (island.boundaryBox.hit(island2.boundaryBox))
                    {
                        for (SliceIslandRegion &region2 : island2.regions)
                        {
                            if (region2.insets.size() > 1)
                            {
                                downskin = downskin.difference(region2.insets[region2.insets.size() - 2]);
                            }else{
                                downskin = downskin.difference(region2.insets.back());
                            }
                        }
                    }
                }
            }
            if (static_cast<int>(layerNr + upSkinCount) < static_cast<int>(storage.layers.size()))
            {
                for(SliceLayerIsland &island2 : storage.layers[layerNr + upSkinCount].islands)
                {
                    if (island.boundaryBox.hit(island2.boundaryBox))
                    {
                        for (SliceIslandRegion &region2 : island2.regions)
                        {
                            if (region2.insets.size() > 1)
                            {
                                upskin = upskin.difference(region2.insets[region2.insets.size() - 2]);
                            }else{
                                upskin = upskin.difference(region2.insets.back());
                            }
                        }
                    }
                }
            }
            
            Polygons result = upskin.unionPolygons(downskin);

            double minAreaSize = 3.0;//(2 * M_PI * INT2MM(config.extrusionWidth) * INT2MM(config.extrusionWidth)) * 3;
            for(unsigned int i=0; i<result.size(); i++)
            {
                double area = INT2MM(INT2MM(fabs(result[i].area())));
                if (area < minAreaSize) /* Only create an up/down skin if the area is large enough. So you do not create tiny blobs of "trying to fill" */
                {
                    result.remove(i);
                    i -= 1;
                }
            }
            
            region.sparseOutline = sparse.difference(result);
        }
    }
}

}//namespace cura
