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

#ifndef WARHEAD_GUARDAI_H
#define WARHEAD_GUARDAI_H

#include "ScriptedCreature.h"

class Creature;

class WH_GAME_API GuardAI : public ScriptedAI
{
public:
    explicit GuardAI(Creature* creature);

    static int Permissible(Creature const* creature);

    void Reset() override;
    void EnterEvadeMode() override;
    void JustDied(Unit* killer) override;
};
#endif
