/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "GossipDef.h"
#include "SocialMgr.h"

// Helper for getting guild object of session's player.
// If guild does not exist, sends error (if necessary).
inline Guild* _GetPlayerGuild(WorldSession* session, bool sendError = false)
{
    if (uint32 guildId = session->GetPlayer()->GetGuildId())    // If guild id = 0, player is not in guild
        if (Guild* guild = sGuildMgr->GetGuildById(guildId))   // Find guild by id
            return guild;
    if (sendError)
        Guild::SendCommandResult(session, GUILD_CREATE_S, ERR_GUILD_PLAYER_NOT_IN_GUILD);
    return NULL;
}

void WorldSession::HandleGuildQueryOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_QUERY");

    ObjectGuid guildGuid;
    ObjectGuid playerGuid;

	playerGuid[7] = recvPacket.ReadBit();
	playerGuid[3] = recvPacket.ReadBit();
	playerGuid[4] = recvPacket.ReadBit();
	guildGuid[3] = recvPacket.ReadBit();
	guildGuid[4] = recvPacket.ReadBit();
	playerGuid[2] = recvPacket.ReadBit();
	playerGuid[6] = recvPacket.ReadBit();
	guildGuid[2] = recvPacket.ReadBit();
	guildGuid[5] = recvPacket.ReadBit();
	playerGuid[1] = recvPacket.ReadBit();
	playerGuid[5] = recvPacket.ReadBit();
	guildGuid[7] = recvPacket.ReadBit();
	playerGuid[0] = recvPacket.ReadBit();
	guildGuid[1] = recvPacket.ReadBit();
	guildGuid[6] = recvPacket.ReadBit();
	guildGuid[0] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(playerGuid[7]);
	recvPacket.ReadByteSeq(guildGuid[2]);
	recvPacket.ReadByteSeq(guildGuid[4]);
	recvPacket.ReadByteSeq(guildGuid[7]);
	recvPacket.ReadByteSeq(playerGuid[6]);
	recvPacket.ReadByteSeq(playerGuid[0]);
	recvPacket.ReadByteSeq(guildGuid[6]);
	recvPacket.ReadByteSeq(guildGuid[0]);
	recvPacket.ReadByteSeq(guildGuid[3]);
	recvPacket.ReadByteSeq(playerGuid[2]);
	recvPacket.ReadByteSeq(guildGuid[5]);
	recvPacket.ReadByteSeq(playerGuid[3]);
	recvPacket.ReadByteSeq(guildGuid[1]);
	recvPacket.ReadByteSeq(playerGuid[4]);
	recvPacket.ReadByteSeq(playerGuid[1]);
	recvPacket.ReadByteSeq(playerGuid[5]);

    // If guild doesn't exist or player is not part of the guild send error
    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(playerGuid))
        {
            guild->HandleQuery(this);
            return;
        }

    Guild::SendCommandResult(this, GUILD_CREATE_S, ERR_GUILD_PLAYER_NOT_IN_GUILD);
}

void WorldSession::HandleGuildInviteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_INVITE");

    uint32 nameLength = recvPacket.ReadBits(9);
    std::string invitedName = recvPacket.ReadString(nameLength);

    if (normalizePlayerName(invitedName))
        if (Guild* guild = _GetPlayerGuild(this, true))
            guild->HandleInviteMember(this, invitedName);
}

void WorldSession::HandleGuildRemoveOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_REMOVE");

    ObjectGuid playerGuid;

	playerGuid[7] = recvPacket.ReadBit();
	playerGuid[3] = recvPacket.ReadBit();
	playerGuid[4] = recvPacket.ReadBit();
	playerGuid[2] = recvPacket.ReadBit();
	playerGuid[5] = recvPacket.ReadBit();
	playerGuid[6] = recvPacket.ReadBit();
	playerGuid[1] = recvPacket.ReadBit();
	playerGuid[0] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(playerGuid[0]);
	recvPacket.ReadByteSeq(playerGuid[2]);
	recvPacket.ReadByteSeq(playerGuid[5]);
	recvPacket.ReadByteSeq(playerGuid[6]);
	recvPacket.ReadByteSeq(playerGuid[7]);
	recvPacket.ReadByteSeq(playerGuid[1]);
	recvPacket.ReadByteSeq(playerGuid[4]);
	recvPacket.ReadByteSeq(playerGuid[3]);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleRemoveMember(this, playerGuid);
}

void WorldSession::HandleGuildMasterReplaceOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_REPLACE_GUILD_MASTER");

    Guild* guild = _GetPlayerGuild(this, true);

    if (!guild)
        return; // Cheat

    uint32 logoutTime = guild->GetMemberLogoutTime(guild->GetLeaderGUID());

    if (!logoutTime)
        return;

    time_t now = time(NULL);

    if (time_t(logoutTime + 3 * MONTH) > now)
        return; // Cheat

    guild->SwitchGuildLeader(GetPlayer()->GetGUID());
}

void WorldSession::HandleGuildAcceptOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ACCEPT");
    // Player cannot be in guild
    if (!GetPlayer()->GetGuildId())
        // Guild where player was invited must exist
        if (Guild* guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildIdInvited()))
            guild->HandleAcceptMember(this);
}

void WorldSession::HandleGuildDeclineOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DECLINE");

    GetPlayer()->SetGuildIdInvited(0);
    GetPlayer()->SetInGuild(0);
}

void WorldSession::HandleGuildRosterOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ROSTER");

    recvPacket.rfinish();

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleRoster(this);
}

// Deprecated Handler
void WorldSession::HandleGuildPromoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_PROMOTE");

	ObjectGuid targetGuid;
	targetGuid[6] = recvPacket.ReadBit();
	targetGuid[0] = recvPacket.ReadBit();
	targetGuid[4] = recvPacket.ReadBit();
	targetGuid[3] = recvPacket.ReadBit();
	targetGuid[1] = recvPacket.ReadBit();
	targetGuid[7] = recvPacket.ReadBit();
	targetGuid[2] = recvPacket.ReadBit();
	targetGuid[5] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(targetGuid[1]);
	recvPacket.ReadByteSeq(targetGuid[7]);
	recvPacket.ReadByteSeq(targetGuid[2]);
	recvPacket.ReadByteSeq(targetGuid[5]);
	recvPacket.ReadByteSeq(targetGuid[3]);
	recvPacket.ReadByteSeq(targetGuid[4]);
	recvPacket.ReadByteSeq(targetGuid[0]);
	recvPacket.ReadByteSeq(targetGuid[6]);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleUpdateMemberRank(this, targetGuid, false);
}

void WorldSession::HandleGuildDemoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DEMOTE");

	ObjectGuid targetGuid;

	targetGuid[3] = recvPacket.ReadBit();
	targetGuid[6] = recvPacket.ReadBit();
	targetGuid[0] = recvPacket.ReadBit();
	targetGuid[2] = recvPacket.ReadBit();
	targetGuid[7] = recvPacket.ReadBit();
	targetGuid[5] = recvPacket.ReadBit();
	targetGuid[4] = recvPacket.ReadBit();
	targetGuid[1] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(targetGuid[7]);
	recvPacket.ReadByteSeq(targetGuid[4]);
	recvPacket.ReadByteSeq(targetGuid[2]);
	recvPacket.ReadByteSeq(targetGuid[5]);
	recvPacket.ReadByteSeq(targetGuid[1]);
	recvPacket.ReadByteSeq(targetGuid[3]);
	recvPacket.ReadByteSeq(targetGuid[0]);
	recvPacket.ReadByteSeq(targetGuid[6]);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleUpdateMemberRank(this, targetGuid, 0);
}

