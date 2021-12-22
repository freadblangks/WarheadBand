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

 /* ScriptData
 Name: bf_commandscript
 %Complete: 100
 Comment: All bf related commands
 Category: commandscripts
 EndScriptData */

#include "BattlefieldMgr.h"
#include "Chat.h"
#include "ScriptMgr.h"

using namespace Warhead::ChatCommands;

class bg_commandscript : public CommandScript
{
public:
    bg_commandscript() : CommandScript("bg_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable bgCommandTable =
        {
            { "start", HandleBattlegroundStartCommand, SEC_ADMINISTRATOR, Console::No },
            //{ "stop",  HandleBattlegroundStopCommand,  SEC_ADMINISTRATOR, Console::No }
        };

        static ChatCommandTable commandTable =
        {
            { "bg", bgCommandTable }
        };

        return commandTable;
    }

    static bool HandleBattlegroundStartCommand(ChatHandler* handler, int32 time)
    {
        Player* player = handler->GetPlayer();
        if (!player)
        {
            return false;
        }

        auto bg = player->GetBattleground();
        if (!bg)
        {
            return false;
        }

        bg->SetStartDelayTime(time);

        return true;
    }

    //static bool HandleBattlegroundStopCommand(ChatHandler* handler)
    //{
    //    Player* target = handler->getSelectedPlayer();
    //    if (!target)
    //    {
    //        handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
    //        handler->SetSentErrorMessage(true);
    //        return false;
    //    }

    //    //target->CheckAllAchievementCriteria();
    //    return true;
    //}
};

void AddSC_bg_commandscript()
{
    new bg_commandscript();
}
