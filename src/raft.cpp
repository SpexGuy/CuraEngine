/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include "raft.h"
#include "support.h"

namespace cura {

void generateRaft(SliceDataStorage& storage, int distance)
{
    for(SliceVolumeStorage &volume : storage.volumes)
    {
        if (volume.layers.size() < 1) continue;
        SliceLayer &layer = volume.layers[0];
        for(SliceLayerIsland &island : layer.islands)
        {
            storage.raftOutline = storage.raftOutline.unionPolygons(island.outline.offset(distance));
        }
    }

    SupportPolyGenerator supportGenerator(storage.support, 0);
    storage.raftOutline = storage.raftOutline.unionPolygons(supportGenerator.polygons.offset(distance));
    storage.raftOutline = storage.raftOutline.unionPolygons(storage.wipeTower.offset(distance));
}

}//namespace cura