void WorldSession::HandleGuildAssignRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ASSIGN_MEMBER_RANK");

	ObjectGuid targetGuid;
	uint32 rankId;

	recvPacket >> rankId;

	targetGuid[2] = recvPacket.ReadBit();
	targetGuid[3] = recvPacket.ReadBit();
	targetGuid[1] = recvPacket.ReadBit();
	targetGuid[6] = recvPacket.ReadBit();
	targetGuid[0] = recvPacket.ReadBit();
	targetGuid[4] = recvPacket.ReadBit();
	targetGuid[7] = recvPacket.ReadBit();
	targetGuid[5] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(targetGuid[7]);
	recvPacket.ReadByteSeq(targetGuid[3]);
	recvPacket.ReadByteSeq(targetGuid[2]);
	recvPacket.ReadByteSeq(targetGuid[5]);
	recvPacket.ReadByteSeq(targetGuid[6]);
	recvPacket.ReadByteSeq(targetGuid[0]);
	recvPacket.ReadByteSeq(targetGuid[4]);
	recvPacket.ReadByteSeq(targetGuid[1]);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleSetMemberRank(this, targetGuid, GetPlayer()->GetGUID(), rankId);
}

void WorldSession::HandleGuildLeaveOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_LEAVE");

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleLeaveMember(this);
}

void WorldSession::HandleGuildDisbandOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DISBAND");

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleDisband(this);
}

void WorldSession::HandleGuildLeaderOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_LEADER");

    std::string name;
    uint32 len = recvPacket.ReadBits(9);
    name = recvPacket.ReadString(len);

    if (normalizePlayerName(name))
        if (Guild* guild = _GetPlayerGuild(this, true))
            guild->HandleSetLeader(this, name);
}

void WorldSession::HandleGuildMOTDOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_MOTD");

    uint32 motdLength = recvPacket.ReadBits(10);
    std::string motd = recvPacket.ReadString(motdLength);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleSetMOTD(this, motd);
}

void WorldSession::HandleSwapRanks(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_SWITCH_RANK");

    uint32 id = 0;
    bool up = false;

    recvPacket >> id;
    up = recvPacket.ReadBit();
    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleSwapRanks(this, id, up);
}

void WorldSession::HandleGuildSetNoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_SET_NOTE");

    ObjectGuid playerGuid;

    playerGuid[1] = recvPacket.ReadBit();
	uint32 noteLength = recvPacket.ReadBits(8);
    playerGuid[4] = recvPacket.ReadBit();
    playerGuid[2] = recvPacket.ReadBit();
	bool type = recvPacket.ReadBit();     // 0 == Officer, 1 == Public
    playerGuid[3] = recvPacket.ReadBit();
    playerGuid[5] = recvPacket.ReadBit();
    playerGuid[0] = recvPacket.ReadBit();
    playerGuid[6] = recvPacket.ReadBit();
    playerGuid[7] = recvPacket.ReadBit();

    recvPacket.FlushBits();
    /*
    recvPacket.ReadBit(); //noteLength & 0x1F
    */
	recvPacket.ReadByteSeq(playerGuid[5]);
	recvPacket.ReadByteSeq(playerGuid[1]);
	recvPacket.ReadByteSeq(playerGuid[6]);
	std::string note = recvPacket.ReadString(noteLength);
	recvPacket.ReadByteSeq(playerGuid[0]);
	recvPacket.ReadByteSeq(playerGuid[7]);
	recvPacket.ReadByteSeq(playerGuid[4]);
	recvPacket.ReadByteSeq(playerGuid[3]);
	recvPacket.ReadByteSeq(playerGuid[2]);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleSetMemberNote(this, note, playerGuid, type);
}

void WorldSession::HandleGuildQueryRanksOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_QUERY_RANKS");

    ObjectGuid guildGuid;

	guildGuid[0] = recvData.ReadBit();
	guildGuid[2] = recvData.ReadBit();
	guildGuid[5] = recvData.ReadBit();
	guildGuid[4] = recvData.ReadBit();
	guildGuid[3] = recvData.ReadBit();
	guildGuid[7] = recvData.ReadBit();
	guildGuid[6] = recvData.ReadBit();
	guildGuid[1] = recvData.ReadBit();

	recvData.ReadByteSeq(guildGuid[6]);
	recvData.ReadByteSeq(guildGuid[0]);
	recvData.ReadByteSeq(guildGuid[1]);
	recvData.ReadByteSeq(guildGuid[7]);
	recvData.ReadByteSeq(guildGuid[3]);
	recvData.ReadByteSeq(guildGuid[2]);
	recvData.ReadByteSeq(guildGuid[5]);
	recvData.ReadByteSeq(guildGuid[4]);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(_player->GetGUID()))
            guild->HandleGuildRanks(this);
}

void WorldSession::HandleGuildAddRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ADD_RANK");

    uint32 rankId;
    recvPacket >> rankId;

    uint32 length = recvPacket.ReadBits(7);
    std::string rankName = recvPacket.ReadString(length);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleAddNewRank(this, rankName); //, rankId);
}

void WorldSession::HandleGuildDelRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DEL_RANK");

    uint32 rankId;
    recvPacket >> rankId;

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleRemoveRank(this, rankId);
}

void WorldSession::HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_INFO_TEXT");

    uint32 length = recvPacket.ReadBits(11);
    std::string info = recvPacket.ReadString(length);

    if (Guild* guild = _GetPlayerGuild(this, true))
        guild->HandleSetInfo(this, info);
}

void WorldSession::HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received MSG_SAVE_GUILD_EMBLEM");

    uint64 vendorGuid;
    recvPacket >> vendorGuid;

    EmblemInfo emblemInfo;
    emblemInfo.ReadPacket(recvPacket);

    if (GetPlayer()->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_TABARDDESIGNER))
    {
        // Remove fake death
        if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
            GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

        if (Guild* guild = _GetPlayerGuild(this))
            guild->HandleSetEmblem(this, emblemInfo);
        else
            // "You are not part of a guild!";
            Guild::SendSaveEmblemResult(this, ERR_GUILDEMBLEM_NOGUILD);
    }
    else
    {
        // "That's not an emblem vendor!"
        Guild::SendSaveEmblemResult(this, ERR_GUILDEMBLEM_INVALIDVENDOR);
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleSaveGuildEmblemOpcode - Unit (GUID: %u) not found or you can't interact with him.", GUID_LOPART(vendorGuid));
    }
}

void WorldSession::HandleGuildEventLogQueryOpcode(WorldPacket& /* recvPacket */)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_EVENT_LOG_QUERY)");

    if (Guild* guild = _GetPlayerGuild(this))
        guild->SendEventLog(this);
}

void WorldSession::HandleGuildBankMoneyWithdrawn(WorldPacket& /* recvData */)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_MONEY_WITHDRAWN_QUERY)");

    if (Guild* guild = _GetPlayerGuild(this))
        guild->SendMoneyInfo(this);
}

void WorldSession::HandleGuildPermissions(WorldPacket& /* recvData */)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_PERMISSIONS)");

    if (Guild* guild = _GetPlayerGuild(this))
        guild->SendPermissions(this);
}

