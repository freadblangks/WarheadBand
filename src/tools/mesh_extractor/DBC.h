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

#ifndef DBC_H
#define DBC_H
#include "Define.h"
#include <string>
#include <vector>

class Record;

class DBC
{
public:
    DBC(FILE* stream);

    std::string GetStringByOffset(int offset);

    Record* GetRecordById(int id);

    std::string Name;
    std::vector<Record*> Records;
    int RecordCount;
    int Fields;
    int RecordSize;
    uint8* StringBlock;
    uint32 StringBlockSize;
    bool IsFaulty;
};

class Record
{
public:
    Record(DBC* dbc) : Source(dbc) {}

    DBC* Source;
    std::vector<int> Values;

    int operator[](int index)
    {
        return Values[index];
    }

    template <typename T>
    T GetValue(int index)
    {
        return *(T*)(&Values[index]);
    }

    std::string Get<std::string>(int index)
    {
        return Source->GetStringByOffset(Values[index]);
    }
};

#endif
