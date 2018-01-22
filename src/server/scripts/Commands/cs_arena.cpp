/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: arena_commandscript
%Complete: 100
Comment: All arena team related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "ArenaTeamMgr.h"
#include "Chat.h"
#include "Language.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "ObjectMgr.h"

class arena_commandscript : public CommandScript
{
public:
    arena_commandscript() : CommandScript("arena_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> arenaCommandTable =
        {
            { "create",         SEC_DEVELOPER,		true,	&HandleArenaCreateCommand,   "" },
            { "disband",        SEC_DEVELOPER,		true,	&HandleArenaDisbandCommand,  "" },
            { "rename",         SEC_DEVELOPER,		true,	&HandleArenaRenameCommand,   "" },
            { "captain",        SEC_DEVELOPER,		false,	&HandleArenaCaptainCommand,  "" },
            { "info",           SEC_DEVELOPER,		true,	&HandleArenaInfoCommand,     "" },
            { "lookup",         SEC_DEVELOPER,		false,	&HandleArenaLookupCommand,   "" },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "arena",          SEC_DEVELOPER,		false, nullptr,                      "", arenaCommandTable },
        };
        return commandTable;
    }

    static bool HandleArenaCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target;
        if (!handler->extractPlayerTarget(*args != '"' ? (char*)args : nullptr, &target))
            return false;

        char* tailStr = *args != '"' ? strtok(nullptr, "") : (char*)args;
        if (!tailStr)
            return false;

        char* name = handler->extractQuotedArg(tailStr);
        if (!name)
            return false;

        char* typeStr = strtok(nullptr, "");
        if (!typeStr)
            return false;

        int8 type = atoi(typeStr);
        if (sArenaTeamMgr->GetArenaTeamByName(name))
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NAME_EXISTS, name);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (type == 2 || type == 3 || type == 1)
        {
			ArenaTeam* arena = new ArenaTeam();

            if (!arena->Create(target->GetGUID(), type, name, 4293102085, 101, 4293253939, 4, 4284049911))
            {
                delete arena;
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            sArenaTeamMgr->AddArenaTeam(arena);
            handler->PSendSysMessage(LANG_ARENA_CREATE, arena->GetName().c_str(), arena->GetId(), arena->GetType(), arena->GetCaptain());
        }
        else
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandleArenaDisbandCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 teamId = atoi((char*)args);
        if (!teamId)
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->IsFighting())
        {
            handler->SendSysMessage(LANG_ARENA_ERROR_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string name = arena->GetName();
        arena->Disband();

        if (handler->GetSession())
            sLog->outStaticDebug("GameMaster: %s [GUID: %lu] disbanded arena team type: %u [Id: %u].",
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID(), arena->GetType(), teamId);
        else
			sLog->outStaticDebug("Console: disbanded arena team type: %u [Id: %u].", arena->GetType(), teamId);

        delete(arena);

        handler->PSendSysMessage(LANG_ARENA_DISBAND, name.c_str(), teamId);
        return true;
    }

    static bool HandleArenaRenameCommand(ChatHandler* handler, char const* _args)
    {
        if (!*_args)
            return false;

        char* args = (char *)_args;

        char const* oldArenaStr = handler->extractQuotedArg(args);
        if (!oldArenaStr)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char const* newArenaStr = handler->extractQuotedArg(strtok(nullptr, ""));
        if (!newArenaStr)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamByName(oldArenaStr);
        if (!arena)
        {
            handler->PSendSysMessage(LANG_AREAN_ERROR_NAME_NOT_FOUND, oldArenaStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (sArenaTeamMgr->GetArenaTeamByName(newArenaStr))
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NAME_EXISTS, oldArenaStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->IsFighting())
        {
            handler->SendSysMessage(LANG_ARENA_ERROR_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!arena->SetName(newArenaStr))
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_ARENA_RENAME, arena->GetId(), oldArenaStr, newArenaStr);
        if (handler->GetSession())
			sLog->outStaticDebug("GameMaster: %s [GUID: %lu] rename arena team \"%s\"[Id: %u] to \"%s\"",
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID(), oldArenaStr, arena->GetId(), newArenaStr);
        else
			sLog->outStaticDebug("Console: rename arena team \"%s\"[Id: %u] to \"%s\"", oldArenaStr, arena->GetId(), newArenaStr);

        return true;
    }

    static bool HandleArenaCaptainCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* idStr;
        char* nameStr;
        handler->extractOptFirstArg((char*)args, &idStr, &nameStr);
        if (!idStr)
            return false;

        uint32 teamId = atoi(idStr);
        if (!teamId)
            return false;

        Player* target;
        uint64 targetGuid;
        if (!handler->extractPlayerTarget(nameStr, &target, &targetGuid))
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!target)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_EXIST_OR_OFFLINE, nameStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->IsFighting())
        {
            handler->SendSysMessage(LANG_ARENA_ERROR_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!arena->IsMember(targetGuid))
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_MEMBER, nameStr, arena->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->GetCaptain() == targetGuid)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_CAPTAIN, nameStr, arena->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        arena->SetCaptain(targetGuid);

		std::string capitan_name;
		uint64 capitanguid;

		capitanguid = arena->GetCaptain();

		QueryResult char_name_capitain = CharacterDatabase.PQuery("SELECT name FROM `characters` WHERE `guid` = '%u'", capitanguid);
		if (!char_name_capitain)
			capitan_name = "Неизвестно";
		else
			capitan_name = char_name_capitain->Fetch()->GetString();	
		

        handler->PSendSysMessage(LANG_ARENA_CAPTAIN, arena->GetName().c_str(), arena->GetId(), capitan_name.c_str(), target->GetName().c_str());
        if (handler->GetSession())
			sLog->outStaticDebug("GameMaster: %s [GUID: %lu] promoted player: %s [GUID: %lu] to leader of arena team \"%s\"[Id: %u]",
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID(), target->GetName().c_str(), target->GetGUID(), arena->GetName().c_str(), arena->GetId());
        else
			sLog->outStaticDebug("Console: promoted player: %s [GUID: %lu] to leader of arena team \"%s\"[Id: %u]",
                target->GetName().c_str(), target->GetGUID(), arena->GetName().c_str(), arena->GetId());

        return true;
    }

    static bool HandleArenaInfoCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 teamId = atoi((char*)args);
        if (!teamId)
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_ARENA_INFO_HEADER, arena->GetName().c_str(), arena->GetId(), arena->GetRating(), arena->GetType(), arena->GetType());
        for (ArenaTeam::MemberList::iterator itr = arena->m_membersBegin(); itr != arena->m_membersEnd(); ++itr)
            handler->PSendSysMessage(LANG_ARENA_INFO_MEMBERS, itr->Name.c_str(), itr->Guid, itr->PersonalRating, (arena->GetCaptain() == itr->Guid ? "- Captain" : ""));

        return true;
    }

    static bool HandleArenaLookupCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        bool found = false;
        ArenaTeamMgr::ArenaTeamContainer::const_iterator i = sArenaTeamMgr->GetArenaTeamMapBegin();
        for (; i != sArenaTeamMgr->GetArenaTeamMapEnd(); ++i)
        {
            ArenaTeam* arena = i->second;

            if (Utf8FitTo(arena->GetName(), wnamepart))
            {
                if (handler->GetSession())
                {
                    handler->PSendSysMessage(LANG_ARENA_LOOKUP, arena->GetName().c_str(), arena->GetId(), arena->GetType(), arena->GetType());
                    found = true;
                    continue;
                }
             }
        }

        if (!found)
            handler->PSendSysMessage(LANG_AREAN_ERROR_NAME_NOT_FOUND, namepart.c_str());

        return true;
    }
};

void AddSC_arena_commandscript()
{
    new arena_commandscript();
}