// Called when clicking on Guild bank gameobject
void WorldSession::HandleGuildBankerActivate(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANKER_ACTIVATE)");

	ObjectGuid GoGuid;
	bool sendAllSlots;

	GoGuid[3] = recvData.ReadBit();
	sendAllSlots = recvData.ReadBit();
	GoGuid[0] = recvData.ReadBit();
	GoGuid[7] = recvData.ReadBit();
	GoGuid[1] = recvData.ReadBit();
	GoGuid[5] = recvData.ReadBit();
	GoGuid[2] = recvData.ReadBit();
	GoGuid[6] = recvData.ReadBit();
	GoGuid[4] = recvData.ReadBit();

	recvData.ReadByteSeq(GoGuid[7]);
	recvData.ReadByteSeq(GoGuid[1]);
	recvData.ReadByteSeq(GoGuid[0]);
	recvData.ReadByteSeq(GoGuid[6]);
	recvData.ReadByteSeq(GoGuid[4]);
	recvData.ReadByteSeq(GoGuid[2]);
	recvData.ReadByteSeq(GoGuid[5]);
	recvData.ReadByteSeq(GoGuid[3]);

    if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
    {
        if (Guild* guild = _GetPlayerGuild(this))
            guild->SendBankList(this, 0, true, true);
        else
            Guild::SendCommandResult(this, GUILD_UNK1, ERR_GUILD_PLAYER_NOT_IN_GUILD);
    }
}

// Called when opening guild bank tab only (first one)
void WorldSession::HandleGuildBankQueryTab(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_QUERY_TAB)");

    uint64 GoGuid;
    recvData >> GoGuid;

    uint8 tabId;
    recvData >> tabId;

    uint8 fullSlotList;
    recvData >> fullSlotList; // 0 = only slots updated in last operation are shown. 1 = all slots updated

    if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* guild = _GetPlayerGuild(this))
            guild->SendBankList(this, tabId, true, false);
}

void WorldSession::HandleGuildBankDepositMoney(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_DEPOSIT_MONEY)");

	ObjectGuid goGuid;
	uint64 money;

	recvData >> money;

	goGuid[2] = recvData.ReadBit();
	goGuid[7] = recvData.ReadBit();
	goGuid[6] = recvData.ReadBit();
	goGuid[4] = recvData.ReadBit();
	goGuid[0] = recvData.ReadBit();
	goGuid[1] = recvData.ReadBit();
	goGuid[5] = recvData.ReadBit();
	goGuid[3] = recvData.ReadBit();

	recvData.ReadByteSeq(goGuid[1]);
	recvData.ReadByteSeq(goGuid[4]);
	recvData.ReadByteSeq(goGuid[5]);
	recvData.ReadByteSeq(goGuid[0]);
	recvData.ReadByteSeq(goGuid[2]);
	recvData.ReadByteSeq(goGuid[7]);
	recvData.ReadByteSeq(goGuid[6]);
	recvData.ReadByteSeq(goGuid[3]);

    if (GetPlayer()->GetGameObjectIfCanInteractWith(goGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (money && GetPlayer()->HasEnoughMoney(money))
            if (Guild* guild = _GetPlayerGuild(this))
                guild->HandleMemberDepositMoney(this, money);
}

void WorldSession::HandleGuildBankWithdrawMoney(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_WITHDRAW_MONEY)");

    uint64 GoGuid;
    recvData >> GoGuid;

    uint32 money;
    recvData >> money;

    if (money)
        if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
            if (Guild* guild = _GetPlayerGuild(this))
                guild->HandleMemberWithdrawMoney(this, money);
}

