/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILE_BUILD_H
#define TILE_BUILD_H
#include "Recast.h"
#include <string>

#include "Geometry.h"
#include "WorldModelRoot.h"

class ContinentBuilder;
class WDT;

class TileBuilder
{
public:
    TileBuilder(ContinentBuilder* _cBuilder, std::string world, int x, int y, uint32 mapId);
    ~TileBuilder();

    void CalculateTileBounds(float*& bmin, float*& bmax, dtNavMeshParams& navMeshParams);
    uint8* BuildTiled(dtNavMeshParams& navMeshParams);
    uint8* BuildInstance(dtNavMeshParams& navMeshParams);
    void AddGeometry(WorldModelRoot* root, const WorldModelDefinition& def);
    void OutputDebugVertices();
    std::string World;
    int X;
    int Y;
    int MapId;
    rcConfig Config;
    rcConfig InstanceConfig;
    rcContext* Context;
    Geometry* _Geometry;
    uint32 DataSize;
    ContinentBuilder* cBuilder;
};
#endif
