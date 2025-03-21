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

#ifndef ADT_H
#define ADT_H
#include "ChunkedData.h"
#include "MapChunk.h"

class DoodadHandler;
class WorldModelHandler;
class LiquidHandler;

class ADT
{
public:
    ADT(std::string file, int x, int y);
    ~ADT();

    void Read();

    ChunkedData* ObjectData;
    ChunkedData* Data;
    std::vector<MapChunk*> MapChunks;
    MHDR Header;
    // Can we dispose of this?
    bool HasObjectData;

    DoodadHandler* _DoodadHandler;
    WorldModelHandler* _WorldModelHandler;
    LiquidHandler* _LiquidHandler;

    int X;
    int Y;
};
#endif