void WorldSession::HandleGuildBankSwapItems(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_SWAP_ITEMS)");

    uint64 GoGuid;
    recvData >> GoGuid;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
    {
        recvData.rfinish();                   // Prevent additional spam at rejected packet
        return;
    }

    Guild* guild = _GetPlayerGuild(this);
    if (!guild)
    {
        recvData.rfinish();                   // Prevent additional spam at rejected packet
        return;
    }

    uint8 bankToBank;
    recvData >> bankToBank;

    uint8 tabId;
    uint8 slotId;
    uint32 itemEntry;
    uint32 splitedAmount = 0;

    if (bankToBank)
    {
        uint8 destTabId;
        recvData >> destTabId;

        uint8 destSlotId;
        recvData >> destSlotId;

        uint32 destItemEntry;
        recvData >> destItemEntry;

        recvData >> tabId;
        recvData >> slotId;
        recvData >> itemEntry;
        recvData.read_skip<uint8>();                       // Always 0
        recvData >> splitedAmount;

        guild->SwapItems(GetPlayer(), tabId, slotId, destTabId, destSlotId, splitedAmount);
    }
    else
    {
        uint8 playerBag = NULL_BAG;
        uint8 playerSlotId = NULL_SLOT;
        uint8 toChar = 1;

        recvData >> tabId;
        recvData >> slotId;
        recvData >> itemEntry;

        uint8 autoStore;
        recvData >> autoStore;
        if (autoStore)
        {
            recvData.read_skip<uint32>();                  // autoStoreCount
            recvData.read_skip<uint8>();                   // ToChar (?), always and expected to be 1 (autostore only triggered in Bank -> Char)
            recvData.read_skip<uint32>();                  // Always 0
        }
        else
        {
            recvData >> playerBag;
            recvData >> playerSlotId;
            recvData >> toChar;
            recvData >> splitedAmount;
        }

        // Player <-> Bank
        // Allow to work with inventory only
        if (!Player::IsInventoryPos(playerBag, playerSlotId) && !(playerBag == NULL_BAG && playerSlotId == NULL_SLOT))
            GetPlayer()->SendEquipError(EQUIP_ERR_INTERNAL_BAG_ERROR, NULL);
        else
            guild->SwapItemsWithInventory(GetPlayer(), toChar, tabId, slotId, playerBag, playerSlotId, splitedAmount);
    }
}

void WorldSession::HandleGuildBankBuyTab(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_BUY_TAB)");

	ObjectGuid GoGuid;
	uint8 tabId;

	recvData >> tabId;

	GoGuid[0] = recvData.ReadBit();
	GoGuid[1] = recvData.ReadBit();
	GoGuid[3] = recvData.ReadBit();
	GoGuid[7] = recvData.ReadBit();
	GoGuid[2] = recvData.ReadBit();
	GoGuid[6] = recvData.ReadBit();
	GoGuid[5] = recvData.ReadBit();
	GoGuid[4] = recvData.ReadBit();

	recvData.ReadByteSeq(GoGuid[1]);
	recvData.ReadByteSeq(GoGuid[4]);
	recvData.ReadByteSeq(GoGuid[6]);
	recvData.ReadByteSeq(GoGuid[7]);
	recvData.ReadByteSeq(GoGuid[3]);
	recvData.ReadByteSeq(GoGuid[5]);
	recvData.ReadByteSeq(GoGuid[2]);
	recvData.ReadByteSeq(GoGuid[0]);

    if (!GoGuid || GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* guild = _GetPlayerGuild(this))
            guild->HandleBuyBankTab(this, tabId);
}

void WorldSession::HandleGuildBankUpdateTab(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_UPDATE_TAB)");

	ObjectGuid GoGuid;
	uint8 tabId;
	uint32 iconLen, nameLen;
    std::string name, icon;

	recvData >> tabId;

	GoGuid[5] = recvData.ReadBit();
	iconLen = recvData.ReadBits(9);
	GoGuid[1] = recvData.ReadBit();
	GoGuid[4] = recvData.ReadBit();
	GoGuid[2] = recvData.ReadBit();
	GoGuid[7] = recvData.ReadBit();
	GoGuid[0] = recvData.ReadBit();
	GoGuid[6] = recvData.ReadBit();
	GoGuid[3] = recvData.ReadBit();
	nameLen = recvData.ReadBits(7);

	recvData.ReadByteSeq(GoGuid[7]);
	recvData.ReadByteSeq(GoGuid[4]);
	icon = recvData.ReadString(iconLen);
	recvData.ReadByteSeq(GoGuid[5]);
	recvData.ReadByteSeq(GoGuid[1]);
	recvData.ReadByteSeq(GoGuid[0]);
	name = recvData.ReadString(nameLen);
	recvData.ReadByteSeq(GoGuid[2]);
	recvData.ReadByteSeq(GoGuid[3]);
	recvData.ReadByteSeq(GoGuid[6]);

    if (!name.empty() && !icon.empty())
        if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
            if (Guild* guild = _GetPlayerGuild(this))
                guild->HandleSetBankTabInfo(this, tabId, name, icon);
}

