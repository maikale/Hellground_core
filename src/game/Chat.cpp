/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2008 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2008-2017 Hellground <http://wow-hellground.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "Common.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "UpdateMask.h"
#include "Chat.h"
#include "MapManager.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "TicketMgr.h"
#include "AccountMgr.h"

bool ChatHandler::load_command_table = true;

ChatCommand * ChatHandler::getCommandTable()
{
    static ChatCommand arenaCommandTable[] =
    {
        { "create",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleArenaCreateCommand,    "", NULL },
        { "flusharenapoints",PERM_CONSOLE,  PERM_CONSOLE, false,  &ChatHandler::HandleFlushArenaPointsCommand,"", NULL },
        { "ready",          PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleArenaReadyCommand,     "", NULL },
        { "spectate",       PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleArenaSpectateCommand,  "", NULL },
        { "unspectate",     PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleArenaUnspectateCommand,"", NULL },
        { NULL,             0,              0,            false,  NULL,                                      "", NULL }
    };

    static ChatCommand accountSetCommandTable[] =
    {
        { "addon",          PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleAccountSetAddonCommand,     "", NULL },
        { "permissions",    PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleAccountSetPermissionsCommand,"", NULL },
        { "password",       PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleAccountSetPasswordCommand,  "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand accountAnnounceCommandTable[] =
    {
        { "battleground",   PERM_PLAYER,        PERM_CONSOLE, false, &ChatHandler::HandleAccountBattleGroundAnnCommand,   "", NULL },
        { "bg",             PERM_PLAYER,        PERM_CONSOLE, false, &ChatHandler::HandleAccountBattleGroundAnnCommand,   "", NULL },
        { "broadcast",      PERM_GMT_DEV,       PERM_CONSOLE, false, &ChatHandler::HandleAccountAnnounceBroadcastCommand, "", NULL },
        { "guild",          PERM_PLAYER,        PERM_CONSOLE, false, &ChatHandler::HandleAccountGuildAnnToggleCommand,    "", NULL },
        { NULL,             0,                  0,            false,  NULL,                                               "", NULL }
    };

    static ChatCommand accountFriendCommandTable[] =
    {
        { "add",            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleAccountFriendAddCommand,    "", NULL },
        { "delete",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleAccountFriendDeleteCommand, "", NULL },
        { "list",           PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleAccountFriendListCommand,   "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand accountCommandTable[] =
    {
        { "announce",       PERM_PLAYER,    PERM_CONSOLE, false,  NULL,                                           "", accountAnnounceCommandTable },
        { "create",         PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleAccountCreateCommand,       "", NULL },
        { "bgann",          PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountBattleGroundAnnCommand, "", NULL },
        { "bgmarks",        PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountBGMarksCommand,      "", NULL },
        { "delete",         PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleAccountDeleteCommand,       "", NULL },
        { "freerespecs",    PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountFreerespecsCommand,  "", NULL },
        { "friend",         PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", accountFriendCommandTable },
        { "gann",           PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountGuildAnnToggleCommand, "", NULL },
        { "bones",          PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountBonesHideCommand,    "", NULL },
        { "log",            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleAccountSpecialLogCommand,   "", NULL },
        { "onlinelist",     PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleAccountOnlineListCommand,   "", NULL },
        { "set",            PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", accountSetCommandTable },
        { "xp",             PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountXPToggleCommand,     "", NULL },
        { "whisp",          PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleAccountWhispLogCommand,     "", NULL },
        { "",               PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountCommand,             "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand serverSetCommandTable[] =
    {
        { "difftime",       PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleServerSetDiffTimeCommand,   "", NULL },
        { "motd",           PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerSetMotdCommand,       "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand sendCommandTable[] =
    {
        { "items",          PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleSendItemsCommand,           "", NULL },
        { "mail",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleSendMailCommand,            "", NULL },
        { "message",        PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleSendMessageCommand,         "", NULL },
        { "money",          PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleSendMoneyCommand,           "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand serverIdleRestartCommandTable[] =
    {
        { "cancel",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerShutDownCancelCommand,"", NULL },
        { ""   ,            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerIdleRestartCommand,   "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand serverIdleShutdownCommandTable[] =
    {
        { "cancel",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerShutDownCancelCommand,"", NULL },
        { ""   ,            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerIdleShutDownCommand,  "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand serverRestartCommandTable[] =
    {
        { "cancel",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerShutDownCancelCommand,"", NULL },
        { ""   ,            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerRestartCommand,       "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand serverShutdownCommandTable[] =
    {
        { "cancel",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerShutDownCancelCommand,"", NULL },
        { ""   ,            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerShutDownCommand,      "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand serverCommandTable[] =
    {
        { "corpses",        PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleServerCorpsesCommand,       "", NULL },
        { "events",         PERM_PLAYER,    PERM_CONSOLE, true,   &ChatHandler::HandleServerEventsCommand,        "", NULL },
        { "exit",           PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleServerExitCommand,          "", NULL },
        { "idlerestart",    PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", serverIdleRestartCommandTable },
        { "idleshutdown",   PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", serverShutdownCommandTable },
        { "info",           PERM_PLAYER,    PERM_CONSOLE, true,   &ChatHandler::HandleServerInfoCommand,          "", NULL },
        { "kickall",        PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerKickallCommand,       "", NULL },
        { "motd",           PERM_PLAYER,    PERM_CONSOLE, true,   &ChatHandler::HandleServerMotdCommand,          "", NULL },
        { "mute",           PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerMuteCommand,          "", NULL },
        { "pvp",            PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleServerPVPCommand,           "", NULL },
        { "restart",        PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", serverRestartCommandTable },
        { "rollshutdown",   PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleServerRollShutDownCommand,  "", NULL},
        { "set",            PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", serverSetCommandTable },
        { "shutdown",       PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", serverShutdownCommandTable },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand mmapCommandTable[] =
    {
        { "path",           PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandleMmapPathCommand,            "", NULL },
        { "loc",            PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandleMmapLocCommand,             "", NULL },
        { "loadedtiles",    PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandleMmapLoadedTilesCommand,     "", NULL },
        { "stats",          PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandleMmapStatsCommand,           "", NULL },
        { "testarea",       PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandleMmapTestArea,               "", NULL },
        { "offmesh",        PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandleMmapOffsetCreateCommand,    "", NULL },
        { "",               PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleMmap,                       "", NULL },
        { NULL,             0,              0,            false, NULL,                                           "", NULL }
    };

    static ChatCommand modifyCommandTable[] =
    {
        { "addtitle",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyAddTitleCommand,      "", NULL },
        { "arena",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyArenaCommand,         "", NULL },
        { "aspeed",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyASpeedCommand,        "", NULL },
        { "awardtitle",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyAwardTitleCommand,    "", NULL },
        { "bit",            PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyBitCommand,           "", NULL },
        { "bwalk",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyBWalkCommand,         "", NULL },
        { "drunk",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyDrunkCommand,         "", NULL },
        { "energy",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyEnergyCommand,        "", NULL },
        { "faction",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyFactionCommand,       "", NULL },
        { "fly",            PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyFlyCommand,           "", NULL },
        { "forceac",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModForceACCommand,          "", NULL },
        { "gender",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyGenderCommand,        "", NULL },
        { "honor",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyHonorCommand,         "", NULL },
        { "hp",             PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyHPCommand,            "", NULL },
        { "mana",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyManaCommand,          "", NULL },
        { "money",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyMoneyCommand,         "", NULL },
        { "morph",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyMorphCommand,         "", NULL },
        { "mount",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyMountCommand,         "", NULL },
        { "rage",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyRageCommand,          "", NULL },
        { "removetitle",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyRemoveTitleCommand,   "", NULL },
        { "rep",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyRepCommand,           "", NULL },
        { "scale",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyScaleCommand,         "", NULL },
        { "speed",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifySpeedCommand,         "", NULL },
        { "spell",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifySpellCommand,         "", NULL },
        { "standstate",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyStandStateCommand,    "", NULL },
        { "swim",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifySwimCommand,          "", NULL },
        { "strength",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyStrengthCommand,      "", NULL },
        { "agility",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyAgilityCommand,       "", NULL },
        { "stamina",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyStaminaCommand,       "", NULL },
        { "intellect",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyIntellectCommand,     "", NULL },
        { "spirit",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifySpiritCommand,        "", NULL },
        { "armor",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyArmorCommand,         "", NULL },
        { "holy",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyHolyCommand,          "", NULL },
        { "fire",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyFireCommand,          "", NULL },
        { "nature",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyNatureCommand,        "", NULL },
        { "frost",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyFrostCommand,         "", NULL },
        { "shadow",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyShadowCommand,        "", NULL },
        { "arcane",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyArcaneCommand,        "", NULL },
        { "ap",             PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyMeleeApCommand,       "", NULL },
        { "rangeap",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyRangedApCommand,      "", NULL },
        { "spellpower",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifySpellPowerCommand,    "", NULL },
        { "crit",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyMeleeCritCommand,     "", NULL },
        { "rangecrit",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyRangedCritCommand,    "", NULL },
        { "spellcrit",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifySpellCritCommand,     "", NULL },
        { "mainspeed",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyMainSpeedCommand,     "", NULL },
        { "offspeed",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyOffSpeedCommand,      "", NULL },
        { "rangespeed",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyRangedSpeedCommand,   "", NULL },
        { "castspeed",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyCastSpeedCommand,     "", NULL },
        { "block",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyBlockCommand,         "", NULL },
        { "dodge",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyDodgeCommand,         "", NULL },
        { "parry",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyParryCommand,         "", NULL },
        { "combreach",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyCrCommand,            "", NULL },
        { "boundrad",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyBrCommand,            "", NULL },
        { "titles",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyKnownTitlesCommand,   "", NULL },
        { "tp",             PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleModifyTalentCommand,        "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand wpCommandTable[] =
    {
        { "add",            PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleWpAddCommand,                "", NULL },
        { "event",          PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleWpEventCommand,              "", NULL },
        { "load",           PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleWpLoadPathCommand,           "", NULL },
        { "modify",         PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleWpModifyCommand,             "", NULL },
        { "reloadpath",     PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleWpReloadPath,                "", NULL },
        { "show",           PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleWpShowCommand,               "", NULL },
        { "tofile",         PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleWPToFileCommand,             "", NULL },
        { "unload",         PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleWpUnLoadPathCommand,         "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };


    static ChatCommand banCommandTable[] =
    {
        { "account",        PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanAccountCommand,           "", NULL },
        { "character",      PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanCharacterCommand,         "", NULL },
        { "email",          PERM_ADM,       PERM_CONSOLE, true,  &ChatHandler::HandleBanEmailCommand,             "", NULL },
        { "ip",             PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanIPCommand,                "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand baninfoCommandTable[] =
    {
        { "account",        PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanInfoAccountCommand,       "", NULL },
        { "character",      PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanInfoCharacterCommand,     "", NULL },
        { "email",          PERM_ADM,       PERM_CONSOLE, true,  &ChatHandler::HandleBanInfoEmailCommand,         "", NULL },
        { "ip",             PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanInfoIPCommand,            "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand banlistCommandTable[] =
    {
        { "account",        PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanListAccountCommand,       "", NULL },
        { "character",      PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanListCharacterCommand,     "", NULL },
        { "email",          PERM_ADM,       PERM_CONSOLE, true,  &ChatHandler::HandleBanListEmailCommand,         "", NULL },
        { "ip",             PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBanListIPCommand,            "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand botCommandTable[] =
    {
        { "add",            PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotAddCommand,               "", NULL },
        { "add_all",        PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotAddAllCommand,            "", NULL },
        { "delete",         PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotDeleteCommand,            "", NULL },
        { "info",           PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotInfoCommand,              "", NULL },
        { "reload",         PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotReloadCommand,            "", NULL },
        { "stop",           PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotStopCommand,              "", NULL },
        { "start",          PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotStartCommand,             "", NULL },
        { "ranadd",         PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleBotAddRandomCommand,         "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand partyBotCommandTable[] =
    {
        { "add",            PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotAddCommand,          "", NULL },
        { "clone",          PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotCloneCommand,        "", NULL },
        { "load",           PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotLoadCommand,         "", NULL },
        { "setrole",        PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotSetRoleCommand,      "", NULL },
        { "attackstart",    PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotAttackStartCommand,  "", NULL },
        { "attackstop",     PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotAttackStopCommand,   "", NULL },
        { "aoe",            PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotAoECommand,          "", NULL },
        { "ccmark",         PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotControlMarkCommand,  "", NULL },
        { "focusmark",      PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotFocusMarkCommand,    "", NULL },
        { "clearmarks",     PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotClearMarksCommand,   "", NULL },
        { "cometome",       PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotComeToMeCommand,     "", NULL },
        { "usegobject",     PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotUseGObjectCommand,   "", NULL },
        { "pause",          PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotPauseCommand,        "", NULL },
        { "unpause",        PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotUnpauseCommand,      "", NULL },
        { "unequip",        PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotUnequipCommand,      "", NULL },
        { "remove",         PERM_GMT,       PERM_CONSOLE, false, &ChatHandler::HandlePartyBotRemoveCommand,       "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand unbanCommandTable[] =
    {
        { "account",        PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleUnBanAccountCommand,         "", NULL },
        { "character",      PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleUnBanCharacterCommand,       "", NULL },
        { "email",          PERM_ADM,       PERM_CONSOLE, true,  &ChatHandler::HandleUnBanEmailCommand,           "", NULL },
        { "ip",             PERM_GMT,       PERM_CONSOLE, true,  &ChatHandler::HandleUnBanIPCommand,              "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand debugPlayCommandTable[] =
    {
        { "cinematic",      PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleDebugPlayCinematicCommand,   "", NULL },
        { "sound",          PERM_DEVELOPER, PERM_CONSOLE, false, &ChatHandler::HandleDebugPlaySoundCommand,       "", NULL },
        { "rangesound",     PERM_HIGH_GMT,  PERM_CONSOLE, false, &ChatHandler::HandleDebugPlayRangeSound,         "", NULL },
        { NULL,             0,              0,            false, NULL,                                            "", NULL }
    };

    static ChatCommand debugSendCommandTable[] =
    {
        { "bgopcode",       PERM_GMT_DEV,   PERM_CONSOLE, false, &ChatHandler::HandleDebugSendBattlegroundOpcodes,    "", NULL },
        { "buyerror",       PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendBuyErrorCommand,        "", NULL },
        { "channelnotify",  PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendChannelNotifyCommand,   "", NULL },
        { "chatmessage",    PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendChatMsgCommand,         "", NULL },
        { "equiperror",     PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendEquipErrorCommand,      "", NULL },
        { "opcode",         PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendOpcodeCommand,          "", NULL },
        { "petspellinit",   PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendPetSpellInitCommand,    "", NULL },
        { "poi",            PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendPoiCommand,             "", NULL },
        { "qpartymsg",      PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendQuestPartyMsgCommand,   "", NULL },
        { "qinvalidmsg",    PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendQuestInvalidMsgCommand, "", NULL },
        { "sellerror",      PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendSellErrorCommand,       "", NULL },
        { "spellfail",      PERM_ADM,       PERM_CONSOLE, false, &ChatHandler::HandleDebugSendSpellFailCommand,       "", NULL },
        { NULL,             0,              0,            false, NULL,                                                "", NULL }
    };

    static ChatCommand debugCommandTable[] =
    {
        { "addformation",   PERM_DEVELOPER, PERM_CONSOLE, false,  &ChatHandler::HandleDebugAddFormationToFileCommand, "", NULL },
        { "anim",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugAnimCommand,               "", NULL },
        { "arena",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugArenaCommand,              "", NULL },
        { "bg",             PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugBattleGroundCommand,       "", NULL },
        { "bossemote",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugBossEmoteCommand,          "", NULL },
        { "cell",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugCellCommand,               "", NULL },
        { "cooldowns",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugCooldownsCommand,          "", NULL },
        { "getitemstate",   PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugGetItemState,              "", NULL },
        { "getinstdata",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugGetInstanceDataCommand,    "", NULL },
        { "getinstdata64",  PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugGetInstanceData64Command,  "", NULL },
        { "getvalue",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugGetValue,                  "", NULL },
        { "guildkill",      PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugGuildKill,                 "", NULL },
        { "hostilelist",    PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugHostileRefList,            "", NULL },
        { "lootrecipient",  PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugGetLootRecipient,          "", NULL },
        { "joinbg",         PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugJoinBG,                    "", NULL },
        { "map",            PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugMapCommand,                "", NULL },
        { "Mod32Value",     PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugMod32Value,                "", NULL },
        { "play",           PERM_DEVELOPER, PERM_CONSOLE, false,  NULL,                                               "", debugPlayCommandTable },
        { "poolstats",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGetPoolObjectStatsCommand,      "", NULL },
        { "rel",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleRelocateCreatureCommand,        "", NULL },
        { "send",           PERM_ADM,       PERM_CONSOLE, false,  NULL,                                               "", debugSendCommandTable },
        { "setinstdata",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugSetInstanceDataCommand,    "", NULL },
        { "setinstdata64",  PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugSetInstanceData64Command,  "", NULL },
        { "setitemflag",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugSetItemFlagCommand,        "", NULL },
        { "setvalue",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugSetValue,                  "", NULL },
        { "showcombatstats",PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugShowCombatStats,           "", NULL },
        { "threatlist",     PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugThreatList,                "", NULL },
        { "printstate",     PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugUnitState,                 "", NULL },
        { "update",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugUpdate,                    "", NULL },
        { "uws",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDebugUpdateWorldStateCommand,   "", NULL },
        { "vmap",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDebugVmapsCommand,              "", NULL },
        { NULL,             0,              0,            false,  NULL,                                               "", NULL }
    };

    static ChatCommand eventCommandTable[] =
    {
        { "activelist",     PERM_GMT_DEV,   PERM_CONSOLE, true,   &ChatHandler::HandleEventActiveListCommand,     "", NULL },
        { "award",          PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleEventAwardCommand,          "", NULL },
        { "start",          PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleEventStartCommand,          "", NULL },
        { "stop",           PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleEventStopCommand,           "", NULL },
        { "",               PERM_GMT_DEV,   PERM_CONSOLE, true,   &ChatHandler::HandleEventInfoCommand,           "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand learnCommandTable[] =
    {
        { "all",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllCommand,            "", NULL },
        { "all_crafts",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllCraftsCommand,      "", NULL },
        { "all_default",    PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllDefaultCommand,     "", NULL },
        { "all_gm",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllGMCommand,          "", NULL },
        { "all_lang",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllLangCommand,        "", NULL },
        { "all_myclass",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllMyClassCommand,     "", NULL },
        { "all_myspells",   PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllMySpellsCommand,    "", NULL },
        { "all_mytalents",  PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllMyTalentsCommand,   "", NULL },
        { "all_recipes",    PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnAllRecipesCommand,     "", NULL },
        { "",               PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleLearnCommand,               "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand reloadCommandTable[] =
    {
        { "all",                         PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllCommand,                       "", NULL },
        { "all_item",                    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllItemCommand,                   "", NULL },
        { "all_loot",                    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllLootCommand,                   "", NULL },
        { "all_npc",                     PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllNpcCommand,                    "", NULL },
        { "all_quest",                   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllQuestCommand,                  "", NULL },
        { "all_scripts",                 PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllScriptsCommand,                "", NULL },
        { "all_spell",                   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAllSpellCommand,                  "", NULL },
        { "config",                      PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadConfigCommand,                    "", NULL },
        { "areatrigger_tavern",          PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAreaTriggerTavernCommand,         "", NULL },
        { "areatrigger_teleport",        PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAreaTriggerTeleportCommand,       "", NULL },
        { "access_requirement",          PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAccessRequirementCommand,         "", NULL },
        { "areatrigger_involvedrelation",PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadQuestAreaTriggersCommand,         "", NULL },
        { "auctions",                    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAuctionsCommand,                  "", NULL },
        { "autobroadcast",               PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadAutobroadcastCommand,             "", NULL },
        { "command",                     PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadCommandCommand,                   "", NULL },
        { "creature_involvedrelation",   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadCreatureQuestInvRelationsCommand, "", NULL },
        { "creature_linked_respawn",     PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadCreatureLinkedRespawnCommand,     "", NULL },
        { "creature_loot_template",      PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesCreatureCommand,     "", NULL },
        { "creature_questrelation",      PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadCreatureQuestRelationsCommand,    "", NULL },
        { "creature_ai_scripts",         PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadEventAIScriptsCommand,            "", NULL },
        { "disenchant_loot_template",    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesDisenchantCommand,   "", NULL },
        { "eventai",                     PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadEventAICommand,                   "", NULL },
        { "event_scripts",               PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadEventScriptsCommand,              "", NULL },
        { "fishing_loot_template",       PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesFishingCommand,      "", NULL },
        { "game_graveyard_zone",         PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadGameGraveyardZoneCommand,         "", NULL },
        { "game_tele",                   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadGameTeleCommand,                  "", NULL },
        { "gameobject_involvedrelation", PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadGOQuestInvRelationsCommand,       "", NULL },
        { "gameobject_loot_template",    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesGameobjectCommand,   "", NULL },
        { "gameobject_questrelation",    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadGOQuestRelationsCommand,          "", NULL },
        { "gameobject_scripts",          PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadGameObjectScriptsCommand,         "", NULL },
        { "gm_tickets",                  PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadGMTicketCommand,                  "", NULL },
        { "item_enchantment_template",   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadItemEnchantementsCommand,         "", NULL },
        { "item_loot_template",          PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesItemCommand,         "", NULL },
        { "npc_gossip",                  PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadNpcGossipCommand,                 "", NULL },
        { "npc_option",                  PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadNpcOptionCommand,                 "", NULL },
        { "npc_trainer",                 PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadNpcTrainerCommand,                "", NULL },
        { "npc_vendor",                  PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadNpcVendorCommand,                 "", NULL },
        { "page_text",                   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadPageTextsCommand,                 "", NULL },
        { "pickpocketing_loot_template", PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesPickpocketingCommand,"", NULL},
        { "prospecting_loot_template",   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesProspectingCommand,  "", NULL },
        { "quest_mail_loot_template",    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesQuestMailCommand,    "", NULL },
        { "quest_end_scripts",           PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadQuestEndScriptsCommand,           "", NULL },
        { "quest_start_scripts",         PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadQuestStartScriptsCommand,         "", NULL },
        { "quest_template",              PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadQuestTemplateCommand,             "", NULL },
        { "reference_loot_template",     PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesReferenceCommand,    "", NULL },
        { "reserved_name",               PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadReservedNameCommand,              "", NULL },
        { "reputation_reward_rate",      PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadReputationRewardRateCommand,      "", NULL },
        { "reputation_spillover_template", PERM_ADM, PERM_CONSOLE, true, &ChatHandler::HandleReloadReputationSpilloverTemplateCommand, "", NULL },
        { "skill_discovery_template",    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSkillDiscoveryTemplateCommand,    "", NULL },
        { "skill_extra_item_template",   PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSkillExtraItemPrototypeCommand,    "", NULL },
        { "skill_fishing_base_level",    PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSkillFishingBaseLevelCommand,     "", NULL },
        { "skinning_loot_template",      PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadLootTemplatesSkinningCommand,     "", NULL },
        { "spell_affect",                PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellAffectCommand,               "", NULL },
        { "spell_required",              PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellRequiredCommand,             "", NULL },
        { "spell_elixir",                PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellElixirCommand,               "", NULL },
        { "spell_learn_spell",           PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellLearnSpellCommand,           "", NULL },
        { "spell_linked_spell",          PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellLinkedSpellCommand,          "", NULL },
        { "spell_pet_auras",             PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellPetAurasCommand,             "", NULL },
        { "spell_proc_event",            PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellProcEventCommand,            "", NULL },
        { "spell_enchant_proc_data",     PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellEnchantDataCommand,          "", NULL },
        { "spell_script_target",         PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellScriptTargetCommand,         "", NULL },
        { "spell_scripts",               PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellScriptsCommand,              "", NULL },
        { "spell_target_position",       PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellTargetPositionCommand,       "", NULL },
        { "spell_threats",               PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellThreatsCommand,              "", NULL },
        { "spell_disabled",              PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadSpellDisabledCommand,             "", NULL },
        { "hellground_string",           PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadHellgroundStringCommand,          "", NULL },
        { "unqueue_account",             PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadUnqueuedAccountListCommand,       "", NULL },
        { "waypoint_scripts",            PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadWpScriptsCommand,                 "", NULL },
        { "",                            PERM_ADM,  PERM_CONSOLE, true,   &ChatHandler::HandleReloadCommand,                          "", NULL },
        { NULL,                          0,         0,            false,  NULL,                                                       "", NULL }
    };

    static ChatCommand honorCommandTable[] =
    {
        { "add",            PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleHonorAddCommand,            "", NULL },
        { "addkill",        PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleHonorAddKillCommand,        "", NULL },
        { "update",         PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleHonorUpdateCommand,         "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand guildDisableCommandTable[] =
    {
        { "announce",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGuildDisableAnnounceCommand,    "", NULL},
        { NULL,             0,              0,            false,  NULL,                                               "", NULL}
    };

    static ChatCommand guildEnableCommandTable[] =
    {
        { "announce",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGuildEnableAnnounceCommand,     "", NULL},
        { NULL,             0,              0,            false,  NULL,                                               "", NULL}
    };

    static ChatCommand guildCommandTable[] =
    {
        { "advert",         PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleGuildAdvertCommand,         "", NULL },
        { "ann",            PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleGuildAnnounceCommand,       "", NULL },
        { "create",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGuildCreateCommand,         "", NULL },
        { "delete",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGuildDeleteCommand,         "", NULL },
        { "disable",        PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", guildDisableCommandTable },
        { "enable",         PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", guildEnableCommandTable },
        { "invite",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGuildInviteCommand,         "", NULL },
        { "rank",           PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGuildRankCommand,           "", NULL },
        { "uninvite",       PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGuildUninviteCommand,       "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand petCommandTable[] =
    {
        { "create",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleCreatePetCommand,           "", NULL },
        { "learn",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandlePetLearnCommand,            "", NULL },
        { "tp",             PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandlePetTpCommand,               "", NULL },
        { "unlearn",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandlePetUnlearnCommand,          "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };


    static ChatCommand groupCommandTable[] =
    {
        { "disband",        PERM_ADM,       PERM_CONSOLE, false,   &ChatHandler::HandleGroupDisbandCommand,    "", NULL },
        { "leader",         PERM_ADM,       PERM_CONSOLE, false,   &ChatHandler::HandleGroupLeaderCommand,     "", NULL },
        { "remove",         PERM_ADM,       PERM_CONSOLE, false,   &ChatHandler::HandleGroupRemoveCommand,     "", NULL },
        { NULL,             0,              0,            false,   NULL,                                       "", NULL }
    };

    static ChatCommand lookupPlayerCommandTable[] =
    {
        { "account",        PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupPlayerAccountCommand, "", NULL },
        { "email",          PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleLookupPlayerEmailCommand,   "", NULL },
        { "ip",             PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupPlayerIpCommand,      "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand lookupCommandTable[] =
    {
        { "area",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupAreaCommand,          "", NULL },
        { "creature",       PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupCreatureCommand,      "", NULL },
        { "event",          PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupEventCommand,         "", NULL },
        { "faction",        PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupFactionCommand,       "", NULL },
        { "item",           PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupItemCommand,          "", NULL },
        { "itemset",        PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupItemSetCommand,       "", NULL },
        { "object",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupObjectCommand,        "", NULL },
        { "player",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   NULL,                                           "", lookupPlayerCommandTable },
        { "quest",          PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupQuestCommand,         "", NULL },
        { "skill",          PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupSkillCommand,         "", NULL },
        { "spell",          PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupSpellCommand,         "", NULL },
        { "tele",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleLookupTeleCommand,          "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand resetCommandTable[] =
    {
        { "all",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleResetAllCommand,            "", NULL },
        { "honor",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleResetHonorCommand,          "", NULL },
        { "level",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleResetLevelCommand,          "", NULL },
        { "spells",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleResetSpellsCommand,         "", NULL },
        { "stats",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleResetStatsCommand,          "", NULL },
        { "talents",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleResetTalentsCommand,        "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand castCommandTable[] =
    {
        { "back",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCastBackCommand,            "", NULL },
        { "dist",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCastDistCommand,            "", NULL },
        { "null",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCastNullCommand,            "", NULL },
        { "self",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCastSelfCommand,            "", NULL },
        { "target",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCastTargetCommand,          "", NULL },
        { "",               PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCastCommand,                "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand listCommandTable[] =
    {
        { "auras",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleListAurasCommand,           "", NULL },
        { "creature",       PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleListCreatureCommand,        "", NULL },
        { "item",           PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleListItemCommand,            "", NULL },
        { "object",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleListObjectCommand,          "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand teleCommandTable[] =
    {
        { "add",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleTeleAddCommand,             "", NULL },
        { "del",            PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleTeleDelCommand,             "", NULL },
        { "group",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleTeleGroupCommand,           "", NULL },
        { "name",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleTeleNameCommand,            "", NULL },
        { "",               PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleTeleCommand,                "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand npcCommandTable[] =
    {
        { "add",            PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleNpcAddCommand,              "", NULL },
        { "addformation",   PERM_DEVELOPER, PERM_CONSOLE, false,  &ChatHandler::HandleNpcAddFormationCommand,     "", NULL },
        { "additem",        PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleNpcAddItemCommand,          "", NULL },
        { "addmove",        PERM_DEVELOPER, PERM_CONSOLE, false,  &ChatHandler::HandleNpcAddMoveCommand,          "", NULL },
        { "addtemp",        PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleNpcAddTempCommand,          "", NULL },
        { "changeentry",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleNpcChangeEntryCommand,      "", NULL },
        { "changelevel",    PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleNpcChangeLevelCommand,      "", NULL },
        { "debugai",        PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcDebugAICommand,          "", NULL },
        { "delete",         PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleNpcDeleteCommand,           "", NULL },
        { "deleteformation",PERM_DEVELOPER, PERM_CONSOLE, false,  &ChatHandler::HandleNpcDeleteFormationCommand,  "", NULL },
        { "delitem",        PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcDelItemCommand,          "", NULL },
        { "dmginfo",        PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcDmginfoCommand,          "", NULL },
        { "doaction",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcDoActionCommand,         "", NULL },
        { "enterevademode", PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcEnterEvadeModeCommand,   "", NULL },
        { "extraflag",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcExtraFlagCommand,        "", NULL },
        { "factionid",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcFactionIdCommand,        "", NULL },
        { "fieldflag",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcFieldFlagCommand,        "", NULL },
        { "flag",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcFlagCommand,             "", NULL },
        { "follow",         PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcFollowCommand,           "", NULL },
        { "info",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcInfoCommand,             "", NULL },
        { "move",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcMoveCommand,             "", NULL },
        { "playemote",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcPlayEmoteCommand,        "", NULL },
        { "resetai",        PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcResetAICommand,          "", NULL },
        { "say",            PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNpcSayCommand,              "", NULL },
        { "setlink",        PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcSetLinkCommand,          "", NULL },
        { "setmodel",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcSetModelCommand,         "", NULL },
        { "setmovetype",    PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcSetMoveTypeCommand,      "", NULL },
        { "spawndist",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcSpawnDistCommand,        "", NULL },
        { "spawntime",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcSpawnTimeCommand,        "", NULL },
        { "standstate",     PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcStandState,              "", NULL },
        { "textemote",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNpcTextEmoteCommand,        "", NULL },
        { "unfollow",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNpcUnFollowCommand,         "", NULL },
        { "whisper",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNpcWhisperCommand,          "", NULL },
        { "yell",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNpcYellCommand,             "", NULL },

        //{ TODO: fix or remove this commands
        { "name",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNameCommand,                "", NULL },
        { "subname",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleSubNameCommand,             "", NULL },
        { "addweapon",      PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleAddWeaponCommand,           "", NULL },
        //}
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand goCommandTable[] =
    {
        { "creature",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoCreatureCommand,          "", NULL },
        { "direct",         PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoCreatureDirectCommand,    "", NULL },
        { "graveyard",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoGraveyardCommand,         "", NULL },
        { "grid",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoGridCommand,              "", NULL },
        { "object",         PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoObjectCommand,            "", NULL },
        { "ticket",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGoTicketCommand,            "", NULL },
        { "trigger",        PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoTriggerCommand,           "", NULL },
        { "zonexy",         PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoZoneXYCommand,            "", NULL },
        { "xy",             PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoXYCommand,                "", NULL },
        { "xyz",            PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoXYZCommand,               "", NULL },
        { "",               PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGoXYZCommand,               "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand gobjectCommandTable[] =
    {
        { "activate",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectActivateCommand,  "", NULL },
        { "add",            PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectAddCommand,       "", NULL },
        { "addtemp",        PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectAddTempCommand,   "", NULL },
        { "delete",         PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectDeleteCommand,    "", NULL },
        { "grid",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectGridCommand,      "", NULL },
        { "move",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectMoveCommand,      "", NULL },
        { "near",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectNearCommand,      "", NULL },
        { "reset",          PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectResetCommand,     "", NULL },
        { "rotate",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectRotateCommand,    "", NULL },
        { "target",         PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectTargetCommand,    "", NULL },
        { "turn",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGameObjectTurnCommand,      "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand questCommandTable[] =
    {
        { "add",            PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleQuestAdd,                   "", NULL },
        { "complete",       PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleQuestComplete,              "", NULL },
        { "remove",         PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleQuestRemove,                "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand gmCommandTable[] =
    {
        { "announce",       PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGMAnnounceCommand,          "", NULL },
        { "chat",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGMChatCommand,              "", NULL },
        { "fly",            PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleGMFlyCommand,               "", NULL },
        { "list",           PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleGMListFullCommand,          "", NULL },
        { "nameannounce",   PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGMNameAnnounceCommand,      "", NULL },
        { "notify",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleGMNotifyCommand,            "", NULL },
        { "triggers",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGMTriggersCommand,          "", NULL },
        { "visible",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGMVisibleCommand,           "", NULL },
        { "",               PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGMCommand,                  "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand instanceCommandTable[] =
    {
        { "listbinds",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleInstanceListBindsCommand,   "", NULL },
        { "savedata",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleInstanceSaveDataCommand,    "", NULL },
        { "selfunbind",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleInstanceSelfUnbindCommand,  "", NULL },
        { "stats",          PERM_GMT_DEV,   PERM_CONSOLE, true,   &ChatHandler::HandleInstanceStatsCommand,       "", NULL },
        { "unbind",         PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleInstanceUnbindCommand,      "", NULL },
        { "bind",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleInstanceBindCommand,        "", NULL },
        { "resetencounters", PERM_GMT,      PERM_CONSOLE, false,  &ChatHandler::HandleInstanceResetEncountersCommand, "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand ticketCommandTable[] =
    {
        { "assign",         PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketAssignToCommand,    "", NULL },
        { "close",          PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketCloseByIdCommand,   "", NULL },
        { "closedlist",     PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketListClosedCommand,  "", NULL },
        { "comment",        PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketCommentCommand,     "", NULL },
        { "delete",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketDeleteByIdCommand,  "", NULL },
        { "history",        PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketHistoryCommand,     "", NULL },
        { "list",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketListCommand,        "", NULL },
        { "onlinelist",     PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketListOnlineCommand,  "", NULL },
        { "response",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketResponseCommand,    "", NULL },
        { "unassign",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketUnAssignCommand,    "", NULL },
        { "viewid",         PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketGetByIdCommand,     "", NULL },
        { "viewname",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleGMTicketGetByNameCommand,   "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand channelCommandTable[] =
    {
        { "kick",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleChannelKickCommand,         "", NULL },
        { "list",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleChannelListCommand,         "", NULL },
        { "masskick",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleChannelMassKickCommand,     "", NULL },
        { "pass",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleChannelPassCommand,         "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand characterPremadeCommandTable[] =
    {
        { "gear",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleCharacterPremadeGearCommand,     "", NULL },
        { "spec",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleCharacterPremadeSpecCommand,     "", NULL },
        { "savegear",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleCharacterPremadeSaveGearCommand, "", NULL },
        { "savespec",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleCharacterPremadeSaveSpecCommand, "", NULL },
        { NULL,             0,              0,            false,  NULL,                                                "", NULL }
    };

    static ChatCommand characterCommandTable[] =
    {
        { "premade",        PERM_PLAYER,    PERM_CONSOLE, true,   NULL,                                           "", characterPremadeCommandTable },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand crashCommandTable[] =
    {
        { "map",            PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleCrashMapCommand,            "", NULL },
        { "server",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleCrashServerCommand,         "", NULL },
        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    static ChatCommand commandTable[] =
    {
        { "account",        PERM_PLAYER,    PERM_CONSOLE, true,   NULL,                                           "", accountCommandTable },
        { "arena",          PERM_PLAYER,    PERM_CONSOLE, false,  NULL,                                           "", arenaCommandTable },
        { "ban",            PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", banCommandTable },
        { "baninfo",        PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", baninfoCommandTable },
        { "banlist",        PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", banlistCommandTable },
        { "bot",            PERM_GMT,       PERM_CONSOLE, false,  NULL,                                "Manage bots", botCommandTable },
        { "partybot",       PERM_GMT,       PERM_CONSOLE, false,  NULL,                          "Manage party bots", partyBotCommandTable },
        { "cast",           PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", castCommandTable },
        { "character",      PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", characterCommandTable },
        { "channel",        PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", channelCommandTable},
        { "crash",          PERM_HIGH_GMT,  PERM_CONSOLE, false,  NULL,                                           "", crashCommandTable },
        { "debug",          PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", debugCommandTable },
        { "event",          PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", eventCommandTable },
        { "gm",             PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", gmCommandTable },
        { "go",             PERM_GMT_DEV,   PERM_CONSOLE, false,  NULL,                                           "", goCommandTable },
        { "gobject",        PERM_GMT_DEV,   PERM_CONSOLE, false,  NULL,                                           "", gobjectCommandTable },
        { "guild",          PERM_HIGH_GMT,  PERM_CONSOLE, true,   NULL,                                           "", guildCommandTable },
        { "honor",          PERM_HIGH_GMT,  PERM_CONSOLE, false,  NULL,                                           "", honorCommandTable },
        { "instance",       PERM_GMT_DEV,   PERM_CONSOLE, true,   NULL,                                           "", instanceCommandTable },
        { "learn",          PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", learnCommandTable },
        { "list",           PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", listCommandTable },
        { "lookup",         PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", lookupCommandTable },
        { "modify",         PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", modifyCommandTable },
        { "mmap",           PERM_GMT_DEV,   PERM_CONSOLE, false,  NULL,                                           "", mmapCommandTable },
        { "npc",            PERM_GMT_DEV,   PERM_CONSOLE, false,  NULL,                                           "", npcCommandTable },
        { "path",           PERM_GMT_DEV,   PERM_CONSOLE, false,  NULL,                                           "", wpCommandTable },
        { "pet",            PERM_GMT,       PERM_CONSOLE, false,  NULL,                                           "", petCommandTable },
        { "quest",          PERM_HIGH_GMT,  PERM_CONSOLE, false,  NULL,                                           "", questCommandTable },
        { "reload",         PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", reloadCommandTable },
        { "reset",          PERM_ADM,       PERM_CONSOLE, false,  NULL,                                           "", resetCommandTable },
        { "send",           PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", sendCommandTable },
        { "server",         PERM_ADM,       PERM_CONSOLE, true,   NULL,                                           "", serverCommandTable },
        { "tele",           PERM_GMT_DEV,   PERM_CONSOLE, true,   NULL,                                           "", teleCommandTable },
        { "ticket",         PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", ticketCommandTable },
        { "unban",          PERM_GMT,       PERM_CONSOLE, true,   NULL,                                           "", unbanCommandTable },
        { "wp",             PERM_GMT_DEV,   PERM_CONSOLE, false,  NULL,                                           "", wpCommandTable },

        { "additem",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleAddItemCommand,             "", NULL },
        { "additemset",     PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleAddItemSetCommand,          "", NULL },
        { "allowmove",      PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleAllowMovementCommand,       "", NULL },
        { "announce",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleAnnounceCommand,            "", NULL },
        { "aura",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleAuraCommand,                "", NULL },
        { "bank",           PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleBankCommand,                "", NULL },
        { "bindfollow",     PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleBindFollowCommand,          "", NULL },
        { "bindsight",      PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleBindSightCommand,           "", NULL },
        { "cd",             PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleCooldownCommand,            "", NULL },
        { "chardelete",     PERM_CONSOLE,   PERM_CONSOLE, true,   &ChatHandler::HandleCharacterDeleteCommand,     "", NULL },
        { "combatstop",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleCombatStopCommand,          "", NULL },
        { "cometome",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleComeToMeCommand,            "", NULL },
        { "commands",       PERM_PLAYER,    PERM_CONSOLE, true,   &ChatHandler::HandleCommandsCommand,            "", NULL },
        { "cooldown",       PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleCooldownCommand,            "", NULL },
        { "damage",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDamageCommand,              "", NULL },
        { "demorph",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleDeMorphCommand,             "", NULL },
        { "die",            PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleDieCommand,                 "", NULL },
        { "dismount",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleDismountCommand,            "", NULL },
        { "distance",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGetDistanceCommand,         "", NULL },
        { "explorecheat",   PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleExploreCheatCommand,        "", NULL },
        { "freeze",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleFreezeCommand,              "", NULL },
        { "goname",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGonameCommand,              "", NULL },
        { "gps",            PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGPSCommand,                 "", NULL },
        { "info",           PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleInfoCommand,                "", NULL },
        { "groupgo",        PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleGroupgoCommand,             "", NULL },
        { "guid",           PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleGUIDCommand,                "", NULL },
        { "hdevannounce",   PERM_HEAD_DEVELOPER,PERM_CONSOLE, false,&ChatHandler::HandleHDevAnnounceCommand,      "", NULL },
        { "help",           PERM_PLAYER,    PERM_CONSOLE, true,   &ChatHandler::HandleHelpCommand,                "", NULL },
        { "hidearea",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleHideAreaCommand,            "", NULL },
        { "hover",          PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleHoverCommand,               "", NULL },
        { "itemmove",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleItemMoveCommand,            "", NULL },
        { "kick",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleKickPlayerCommand,          "", NULL },
        { "levelup",        PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleLevelUpCommand,             "", NULL },
        { "linkgrave",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleLinkGraveCommand,           "", NULL },
        { "listfreeze",     PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleListFreezeCommand,          "", NULL },
        { "loadscripts",    PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandleLoadScriptsCommand,         "", NULL },
        { "lockaccount",    PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleLockAccountCommand,         "", NULL },
        { "maxskill",       PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleMaxSkillCommand,            "", NULL },
        { "movegens",       PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleMovegensCommand,            "", NULL },
        { "mute",           PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleMuteCommand,                "", NULL },
        { "muteinfo",       PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleMuteInfoCommand,            "", NULL },
        { "nameannounce",   PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNameAnnounceCommand,        "", NULL },
        { "namego",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleNamegoCommand,              "", NULL },
        { "neargrave",      PERM_GMT_DEV,   PERM_CONSOLE, false,  &ChatHandler::HandleNearGraveCommand,           "", NULL },
        { "notify",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleNotifyCommand,              "", NULL },
        { "password",       PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandlePasswordCommand,            "", NULL },
        { "pinfo",          PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandlePInfoCommand,               "", NULL },
        { "playall",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandlePlayAllCommand,             "", NULL },
        { "plimit",         PERM_ADM,       PERM_CONSOLE, true,   &ChatHandler::HandlePLimitCommand,              "", NULL },
        { "possess",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandlePossessCommand,             "", NULL },
        { "recall",         PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleRecallCommand,              "", NULL },
        { "rename",         PERM_HIGH_GMT,  PERM_CONSOLE, true,   &ChatHandler::HandleRenameCommand,              "", NULL },
        { "replenish",      PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleReplenishCommand,           "", NULL },
        { "repairitems",    PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleRepairitemsCommand,         "", NULL },
        { "respawn",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleRespawnCommand,             "", NULL },
        { "revive",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleReviveCommand,              "", NULL },
        { "revivegroup",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleReviveGroupCommand,         "", NULL },
        { "save",           PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleSaveCommand,                "", NULL },
        { "setskill",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleSetSkillCommand,            "", NULL },
        { "showarea",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleShowAreaCommand,            "", NULL },
        { "start",          PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleStartCommand,               "", NULL },
        { "taxicheat",      PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleTaxiCheatCommand,           "", NULL },
        { "trollmute",      PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleTrollmuteCommand,           "", NULL },
        { "trollmuteinfo",  PERM_HIGH_GMT,  PERM_CONSOLE, false,  &ChatHandler::HandleTrollmuteInfoCommand,       "", NULL },
        { "unaura",         PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleUnAuraCommand,              "", NULL },
        { "unbindfollow",   PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleUnbindFollowCommand,        "", NULL },
        { "unbindsight",    PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleUnbindSightCommand,         "", NULL },
        { "unfreeze",       PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleUnFreezeCommand,            "", NULL },
        { "unlearn",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleUnLearnCommand,             "", NULL },
        { "unmute",         PERM_GMT,       PERM_CONSOLE, true,   &ChatHandler::HandleUnmuteCommand,              "", NULL },
        { "unpossess",      PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleUnPossessCommand,           "", NULL },
        { "waterwalk",      PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleWaterwalkCommand,           "", NULL },
        { "wchange",        PERM_ADM,       PERM_CONSOLE, false,  &ChatHandler::HandleChangeWeather,              "", NULL },
        { "weather",        PERM_PLAYER,    PERM_CONSOLE, false,  &ChatHandler::HandleAccountWeatherCommand,      "", NULL },
        { "whispers",       PERM_GMT,       PERM_CONSOLE, false,  &ChatHandler::HandleWhispersCommand,            "", NULL },

        { NULL,             0,              0,            false,  NULL,                                           "", NULL }
    };

    if (load_command_table)
    {
        load_command_table = false;

        QueryResultAutoPtr result = GameDataDatabase.Query("SELECT name, permission_mask, self_mask, help FROM command");
        if (result)
        {
            do
            {
                Field *fields = result->Fetch();
                std::string name = fields[0].GetCppString();
                for (uint32 i = 0; commandTable[i].Name != NULL; i++)
                {
                    if (name == commandTable[i].Name)
                    {
                        commandTable[i].RequiredPermissions = fields[1].GetUInt64();
                        commandTable[i].SelfPermissions = fields[2].GetUInt64();
                        commandTable[i].Help = fields[3].GetCppString();
                    }
                    if (commandTable[i].ChildCommands != NULL)
                    {
                        ChatCommand *ptable = commandTable[i].ChildCommands;
                        for (uint32 j = 0; ptable[j].Name != NULL; j++)
                        {
                            // first case for "" named subcommand
                            if (ptable[j].Name[0]=='\0' && name == commandTable[i].Name ||
                                name == fmtstring("%s %s", commandTable[i].Name, ptable[j].Name))
                            {
                                ptable[j].RequiredPermissions = fields[1].GetUInt64();
                                ptable[j].SelfPermissions = fields[2].GetUInt64();
                                ptable[j].Help = fields[3].GetCppString();
                            }
                        }
                    }
                }
            }
            while (result->NextRow());
        }
    }

    return commandTable;
}

const char *ChatHandler::GetHellgroundString(int32 entry) const
{
    return m_session->GetHellgroundString(entry);
}

bool ChatHandler::isAvailable(ChatCommand const& cmd, bool self) const
{
    // check security level only for simple  command (without child commands)
    return m_session->HasPermissions(self ? cmd.SelfPermissions : cmd.RequiredPermissions);
}

bool ChatHandler::hasStringAbbr(const char* name, const char* part)
{
    // non "" command
    if (*name)
    {
        // "" part from non-"" command
        if (!*part)
            return false;

        for (;;)
        {
            if (!*part)
                return true;
            else if (!*name)
                return false;
            else if (tolower(*name) != tolower(*part))
                return false;
            ++name; ++part;
        }
    }
    // allow with any for ""

    return true;
}

void ChatHandler::SendSysMessage(const char *str)
{
    if (!m_session)
    {
        puts(str);
        return;
    }

    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = mangos_strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        m_session->SendPacket(&data);
    }

    delete [] buf;
}

void ChatHandler::SendGlobalSysMessage(const char *str)
{
    // Chat output
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = mangos_strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        sWorld.SendGlobalMessage(&data);
    }

    delete [] buf;
}

void ChatHandler::SendGlobalGMSysMessage(int32 entry, ...)
{
    const char *format = GetHellgroundString(entry);
    va_list ap;
    char str [1024];
    va_start(ap, entry);
    vsnprintf(str,1024,format, ap);
    va_end(ap);
    SendGlobalGMSysMessage(str);
}

void ChatHandler::SendGlobalGMSysMessage(const char *str)
{
    // Chat output
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = mangos_strdup(str);
    char* pos = buf;

    while (char* line = LineFromMessage(pos))
    {
        FillSystemMessageData(&data, line);
        sWorld.SendGlobalGMMessage(&data);
     }
    delete [] buf;
}

void ChatHandler::SendSysMessage(int32 entry)
{
    SendSysMessage(GetHellgroundString(entry));
}

void ChatHandler::PSendSysMessage(int32 entry, ...)
{
    const char *format = GetHellgroundString(entry);
    va_list ap;
    char str [1024];
    va_start(ap, entry);
    vsnprintf(str,1024,format, ap);
    va_end(ap);
    SendSysMessage(str);
}

void ChatHandler::PSendSysMessage(const char *format, ...)
{
    va_list ap;
    char str [1024];
    va_start(ap, format);
    vsnprintf(str,1024,format, ap);
    va_end(ap);
    SendSysMessage(str);
}

bool ChatHandler::ExecuteCommandInTable(ChatCommand *table, const char* text, std::string& fullcmd)
{
    char const* oldtext = text;
    std::string cmd = "";

    while (*text != ' ' && *text != '\0')
    {
        cmd += *text;
        ++text;
    }

    while (*text == ' ') ++text;

    for (uint32 i = 0; table[i].Name != NULL; i++)
    {
        if (!hasStringAbbr(table[i].Name, cmd.c_str()))
            continue;
        fullcmd += table[i].Name;
        // select subcommand from child commands list
        if (table[i].ChildCommands != NULL)
        {
            fullcmd += " ";
            if (!ExecuteCommandInTable(table[i].ChildCommands, text, fullcmd))
            {
                if (text && text[0] != '\0')
                    SendSysMessage(LANG_NO_SUBCMD);
                else
                    SendSysMessage(LANG_CMD_SYNTAX);

                ShowHelpForCommand(table[i].ChildCommands,text);
            }

            return true;
        }

        // must be available and have handler
        if (!isAvailable(table[i],false))
        {
            if(isAvailable(table[i],true))
                m_session->GetPlayer()->SetSelection(m_session->GetPlayer()->GetGUID());
            else
                continue;
        }
        if (!table[i].Handler)
            continue;

        SetSentErrorMessage(false);
        // table[i].Name == "" is special case: send original command to handler
        std::string args = strlen(table[i].Name )!=0 ? std::string(" ") + text : oldtext;
        if ((this->*(table[i].Handler))(strlen(table[i].Name)!=0 ? text : oldtext))
        {
        if (m_session && (m_session->GetPermissions() & sWorld.getConfig(CONFIG_COMMAND_LOG_PERMISSION)) && table[i].Name != "password")
            {
                Player* p = m_session->GetPlayer();
                uint64 sel_guid = p->GetSelection();
                Unit* unit = p->GetUnit(sel_guid);
                char sel_string[100];
                if(sel_guid && unit)
                    sprintf(sel_string,"%s (GUID:%u)",unit->GetName(), GUID_LOPART(sel_guid));
                else if (sel_guid)
                    sprintf(sel_string,"(GUID:%u)", GUID_LOPART(sel_guid));
                else
                    sprintf(sel_string,"NONE");

                sLog.outCommand(m_session->GetAccountId(),"Command: %s%s [Player: %s (Account: %u) X: %f Y: %f Z: %f Map: %u Selected: %s]",
                    fullcmd.c_str(),args .c_str(),
                    p->GetName(),m_session->GetAccountId(),p->GetPositionX(),p->GetPositionY(),p->GetPositionZ(),p->GetMapId(),
                    sel_string);
            }
        }
        // some commands have custom error messages. Don't send the default one in these cases.
        else if (!sentErrorMessage)
        {
            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());
            else
                SendSysMessage(LANG_CMD_SYNTAX);
        }

        return true;
    }

    return false;
}

bool ChatHandler::ContainsNotAllowedSigns(std::string text,bool strict)
{
    for (uint32 i = 0; i < text.length(); ++i)
        text[i] = tolower(text[i]);

    if ((text.find(".blp") != text.npos) || (text.find("t|t") != text.npos))
        return true;

    if (!strict)
        return false;

    for (std::string::iterator itr = text.begin(); itr != text.end(); itr++)
    {
        if (char(*itr) < 0x20)
            return true;
    }
    return false;
}

int ChatHandler::ParseCommands(const char* text)
{
    ASSERT(text);
    ASSERT(*text);

    std::string fullcmd = "";
    /// chat case (.command format)
    if (m_session)
    {
        if (text[0] != '.')
            return 0;
    }

    /// ignore single . and ! in line
    if (strlen(text) < 2)
        return 0;
    // original `text` can't be used. It content destroyed in command code processing.

    /// ignore messages staring from many dots.
    if (text[0] == '.' && text[1] == '.')
        return 0;

    /// skip first . (in console allowed use command with . and without its)
    if (text[0] == '.')
        ++text;

    if (!ExecuteCommandInTable(getCommandTable(), text, fullcmd))
    {
        if (m_session && !m_session->HasPermissions(PERM_GMT))
            return 0;
        SendSysMessage(LANG_NO_CMD);
    }
    return 1;
}

bool ChatHandler::ShowHelpForSubCommands(ChatCommand *table, char const* cmd, char const* subcmd)
{
    std::string list;
    for (uint32 i = 0; table[i].Name != NULL; ++i)
    {
        // must be available (ignore handler existence for show command with possibe avalable subcomands
        if (!isAvailable(table[i],false) && !isAvailable(table[i],true))
            continue;

        /// for empty subcmd show all available
        if (*subcmd && !hasStringAbbr(table[i].Name, subcmd))
            continue;

        if (m_session)
            list += "\n    ";
        else
            list += "\n\r    ";

        list += table[i].Name;

        if (table[i].ChildCommands)
            list += " ...";
    }

    if (list.empty())
        return false;

    if (table==getCommandTable())
    {
        SendSysMessage(LANG_AVIABLE_CMD);
        PSendSysMessage("%s",list.c_str());
    }
    else
        PSendSysMessage(LANG_SUBCMDS_LIST,cmd,list.c_str());

    return true;
}

bool ChatHandler::ShowHelpForCommand(ChatCommand *table, const char* cmd)
{
    if (*cmd)
    {
        for (uint32 i = 0; table[i].Name != NULL; ++i)
        {
            // must be available (ignore handler existence for show command with possibe avalable subcomands
            if (!isAvailable(table[i],false) && !isAvailable(table[i],true))
                continue;

            if (!hasStringAbbr(table[i].Name, cmd))
                continue;

            // have subcommand
            char const* subcmd = (*cmd) ? strtok(NULL, " ") : "";

            if (table[i].ChildCommands && subcmd && *subcmd)
            {
                if (ShowHelpForCommand(table[i].ChildCommands, subcmd))
                    return true;
            }

            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());

            if (table[i].ChildCommands)
                if (ShowHelpForSubCommands(table[i].ChildCommands,table[i].Name,subcmd ? subcmd : ""))
                    return true;

            return !table[i].Help.empty();
        }
    }
    else
    {
        for (uint32 i = 0; table[i].Name != NULL; ++i)
        {
            // must be available (ignore handler existence for show command with possibe avalable subcomands
            if (!isAvailable(table[i],false) && !isAvailable(table[i],true))
                continue;

            if (strlen(table[i].Name))
                continue;

            if (!table[i].Help.empty())
                SendSysMessage(table[i].Help.c_str());

            if (table[i].ChildCommands)
                if (ShowHelpForSubCommands(table[i].ChildCommands,"",""))
                    return true;

            return !table[i].Help.empty();
        }
    }

    return ShowHelpForSubCommands(table,"",cmd);
}

//Note: target_guid used only in CHAT_MSG_WHISPER_INFORM mode (in this case channelName ignored)
void ChatHandler::FillMessageData(WorldPacket *data, WorldSession* session, uint8 type, uint32 language, const char *channelName, uint64 target_guid, const char *message, Unit *speaker)
{
    uint32 messageLength = (message ? strlen(message) : 0) + 1;

    data->Initialize(SMSG_MESSAGECHAT, 100);                // guess size
    *data << uint8(type);
    if ((type != CHAT_MSG_CHANNEL && type != CHAT_MSG_WHISPER) || language == LANG_ADDON)
        *data << uint32(language);
    else
        *data << uint32(LANG_UNIVERSAL);

    switch (type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_RAID:
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_YELL:
        case CHAT_MSG_WHISPER:
        case CHAT_MSG_CHANNEL:
        case CHAT_MSG_RAID_LEADER:
        case CHAT_MSG_RAID_WARNING:
        case CHAT_MSG_BG_SYSTEM_NEUTRAL:
        case CHAT_MSG_BG_SYSTEM_ALLIANCE:
        case CHAT_MSG_BG_SYSTEM_HORDE:
        case CHAT_MSG_BATTLEGROUND:
        case CHAT_MSG_BATTLEGROUND_LEADER:
            target_guid = session ? session->GetPlayer()->GetGUID() : 0;
            break;
        case CHAT_MSG_MONSTER_SAY:
        case CHAT_MSG_MONSTER_PARTY:
        case CHAT_MSG_MONSTER_YELL:
        case CHAT_MSG_MONSTER_WHISPER:
        case CHAT_MSG_MONSTER_EMOTE:
        case CHAT_MSG_RAID_BOSS_WHISPER:
        case CHAT_MSG_RAID_BOSS_EMOTE:
        {
            *data << uint64(speaker->GetGUID());
            *data << uint32(0);                             // 2.1.0
            *data << uint32(strlen(speaker->GetName()) + 1);
            *data << speaker->GetName();
            uint64 listener_guid = 0;
            *data << uint64(listener_guid);
            if (listener_guid && !IS_PLAYER_GUID(listener_guid))
            {
                *data << uint32(1);                         // string listener_name_length
                *data << uint8(0);                          // string listener_name
            }
            *data << uint32(messageLength);
            *data << message;
            *data << uint8(0);
            return;
        }
        default:
            if (type != CHAT_MSG_REPLY && type != CHAT_MSG_IGNORED && type != CHAT_MSG_DND && type != CHAT_MSG_AFK)
                target_guid = 0;                            // only for CHAT_MSG_WHISPER_INFORM used original value target_guid
            break;
    }

    *data << uint64(target_guid);                           // there 0 for BG messages
    *data << uint32(0);                                     // can be chat msg group or something

    if (type == CHAT_MSG_CHANNEL)
    {
        ASSERT(channelName);
        *data << channelName;
    }

    *data << uint64(target_guid);
    *data << uint32(messageLength);
    *data << message;
    if (session != 0 && type != CHAT_MSG_REPLY && type != CHAT_MSG_DND && type != CHAT_MSG_AFK)
        *data << uint8(session->GetPlayer()->chatTag());
    else
        *data << uint8(0);
}

Player * ChatHandler::getSelectedPlayer()
{
    if (!m_session)
        return NULL;

    uint64 guid  = m_session->GetPlayer()->GetSelection();

    if (guid == 0)
        return m_session->GetPlayer();

    return sObjectMgr.GetPlayer(guid);
}

Unit* ChatHandler::getSelectedUnit()
{
    if (!m_session)
        return NULL;

    uint64 guid = m_session->GetPlayer()->GetSelection();

    if (guid == 0)
        return m_session->GetPlayer();

    return m_session->GetPlayer()->GetMap()->GetUnit(guid);
}

Creature* ChatHandler::getSelectedCreature()
{
    if (!m_session)
        return NULL;

    Player * tmp = m_session->GetPlayer();

    return tmp->GetMap()->GetCreatureOrPet(tmp->GetSelection());
}

/**
 * Function skip all whitespaces in args string
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *             allowed nullptr string pointer stored in *args
 */
void ChatHandler::SkipWhiteSpaces(char** args)
{
    if (!*args)
        return;

    while (isWhiteSpace(**args))
        ++(*args);
}

/**
 * Function extract to val arg signed integer value or fail
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val  return extracted value if function success, in fail case original value unmodified
 * @return     true if value extraction successful
 */
bool  ChatHandler::ExtractInt32(char** args, int32& val)
{
    if (!*args || !**args)
        return false;

    char* tail = *args;

    long valRaw = strtol(*args, &tail, 10);

    if (tail != *args && isWhiteSpace(*tail))
        *(tail++) = '\0';
    else if (tail && *tail)                                 // some not whitespace symbol
        return false;                                       // args not modified and can be re-parsed

    if (valRaw < std::numeric_limits<int32>::min() || valRaw > std::numeric_limits<int32>::max())
        return false;

    // value successfully extracted
    val = int32(valRaw);
    *args = tail;
    return true;
}

/**
 * Function extract to val arg optional signed integer value or use default value. Fail if extracted not signed integer.
 *
 * @param args    variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val     return extracted value if function success, in fail case original value unmodified
 * @param defVal  default value used if no data for extraction in args
 * @return        true if value extraction successful
 */
bool  ChatHandler::ExtractOptInt32(char** args, int32& val, int32 defVal)
{
    if (!*args || !**args)
    {
        val = defVal;
        return true;
    }

    return ExtractInt32(args, val);
}

/**
 * Function extract to val arg unsigned integer value or fail
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val  return extracted value if function success, in fail case original value unmodified
 * @param base set used base for extracted value format (10 for decimal, 16 for hex, etc), 0 let auto select by system internal function
 * @return     true if value extraction successful
 */
bool  ChatHandler::ExtractUInt32Base(char** args, uint32& val, uint32 base)
{
    if (!*args || !**args)
        return false;

    char* tail = *args;

    unsigned long valRaw = strtoul(*args, &tail, base);

    if (tail != *args && isWhiteSpace(*tail))
        *(tail++) = '\0';
    else if (tail && *tail)                                 // some not whitespace symbol
        return false;                                       // args not modified and can be re-parsed

    if (valRaw > std::numeric_limits<uint32>::max())
        return false;

    // value successfully extracted
    val = uint32(valRaw);
    *args = tail;

    SkipWhiteSpaces(args);
    return true;
}

/**
 * Function extract to val arg optional unsigned integer value or use default value. Fail if extracted not unsigned integer.
 *
 * @param args    variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val     return extracted value if function success, in fail case original value unmodified
 * @param defVal  default value used if no data for extraction in args
 * @return        true if value extraction successful
 */
bool  ChatHandler::ExtractOptUInt32(char** args, uint32& val, uint32 defVal)
{
    if (!*args || !**args)
    {
        val = defVal;
        return true;
    }

    return ExtractUInt32(args, val);
}

/**
 * Function extract to val arg float value or fail
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val  return extracted value if function success, in fail case original value unmodified
 * @return     true if value extraction successful
 */
bool  ChatHandler::ExtractFloat(char** args, float& val)
{
    if (!*args || !**args)
        return false;

    char* tail = *args;

    double valRaw = strtod(*args, &tail);

    if (tail != *args && isWhiteSpace(*tail))
        *(tail++) = '\0';
    else if (tail && *tail)                                 // some not whitespace symbol
        return false;                                       // args not modified and can be re-parsed

    // value successfully extracted
    val = float(valRaw);
    *args = tail;

    SkipWhiteSpaces(args);
    return true;
}

/**
 * Function extract to val arg optional float value or use default value. Fail if extracted not float.
 *
 * @param args    variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val     return extracted value if function success, in fail case original value unmodified
 * @param defVal  default value used if no data for extraction in args
 * @return        true if value extraction successful
 */
bool  ChatHandler::ExtractOptFloat(char** args, float& val, float defVal)
{
    if (!*args || !**args)
    {
        val = defVal;
        return true;
    }

    return ExtractFloat(args, val);
}

/**
 * Function extract name-like string (from non-numeric or special symbol until whitespace)
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param lit  optional explicit literal requirement. function fail if literal is not starting substring of lit.
 *             Note: function in same way fail if no any literal or literal not fit in this case. Need additional check for select specific fail case
 * @return     name/number-like string without whitespaces, or nullptr if args empty or not appropriate content.
 */
char* ChatHandler::ExtractLiteralArg(char** args, char const* lit /*= nullptr*/)
{
    if (!*args || !**args)
        return nullptr;

    char* head = *args;

    // reject quoted string or link (|-started text)
    switch (head[0])
    {
        // reject quoted string
        case '[':
        case '\'':
        case '"':
            return nullptr;
        // reject link (|-started text)
        case '|':
            // client replace all | by || in raw text
            if (head[1] != '|')
                return nullptr;
            ++head;                                         // skip one |
            break;
        default:
            break;
    }

    if (lit)
    {
        int l = strlen(lit);
        int diff = strncmp(head, lit, l);

        if (diff != 0)
            return nullptr;

        if (head[l] && !isWhiteSpace(head[l]))
            return nullptr;

        char* arg = head;

        if (head[l])
        {
            head[l] = '\0';

            head += l + 1;

            *args = head;
        }
        else
            *args = head + l;

        SkipWhiteSpaces(args);
        return arg;
    }

    char* name = strtok(head, " ");

    char* tail = strtok(nullptr, "");

    *args = tail ? tail : (char*)"";                        // *args don't must be nullptr

    SkipWhiteSpaces(args);

    return name;
}

/**
 * Function extract quote-like string (any characters guarded by some special character, in our cases ['")
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param asis control save quote string wrappers
 * @return     quote-like string, or nullptr if args empty or not appropriate content.
 */
char* ChatHandler::ExtractQuotedArg(char** args, bool asis /*= false*/)
{
    if (!*args || !**args)
        return nullptr;

    if (**args != '\'' && **args != '"' && **args != '[')
        return nullptr;

    char guard = (*args)[0];

    if (guard == '[')
        guard = ']';

    char* tail = (*args) + 1;                               // start scan after first quote symbol
    char* head = asis ? *args : tail;                       // start arg

    while (*tail && *tail != guard)
        ++tail;

    if (!*tail || (tail[1] && !isWhiteSpace(tail[1])))      // fail
        return nullptr;

    if (!tail[1])                                           // quote is last char in string
    {
        if (!asis)
            *tail = '\0';
    }
    else                                                    // quote isn't last char
    {
        if (asis)
            ++tail;

        *tail = '\0';
    }

    *args = tail + 1;

    SkipWhiteSpaces(args);

    return head;
}

/**
 * Function extract quote-like string or literal if quote not detected
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param asis control save quote string wrappers
 * @return     quote/literal string, or nullptr if args empty or not appropriate content.
 */
char* ChatHandler::ExtractQuotedOrLiteralArg(char** args, bool asis /*= false*/)
{
    char *arg = ExtractQuotedArg(args, asis);
    if (!arg)
        arg = ExtractLiteralArg(args);
    return arg;
}

/**
 * Function extract on/off literals as boolean values
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param val  return extracted value if function success, in fail case original value unmodified
 * @return     true at success
 */
bool  ChatHandler::ExtractOnOff(char** args, bool& value)
{
    char* arg = ExtractLiteralArg(args);
    if (!arg)
        return false;

    if (strncmp(arg, "on", 3) == 0 || strncmp(arg, "ON", 3) == 0)
        value = true;
    else if (strncmp(arg, "off", 4) == 0 || strncmp(arg, "OFF", 4) == 0)
        value = false;
    else
        return false;

    return true;
}

/**
 * Function extract shift-link-like string (any characters guarded by | and |h|r with some additional internal structure check)
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *
 * @param linkTypes  optional nullptr-terminated array of link types, shift-link must fit one from link type from array if provided or extraction fail
 *
 * @param found_idx  if not nullptr then at return index in linkTypes that fit shift-link type, if extraction fail then non modified
 *
 * @param keyPair    if not nullptr then pointer to 2-elements array for return start and end pointer for found key
 *                   if extraction fail then non modified
 *
 * @param somethingPair then pointer to 2-elements array for return start and end pointer if found.
 *                   if not nullptr then shift-link must have data field, if extraction fail then non modified
 *
 * @return     shift-link-like string, or nullptr if args empty or not appropriate content.
 */
char* ChatHandler::ExtractLinkArg(char** args, char const* const* linkTypes /*= nullptr*/, int* foundIdx /*= nullptr*/, char** keyPair /*= nullptr*/, char** somethingPair /*= nullptr*/)
{
    if (!*args || !**args)
        return nullptr;

    // skip if not linked started or encoded single | (doubled by client)
    if ((*args)[0] != '|' || (*args)[1] == '|')
        return nullptr;

    // |color|Hlinktype:key:data...|h[name]|h|r

    char* head = *args;

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r
    // or
    // [name] Shift-click form |linkType:key|h[name]|h|r

    // |color|Hlinktype:key:data...|h[name]|h|r

    char* tail = (*args) + 1;                               // skip |

    if (*tail != 'H')                                       // skip color part, some links can not have color part
    {
        while (*tail && *tail != '|')
            ++tail;

        if (!*tail)
            return nullptr;

        // |Hlinktype:key:data...|h[name]|h|r

        ++tail;                                             // skip |
    }

    // Hlinktype:key:data...|h[name]|h|r

    if (*tail != 'H')
        return nullptr;

    int linktype_idx = 0;

    if (linkTypes)                                          // check link type if provided
    {
        // check linktypes (its include H in name)
        for (; linkTypes[linktype_idx]; ++linktype_idx)
        {
            // exactly string with follow : or |
            int l = strlen(linkTypes[linktype_idx]);
            if (strncmp(tail, linkTypes[linktype_idx], l) == 0 &&
                    (tail[l] == ':' || tail[l] == '|'))
                break;
        }

        // is search fail?
        if (!linkTypes[linktype_idx])                       // nullptr terminator in last element
            return nullptr;

        tail += strlen(linkTypes[linktype_idx]);            // skip linktype string

        // :key:data...|h[name]|h|r

        if (*tail != ':')
            return nullptr;
    }
    else
    {
        while (*tail && *tail != ':')                       // skip linktype string
            ++tail;

        if (!*tail)
            return nullptr;
    }

    ++tail;

    // key:data...|h[name]|h|r
    char* keyStart = tail;                                  // remember key start for return
    char* keyEnd   = tail;                                  // key end for truncate, will updated

    while (*tail && *tail != '|' && *tail != ':')
        ++tail;

    if (!*tail)
        return nullptr;

    keyEnd = tail;                                          // remember key end for truncate

    // |h[name]|h|r or :something...|h[name]|h|r

    char* somethingStart = tail + 1;
    char* somethingEnd   = tail + 1;                        // will updated later if need

    if (*tail == ':' && somethingPair)                      // optional data extraction
    {
        // :something...|h[name]|h|r
        ++tail;

        // something|h[name]|h|r or something:something2...|h[name]|h|r

        while (*tail && *tail != '|' && *tail != ':')
            ++tail;

        if (!*tail)
            return nullptr;

        somethingEnd = tail;                                // remember data end for truncate
    }

    // |h[name]|h|r or :something2...|h[name]|h|r

    while (*tail && (*tail != '|' || *(tail + 1) != 'h'))   // skip ... part if exist
        ++tail;

    if (!*tail)
        return nullptr;

    // |h[name]|h|r

    tail += 2;                                              // skip |h

    // [name]|h|r
    if (!*tail || *tail != '[')
        return nullptr;

    while (*tail && (*tail != ']' || *(tail + 1) != '|'))   // skip name part
        ++tail;

    tail += 2;                                              // skip ]|

    // h|r
    if (!*tail || *tail != 'h'  || *(tail + 1) != '|')
        return nullptr;

    tail += 2;                                              // skip h|

    // r
    if (!*tail || *tail != 'r' || (*(tail + 1) && !isWhiteSpace(*(tail + 1))))
        return nullptr;

    ++tail;                                                 // skip r

    // success

    if (*tail)                                              // truncate all link string
        *(tail++) = '\0';

    if (foundIdx)
        *foundIdx = linktype_idx;

    if (keyPair)
    {
        keyPair[0] = keyStart;
        keyPair[1] = keyEnd;
    }

    if (somethingPair)
    {
        somethingPair[0] = somethingStart;
        somethingPair[1] = somethingEnd;
    }

    *args = tail;

    SkipWhiteSpaces(args);

    return head;
}

/**
 * Function extract name/number/quote/shift-link-like string
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 * @param asis control save quote string wrappers
 * @return     extracted arg string, or nullptr if args empty or not appropriate content.
 */
char* ChatHandler::ExtractArg(char** args, bool asis /*= false*/)
{
    if (!*args || !**args)
        return nullptr;

    char* arg = ExtractQuotedOrLiteralArg(args, asis);
    if (!arg)
        arg = ExtractLinkArg(args);

    return arg;
}

/**
 * Function extract name/quote/number/shift-link-like string, and return it if args have more non-whitespace data
 *
 * @param args variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *             if args have only single arg then args still pointing to this arg (unmodified pointer)
 * @return     extracted string, or nullptr if args empty or not appropriate content or have single arg totally.
 */
char* ChatHandler::ExtractOptNotLastArg(char** args)
{
    char* arg = ExtractArg(args, true);

    // have more data
    if (*args && **args)
        return arg;

    // optional name not found
    *args = arg ? arg : (char*)"";                          // *args don't must be nullptr

    return nullptr;
}

/**
 * Function extract data from shift-link "|color|LINKTYPE:RETURN:SOMETHING1|h[name]|h|r if linkType == LINKTYPE
 * It also extract literal/quote if not shift-link in args
 *
 * @param args       variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *                   if args have sift link with linkType != LINKTYPE then args still pointing to this arg (unmodified pointer)
 *
 * @param linkType   shift-link must fit by link type to this arg value or extraction fail
 *
 * @param something1 if not nullptr then shift-link must have data field and it returned into this arg
 *                   if extraction fail then non modified
 *
 * @return           extracted key, or nullptr if args empty or not appropriate content or not fit to linkType.
 */
char* ChatHandler::ExtractKeyFromLink(char** text, char const* linkType, char** something1 /*= nullptr*/)
{
    char const* linkTypes[2];
    linkTypes[0] = linkType;
    linkTypes[1] = nullptr;

    int foundIdx;

    return ExtractKeyFromLink(text, linkTypes, &foundIdx, something1);
}

/**
 * Function extract data from shift-link "|color|LINKTYPE:RETURN:SOMETHING1|h[name]|h|r if LINKTYPE in linkTypes array
 * It also extract literal/quote if not shift-link in args
 *
 * @param args       variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *                   if args have sift link with linkType != LINKTYPE then args still pointing to this arg (unmodified pointer)
 *
 * @param linkTypes  nullptr-terminated array of link types, shift-link must fit one from link type from array or extraction fail
 *
 * @param found_idx  if not nullptr then at return index in linkTypes that fit shift-link type, for non-link case return -1
 *                   if extraction fail then non modified
 *
 * @param something1 if not nullptr then shift-link must have data field and it returned into this arg
 *                   if extraction fail then non modified
 *
 * @return           extracted key, or nullptr if args empty or not appropriate content or not fit to linkType.
 */
char* ChatHandler::ExtractKeyFromLink(char** text, char const* const* linkTypes, int* found_idx, char** something1 /*= nullptr*/)
{
    // skip empty
    if (!*text || !**text)
        return nullptr;

    // return non link case
    char* arg = ExtractQuotedOrLiteralArg(text);
    if (arg)
    {
        if (found_idx)
            *found_idx = -1;                                // special index case

        return arg;
    }

    char* keyPair[2];
    char* somethingPair[2];

    arg = ExtractLinkArg(text, linkTypes, found_idx, keyPair, something1 ? somethingPair : nullptr);
    if (!arg)
        return nullptr;

    *keyPair[1] = '\0';                                     // truncate key string

    if (something1)
    {
        *somethingPair[1] = '\0';                           // truncate data string
        *something1 = somethingPair[0];
    }

    return keyPair[0];
}

/**
 * Function extract uint32 key from shift-link "|color|LINKTYPE:RETURN|h[name]|h|r if linkType == LINKTYPE
 * It also extract direct number if not shift-link in args
 *
 * @param args       variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *                   if args have sift link with linkType != LINKTYPE then args still pointing to this arg (unmodified pointer)
 *
 * @param linkType   shift-link must fit by link type to this arg value or extraction fail
 *
 * @param value      store result value at success return, not modified at fail
 *
 * @return           true if extraction succesful
 */
bool ChatHandler::ExtractUint32KeyFromLink(char** text, char const* linkType, uint32& value)
{
    char* arg = ExtractKeyFromLink(text, linkType);
    if (!arg)
        return false;

    return ExtractUInt32(&arg, value);
}

enum SpellLinkType
{
    SPELL_LINK_RAW     = -1,                                // non-link case
    SPELL_LINK_SPELL   = 0,
    SPELL_LINK_TALENT  = 1,
    SPELL_LINK_ENCHANT = 2,
};

static char const* const spellKeys[] =
{
    "Hspell",                                               // normal spell
    "Htalent",                                              // talent spell
    "Henchant",                                             // enchanting recipe spell
    nullptr
};

uint32 ChatHandler::ExtractSpellIdFromLink(char** text)
{
    // number or [name] Shift-click form |color|Henchant:recipe_spell_id|h[prof_name: recipe_name]|h|r
    // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r
    // number or [name] Shift-click form |color|Htalent:talent_id,rank|h[name]|h|r
    int type;
    char* param1_str = nullptr;
    char* idS = ExtractKeyFromLink(text, spellKeys, &type, &param1_str);
    if (!idS)
        return 0;

    uint32 id;
    if (!ExtractUInt32(&idS, id))
        return 0;

    switch (type)
    {
        case SPELL_LINK_RAW:
        case SPELL_LINK_SPELL:
        case SPELL_LINK_ENCHANT:
            return id;
        case SPELL_LINK_TALENT:
        {
            // talent
            TalentEntry const* talentEntry = sTalentStore.LookupEntry(id);
            if (!talentEntry)
                return 0;

            int32 rank;
            if (!ExtractInt32(&param1_str, rank))
                return 0;

            if (rank < 0)                                   // unlearned talent have in shift-link field -1 as rank
                rank = 0;

            return rank < MAX_TALENT_RANK ? talentEntry->RankID[rank] : 0;
        }
    }

    // unknown type?
    return 0;
}

GameTele const* ChatHandler::ExtractGameTeleFromLink(char** text)
{
    // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
    char* cId = ExtractKeyFromLink(text, "Htele");
    if (!cId)
        return nullptr;

    // id case (explicit or from shift link)
    uint32 id;
    if (ExtractUInt32(&cId, id))
        return sObjectMgr.GetGameTele(id);
    else
        return sObjectMgr.GetGameTele(cId);
}

enum GuidLinkType
{
    GUID_LINK_RAW        = -1,                              // non-link case
    GUID_LINK_PLAYER     = 0,
    GUID_LINK_CREATURE   = 1,
    GUID_LINK_GAMEOBJECT = 2,
};

static char const* const guidKeys[] =
{
    "Hplayer",
    "Hcreature",
    "Hgameobject",
    nullptr
};

ObjectGuid ChatHandler::ExtractGuidFromLink(char** text)
{
    int type = 0;

    // |color|Hcreature:creature_guid|h[name]|h|r
    // |color|Hgameobject:go_guid|h[name]|h|r
    // |color|Hplayer:name|h[name]|h|r
    char* idS = ExtractKeyFromLink(text, guidKeys, &type);
    if (!idS)
        return ObjectGuid();

    switch (type)
    {
        case GUID_LINK_RAW:
        case GUID_LINK_PLAYER:
        {
            std::string name = idS;
            if (!normalizePlayerName(name))
                return ObjectGuid();

            if (Player* player = sObjectMgr.GetPlayer(name.c_str()))
                return player->GetObjectGuid();

            return ObjectGuid(sObjectMgr.GetPlayerGUIDByName(name));
        }
        case GUID_LINK_CREATURE:
        {
            uint32 lowguid;
            if (!ExtractUInt32(&idS, lowguid))
                return ObjectGuid();

            if (CreatureData const* data = sObjectMgr.GetCreatureData(lowguid))
                return data->GetObjectGuid(lowguid);
            else
                return ObjectGuid();
        }
        case GUID_LINK_GAMEOBJECT:
        {
            uint32 lowguid;
            if (!ExtractUInt32(&idS, lowguid))
                return ObjectGuid();

            if (GameObjectData const* data = sObjectMgr.GetGOData(lowguid))
                return ObjectGuid(HIGHGUID_GAMEOBJECT, data->id, lowguid);
            else
                return ObjectGuid();
        }
    }

    // unknown type?
    return ObjectGuid();
}

enum LocationLinkType
{
    LOCATION_LINK_RAW               = -1,                   // non-link case
    LOCATION_LINK_PLAYER            = 0,
    LOCATION_LINK_TELE              = 1,
    LOCATION_LINK_TAXINODE          = 2,
    LOCATION_LINK_CREATURE          = 3,
    LOCATION_LINK_GAMEOBJECT        = 4,
    LOCATION_LINK_CREATURE_ENTRY    = 5,
    LOCATION_LINK_GAMEOBJECT_ENTRY  = 6,
    LOCATION_LINK_AREATRIGGER       = 7,
    LOCATION_LINK_AREATRIGGER_TARGET = 8,
};

static char const* const locationKeys[] =
{
    "Htele",
    "Htaxinode",
    "Hplayer",
    "Hcreature",
    "Hgameobject",
    "Hcreature_entry",
    "Hgameobject_entry",
    "Hareatrigger",
    "Hareatrigger_target",
    nullptr
};

bool ChatHandler::ExtractLocationFromLink(char** text, uint32& mapid, float& x, float& y, float& z)
{
    int type = 0;

    // |color|Hplayer:name|h[name]|h|r
    // |color|Htele:id|h[name]|h|r
    // |color|Htaxinode:id|h[name]|h|r
    // |color|Hcreature:creature_guid|h[name]|h|r
    // |color|Hgameobject:go_guid|h[name]|h|r
    // |color|Hcreature_entry:creature_id|h[name]|h|r
    // |color|Hgameobject_entry:go_id|h[name]|h|r
    // |color|Hareatrigger:id|h[name]|h|r
    // |color|Hareatrigger_target:id|h[name]|h|r
    char* idS = ExtractKeyFromLink(text, locationKeys, &type);
    if (!idS)
        return false;

    switch (type)
    {
        case LOCATION_LINK_RAW:
        case LOCATION_LINK_PLAYER:
        {
            std::string name = idS;
            if (!normalizePlayerName(name))
                return false;

            if (Player* player = sObjectMgr.GetPlayer(name.c_str()))
            {
                mapid = player->GetMapId();
                x = player->GetPositionX();
                y = player->GetPositionY();
                z = player->GetPositionZ();
                return true;
            }

            if (uint64 guid = sObjectMgr.GetPlayerGUIDByName(name))
            {
                // to point where player stay (if loaded)
                float o;
                bool in_flight;
                return Player::LoadPositionFromDB(mapid, x, y, z, o, in_flight, guid);
            }

            return false;
        }
        case LOCATION_LINK_TELE:
        {
            uint32 id;
            if (!ExtractUInt32(&idS, id))
                return false;

            GameTele const* tele = sObjectMgr.GetGameTele(id);
            if (!tele)
                return false;
            mapid = tele->mapId;
            x = tele->position_x;
            y = tele->position_y;
            z = tele->position_z;
            return true;
        }
        case LOCATION_LINK_TAXINODE:
        {
            uint32 id;
            if (!ExtractUInt32(&idS, id))
                return false;

            TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
            if (!node)
                return false;
            mapid = node->map_id;
            x = node->x;
            y = node->y;
            z = node->z;
            return true;
        }
        case LOCATION_LINK_CREATURE:
        {
            uint32 lowguid;
            if (!ExtractUInt32(&idS, lowguid))
                return false;

            if (CreatureData const* data = sObjectMgr.GetCreatureData(lowguid))
            {
                mapid = data->mapid;
                x = data->posX;
                y = data->posY;
                z = data->posZ;
                return true;
            }
            else
                return false;
        }
        case LOCATION_LINK_GAMEOBJECT:
        {
            uint32 lowguid;
            if (!ExtractUInt32(&idS, lowguid))
                return false;

            if (GameObjectData const* data = sObjectMgr.GetGOData(lowguid))
            {
                mapid = data->mapid;
                x = data->posX;
                y = data->posY;
                z = data->posZ;
                return true;
            }
            else
                return false;
        }
        case LOCATION_LINK_CREATURE_ENTRY:
        {
            uint32 id;
            if (!ExtractUInt32(&idS, id))
                return false;

            /*if (ObjectMgr::GetCreatureTemplate(id))
            {
                FindCreatureData worker(id, m_session ? m_session->GetPlayer() : nullptr);

                sObjectMgr.DoCreatureData(worker);

                if (CreatureDataPair const* dataPair = worker.GetResult())
                {
                    mapid = dataPair->second.position.mapId;
                    x = dataPair->second.position.x;
                    y = dataPair->second.position.y;
                    z = dataPair->second.position.z;
                    return true;
                }
                else
                    return false;
            }
            else*/
                return false;
        }
        case LOCATION_LINK_GAMEOBJECT_ENTRY:
        {
            uint32 id;
            if (!ExtractUInt32(&idS, id))
                return false;

            /*
            if (ObjectMgr::GetGameObjectInfo(id))
            {
                FindGOData worker(id, m_session ? m_session->GetPlayer() : nullptr);

                sObjectMgr.DoGOData(worker);

                if (GameObjectDataPair const* dataPair = worker.GetResult())
                {
                    mapid = dataPair->second.position.mapId;
                    x = dataPair->second.position.x;
                    y = dataPair->second.position.y;
                    z = dataPair->second.position.z;
                    return true;
                }
                else
                    return false;
            }
            else*/
                return false;
        }
        case LOCATION_LINK_AREATRIGGER:
        {
            uint32 id;
            if (!ExtractUInt32(&idS, id))
                return false;

            AreaTrigger const* atEntry = sObjectMgr.GetAreaTrigger(id);
            if (!atEntry)
            {
                PSendSysMessage(LANG_COMMAND_GOAREATRNOTFOUND, id);
                SetSentErrorMessage(true);
                return false;
            }

            mapid = atEntry->target_mapId;
            x = atEntry->target_X;
            y = atEntry->target_Y;
            z = atEntry->target_Z;
            return true;
        }
        case LOCATION_LINK_AREATRIGGER_TARGET:
        {
            uint32 id;
            if (!ExtractUInt32(&idS, id))
                return false;

            if (!sObjectMgr.GetAreaTrigger(id))
            {
                PSendSysMessage(LANG_COMMAND_GOAREATRNOTFOUND, id);
                SetSentErrorMessage(true);
                return false;
            }

            /*AreaTriggerTeleport const* at = sObjectMgr.GetAreaTriggerTeleport(id);
            if (!at)
            {
                PSendSysMessage(LANG_AREATRIGER_NOT_HAS_TARGET, id);
                SetSentErrorMessage(true);
                return false;
            }

            mapid = at->destination.mapId;
            x = at->destination.x;
            y = at->destination.y;
            z = at->destination.z;
            return true;*/
            return false;
        }
    }

    // unknown type?
    return false;
}

std::string ChatHandler::ExtractPlayerNameFromLink(char** text)
{
    // |color|Hplayer:name|h[name]|h|r
    char* name_str = ExtractKeyFromLink(text, "Hplayer");
    if (!name_str)
        return "";

    std::string name = name_str;
    if (!normalizePlayerName(name))
        return "";

    return name;
}

/**
 * Function extract at least one from request player data (pointer/guid/name) from args name/shift-link or selected player if no args
 *
 * @param args        variable pointer to non parsed args string, updated at function call to new position (with skipped white spaces)
 *
 * @param player      optional arg   One from 3 optional args must be provided at least (or more).
 * @param player_guid optional arg   For function success only one from provided args need get result
 * @param player_name optional arg   But if early arg get value then all later args will have its (if requested)
 *                                   if player_guid requested and not found then name also will not found
 *                                   So at success can be returned 2 cases: (player/guid/name) or (guid/name)
 *
 * @return           true if extraction successful
 */
bool ChatHandler::ExtractPlayerTarget(char** args, Player** player /*= nullptr*/, ObjectGuid* player_guid /*= nullptr*/, std::string* player_name /*= nullptr*/, bool use_extended_response)
{
    std::string name;
    if (*args && **args)
    {
        name = ExtractPlayerNameFromLink(args);
        if (name.empty())
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        Player* pl = sObjectMgr.GetPlayer(name.c_str());

        // if allowed player pointer
        if (player)
            *player = pl;

        // if need guid value from DB (in name case for check player existence)
        ObjectGuid guid = !pl && (player_guid || player_name) ? ObjectGuid(sObjectMgr.GetPlayerGUIDByName(name)) : ObjectGuid();

        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetObjectGuid() : guid;

        if (player_name)
            *player_name = pl || guid ? name : "";
    }
    else
    {
        Player* pl = getSelectedPlayer();
        // if allowed player pointer
        if (player)
            *player = pl;
        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetObjectGuid() : ObjectGuid();

        if (player_name)
            *player_name = pl ? pl->GetName() : "";
    }

    // some from req. data must be provided (note: name is empty if player not exist)
    if ((!player || !*player) && (!player_guid || !*player_guid) && (!player_name || player_name->empty()))
    {
        if (use_extended_response && !name.empty())
        {
            std::string message(GetHellgroundString(LANG_PLAYER_NOT_FOUND));
            message.append(" (" + name + ")");
            SendSysMessage(message.c_str());
        }
        else
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
        }
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

uint32 ChatHandler::ExtractAccountId(char** args, std::string* accountName /*= nullptr*/, Player** targetIfNullArg /*= nullptr*/)
{
    uint32 account_id = 0;

    ///- Get the account name from the command line
    char* account_str = ExtractLiteralArg(args);

    if (!account_str)
    {
        if (!targetIfNullArg)
            return 0;

        /// only target player different from self allowed (if targetPlayer!=nullptr then not console)
        Player* targetPlayer = getSelectedPlayer();
        if (!targetPlayer)
            return 0;

        account_id = targetPlayer->GetSession()->GetAccountId();
        
        if (accountName)
            account_id = sObjectMgr.GetPlayerAccountIdByPlayerName(*accountName);

        if (targetIfNullArg)
            *targetIfNullArg = targetPlayer;

        return account_id;
    }

    std::string account_name;

    if (ExtractUInt32(&account_str, account_id))
    {
        if (!AccountMgr::GetName(account_id, account_name))
        {
            PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_str);
            SetSentErrorMessage(true);
            return 0;
        }
    }
    else
    {
        account_name = account_str;
        if (!AccountMgr::normalizeString(account_name))
        {
            PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            SetSentErrorMessage(true);
            return 0;
        }

        account_id = AccountMgr::GetId(account_name);
        if (!account_id)
        {
            PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            SetSentErrorMessage(true);
            return 0;
        }
    }

    if (accountName)
        *accountName = account_name;

    if (targetIfNullArg)
        *targetIfNullArg = nullptr;

    return account_id;
}

struct RaceMaskName
{
    char const* literal;
    uint32 raceMask;
};

static RaceMaskName const raceMaskNames[] =
{
    // races
    { "human", (1 << (RACE_HUMAN - 1))   },
    { "orc", (1 << (RACE_ORC - 1))     },
    { "dwarf", (1 << (RACE_DWARF - 1))   },
    { "nightelf", (1 << (RACE_NIGHTELF - 1))},
    { "undead", (1 << (RACE_UNDEAD_PLAYER - 1))  },
    { "tauren", (1 << (RACE_TAUREN - 1))  },
    { "gnome", (1 << (RACE_GNOME - 1))   },
    { "troll", (1 << (RACE_TROLL - 1))   },

    // masks
    { "alliance", RACEMASK_ALLIANCE },
    { "horde",    RACEMASK_HORDE },
    { "all", RACEMASK_ALL_PLAYABLE },

    // terminator
    {nullptr, 0 }
};

bool ChatHandler::ExtractRaceMask(char** text, uint32& raceMask, char const** maskName /*=nullptr*/)
{
    if (ExtractUInt32(text, raceMask))
    {
        if (maskName)
            *maskName = "custom mask";
    }
    else
    {
        for (RaceMaskName const* itr = raceMaskNames; itr->literal; ++itr)
        {
            if (ExtractLiteralArg(text, itr->literal))
            {
                raceMask = itr->raceMask;

                if (maskName)
                    *maskName = itr->literal;
                break;
            }
        }

        if (!raceMask)
            return false;
    }

    return true;
}

char* ChatHandler::extractKeyFromLink(char* text, char const* linkType, char** something1)
{
    // skip empty
    if (!text)
        return NULL;

    // skip spaces
    while (*text==' '||*text=='\t'||*text=='\b')
        ++text;

    if (!*text)
        return NULL;

    // return non link case
    if (text[0]!='|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r

    char* check = strtok(text, "|");                        // skip color
    if (!check)
        return NULL;                                        // end of data

    char* cLinkType = strtok(NULL, ":");                    // linktype
    if (!cLinkType)
        return NULL;                                        // end of data

    if (strcmp(cLinkType,linkType) != 0)
    {
        strtok(NULL, " ");                                  // skip link tail (to allow continue strtok(NULL,s) use after retturn from function
        SendSysMessage(LANG_WRONG_LINK_TYPE);
        return NULL;
    }

    char* cKeys = strtok(NULL, "|");                        // extract keys and values
    char* cKeysTail = strtok(NULL, "");

    char* cKey = strtok(cKeys, ":|");                       // extract key
    if (something1)
        *something1 = strtok(NULL, ":|");                   // extract something

    strtok(cKeysTail, "]");                                 // restart scan tail and skip name with possible spaces
    strtok(NULL, " ");                                      // skip link tail (to allow continue strtok(NULL,s) use after retturn from function
    return cKey;
}

char* ChatHandler::extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1)
{
    // skip empty
    if (!text)
        return NULL;

    // skip spaces
    while (*text==' '||*text=='\t'||*text=='\b')
        ++text;

    if (!*text)
        return NULL;

    // return non link case
    if (text[0]!='|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r

    char* check = strtok(text, "|");                        // skip color
    if (!check)
        return NULL;                                        // end of data

    char* cLinkType = strtok(NULL, ":");                    // linktype
    if (!cLinkType)
        return NULL;                                        // end of data

    for (int i = 0; linkTypes[i]; ++i)
    {
        if (strcmp(cLinkType,linkTypes[i]) == 0)
        {
            char* cKeys = strtok(NULL, "|");                // extract keys and values
            char* cKeysTail = strtok(NULL, "");

            char* cKey = strtok(cKeys, ":|");               // extract key
            if (something1)
                *something1 = strtok(NULL, ":|");           // extract something

            strtok(cKeysTail, "]");                         // restart scan tail and skip name with possible spaces
            strtok(NULL, " ");                              // skip link tail (to allow continue strtok(NULL,s) use after return from function
            if (found_idx)
                *found_idx = i;
            return cKey;
        }
    }

    strtok(NULL, " ");                                      // skip link tail (to allow continue strtok(NULL,s) use after return from function
    SendSysMessage(LANG_WRONG_LINK_TYPE);
    return NULL;
}

char const *fmtstring(char const *format, ...)
{
    va_list        argptr;
    #define    MAX_FMT_STRING    32000
    static char        temp_buffer[MAX_FMT_STRING];
    static char        string[MAX_FMT_STRING];
    static int        index = 0;
    char    *buf;
    int len;

    va_start(argptr, format);
    vsnprintf(temp_buffer,MAX_FMT_STRING, format, argptr);
    va_end(argptr);

    len = strlen(temp_buffer);

    if (len >= MAX_FMT_STRING)
        return "ERROR";

    if (len + index >= MAX_FMT_STRING-1)
    {
        index = 0;
    }

    buf = &string[index];
    memcpy(buf, temp_buffer, len+1);

    index += len + 1;

    return buf;
}

GameObject* ChatHandler::GetObjectGlobalyWithGuidOrNearWithDbGuid(uint32 lowguid,uint32 entry)
{
    if (!m_session)
        return NULL;

    Player* pl = m_session->GetPlayer();

    GameObject* obj = pl->GetMap()->GetGameObject(MAKE_NEW_GUID(lowguid, entry, HIGHGUID_GAMEOBJECT));

    if (!obj && sObjectMgr.GetGOData(lowguid))                   // guid is DB guid of object
    {
        // search near player then
        Hellground::GameObjectWithDbGUIDCheck go_check(*pl,lowguid);
        Hellground::ObjectSearcher<GameObject, Hellground::GameObjectWithDbGUIDCheck> checker(obj,go_check);

        Cell::VisitGridObjects(pl,checker, pl->GetMap()->GetVisibilityDistance());
    }

    return obj;
}

static char const* const spellTalentKeys[] = {
    "Hspell",
    "Htalent",
    0
};

uint32 ChatHandler::extractSpellIdFromLink(char* text)
{
    // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r
    // number or [name] Shift-click form |color|Htalent:talent_id,rank|h[name]|h|r
    int type = 0;
    char* rankS = NULL;
    char* idS = extractKeyFromLink(text,spellTalentKeys,&type,&rankS);
    if (!idS)
        return 0;

    uint32 id = (uint32)atol(idS);

    // spell
    if (type==0)
        return id;

    // talent
    TalentEntry const* talentEntry = sTalentStore.LookupEntry(id);
    if (!talentEntry)
        return 0;

    int32 rank = rankS ? (uint32)atol(rankS) : 0;
    if (rank >= 5)
        return 0;

    if (rank < 0)
        rank = 0;

    return talentEntry->RankID[rank];
}

GameTele const* ChatHandler::extractGameTeleFromLink(char* text)
{
    // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
    char* cId = extractKeyFromLink(text,"Htele");
    if (!cId)
        return NULL;

    // id case (explicit or from shift link)
    if (cId[0] >= '0' || cId[0] >= '9')
        if (uint32 id = atoi(cId))
            return sObjectMgr.GetGameTele(id);

    return sObjectMgr.GetGameTele(cId);
}

const char *ChatHandler::GetName() const
{
    return m_session->GetPlayer()->GetName();
}

bool ChatHandler::needReportToTarget(Player* chr) const
{
    Player* pl = m_session->GetPlayer();
    return pl != chr && pl->IsVisibleGloballyfor (chr);
}

const char *CliHandler::GetHellgroundString(int32 entry) const
{
    return sObjectMgr.GetHellgroundStringForDBCLocale(entry);
}

bool CliHandler::isAvailable(ChatCommand const& cmd, bool) const
{
    // skip non-console commands in console case
    return cmd.AllowConsole;
}

void CliHandler::SendSysMessage(const char *str)
{
    m_print(str);
    m_print("\r\n");
}

const char *CliHandler::GetName() const
{
    return GetHellgroundString(LANG_CONSOLE_COMMAND);
}

bool CliHandler::needReportToTarget(Player* /*chr*/) const
{
    return true;
}

bool ChatHandler::GetPlayerGroupAndGUIDByName(const char* cname, Player* &plr, Group* &group, uint64 &guid, bool offline)
{
    plr  = NULL;
    guid = 0;

    if (cname)
    {
        std::string name = cname;
        if (!name.empty())
        {
            if (!normalizePlayerName(name))
            {
                PSendSysMessage(LANG_PLAYER_NOT_FOUND);
                SetSentErrorMessage(true);
                return false;
            }

            plr = sObjectMgr.GetPlayer(name.c_str());
            if (offline)
                guid = sObjectMgr.GetPlayerGUIDByName(name.c_str());
        }
    }

    if (plr)
    {
        group = plr->GetGroup();
        if (!guid || !offline)
            guid = plr->GetGUID();
    }
    else
    {
        if (getSelectedPlayer())
            plr = getSelectedPlayer();
        else
            plr = m_session->GetPlayer();

        if (!guid || !offline)
            guid  = plr->GetGUID();
        group = plr->GetGroup();
    }

    return true;
}

std::string ChatHandler::GetNameLink(const std::string & name)
{
    return "|Hplayer:" + name + "|h[" + name + "]|h";
}