void WorldSession::HandleGuildBankLogQuery(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (MSG_GUILD_BANK_LOG_QUERY)");

    uint32 tabId;
    recvData >> tabId;

    if (Guild* guild = _GetPlayerGuild(this))
        guild->SendBankLog(this, tabId);
}

void WorldSession::HandleQueryGuildBankTabText(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_BANK_QUERY_TEXT");

    uint8 tabId;
    recvData >> tabId;

    if (Guild* guild = _GetPlayerGuild(this))
        guild->SendBankTabText(this, tabId);
}

void WorldSession::HandleSetGuildBankTabText(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SET_GUILD_BANK_TEXT");

    uint32 tabId;
    recvData >> tabId;

    uint32 textLen = recvData.ReadBits(14);
    std::string text = recvData.ReadString(textLen);

    if (Guild* guild = _GetPlayerGuild(this))
        guild->SetBankTabText(tabId, text);
}

void WorldSession::HandleGuildQueryXPOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUERY_GUILD_XP");

    ObjectGuid guildGuid;

	guildGuid[5] = recvPacket.ReadBit();
	guildGuid[6] = recvPacket.ReadBit();
	guildGuid[0] = recvPacket.ReadBit();
	guildGuid[1] = recvPacket.ReadBit();
	guildGuid[3] = recvPacket.ReadBit();
	guildGuid[7] = recvPacket.ReadBit();
	guildGuid[4] = recvPacket.ReadBit();
	guildGuid[2] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(guildGuid[4]);
	recvPacket.ReadByteSeq(guildGuid[6]);
	recvPacket.ReadByteSeq(guildGuid[3]);
	recvPacket.ReadByteSeq(guildGuid[0]);
	recvPacket.ReadByteSeq(guildGuid[7]);
	recvPacket.ReadByteSeq(guildGuid[5]);
	recvPacket.ReadByteSeq(guildGuid[2]);
	recvPacket.ReadByteSeq(guildGuid[1]);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(_player->GetGUID()))
            guild->SendGuildXP(this);
}

void WorldSession::HandleGuildSetRankPermissionsOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_SET_RANK_PERMISSIONS");

    Guild* guild = _GetPlayerGuild(this, true);
    if (!guild)
    {
        recvPacket.rfinish();
        return;
    }

	uint32 oldRankId;
	uint32 newRankId;
	uint32 oldRights;
	uint32 newRights;
	uint32 moneyPerDay;

	recvPacket >> oldRankId;

    GuildBankRightsAndSlotsVec rightsAndSlots(GUILD_BANK_MAX_TABS);
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        uint32 bankRights;
        uint32 slots;

        recvPacket >> bankRights;
        recvPacket >> slots;

        rightsAndSlots[tabId] = GuildBankRightsAndSlots(uint8(bankRights), slots);
    }

	recvPacket >> moneyPerDay;
	recvPacket >> oldRights;
	recvPacket >> newRights;
	recvPacket >> newRankId;

    uint32 nameLength = recvPacket.ReadBits(7);
    std::string rankName = recvPacket.ReadString(nameLength);

	guild->HandleSetRankInfo(this, newRankId, rankName, newRights, moneyPerDay, rightsAndSlots);
}

void WorldSession::HandleGuildRequestPartyState(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_REQUEST_PARTY_STATE");

    ObjectGuid guildGuid;

    uint8 bitOrder[8] = {4, 3, 6, 7, 2, 5, 0, 1};
    recvData.ReadBitInOrder(guildGuid, bitOrder);

    uint8 byteOrder[8] = {5, 0, 2, 6, 3, 1, 4, 7};
    recvData.ReadBytesSeq(guildGuid, byteOrder);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        guild->HandleGuildPartyRequest(this);
}

void WorldSession::HandleGuildRequestMaxDailyXP(WorldPacket& recvPacket)
{
    ObjectGuid guildGuid;

    uint8 bitOrder[8] = {2, 5, 3, 7, 4, 1, 0, 6};
    recvPacket.ReadBitInOrder(guildGuid, bitOrder);

    uint8 byteOrder[8] = {7, 3, 2, 1, 0, 5, 6, 4};
    recvPacket.ReadBytesSeq(guildGuid, byteOrder);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
    {
        if (guild->IsMember(_player->GetGUID()))
        {
            WorldPacket data(SMSG_GUILD_MAX_DAILY_XP, 8);
            data << uint64(sWorld->getIntConfig(CONFIG_GUILD_DAILY_XP_CAP));
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleAutoDeclineGuildInvites(WorldPacket& recvPacket)
{
    uint8 enable;
    recvPacket >> enable;

    GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_AUTO_DECLINE_GUILD, enable);
}

void WorldSession::HandleGuildRewardsQueryOpcode(WorldPacket& recvPacket)
{
    recvPacket.read_skip<uint32>(); // Unk

    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
    {
        std::vector<GuildReward> const& rewards = sGuildMgr->GetGuildRewards();

		ByteBuffer dataBuffer;
        WorldPacket data(SMSG_GUILD_REWARDS_LIST, (3 + rewards.size() * (4 + 4 + 4 + 8 + 4 + 4)));
        data.WriteBits(rewards.size(), 19);

        data.FlushBits();

        for (uint32 i = 0; i < rewards.size(); i++)
		{
			dataBuffer << int32(rewards[i].Racemask);
			dataBuffer << uint32(rewards[i].Entry);
			dataBuffer << uint32(rewards[i].AchievementId);
			dataBuffer << uint32(rewards[i].Standing);
			dataBuffer << uint64(rewards[i].Price);
        }

        data << uint32(time(NULL));
        SendPacket(&data);
    }
}

void WorldSession::HandleGuildQueryNewsOpcode(WorldPacket& recvPacket)
{
	recvPacket.rfinish();

    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
    {
        WorldPacket data;
        guild->GetNewsLog().BuildNewsData(data);
        SendPacket(&data);
    }
}

void WorldSession::HandleGuildNewsUpdateStickyOpcode(WorldPacket& recvPacket)
{
	uint32 newsId;
	bool sticky;
	ObjectGuid guid;

	recvPacket >> newsId;

	guid[6] = recvPacket.ReadBit();
	guid[0] = recvPacket.ReadBit();
	sticky = recvPacket.ReadBit();
	guid[2] = recvPacket.ReadBit();
	guid[7] = recvPacket.ReadBit();
	guid[5] = recvPacket.ReadBit();
	guid[4] = recvPacket.ReadBit();
	guid[3] = recvPacket.ReadBit();
	guid[1] = recvPacket.ReadBit();

	recvPacket.ReadByteSeq(guid[5]);
	recvPacket.ReadByteSeq(guid[4]);
	recvPacket.ReadByteSeq(guid[0]);
	recvPacket.ReadByteSeq(guid[1]);
	recvPacket.ReadByteSeq(guid[6]);
	recvPacket.ReadByteSeq(guid[2]);
	recvPacket.ReadByteSeq(guid[3]);
	recvPacket.ReadByteSeq(guid[7]);

    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
    {
        if (GuildNewsEntry* newsEntry = guild->GetNewsLog().GetNewsById(newsId))
        {
            if (sticky)
                newsEntry->Flags |= 1;
            else
                newsEntry->Flags &= ~1;
            WorldPacket data;
            guild->GetNewsLog().BuildNewsData(newsId, *newsEntry, data);
            SendPacket(&data);
        }
    }
}