
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "LFGMgr.h"
#include "Chat.h"
#include "Position.h"
#include "WorldSession.h"
#include "World.h"
#include <ctime>
#include "Log.h"

#define JOIN_QUE              "Join que"
#define LEAVE_QUE             "Leave que"
#define CURRENT_STATUS        "Que status"
#define ACCEPT_QUE            "Yes"
#define DECLINE_QUE           "No"
#define ASK_ME_LATER          "Ask me later"
#define ARE_YOU_SURE          "Yes, I want to leave que"
#define REGRET_DECLINE        "No, I want to enter BG"

enum Options {
    JOIN_QUE_ACTION = 1,
    LEAVE_QUE_ACTION = 2,
    QUE_STATUS_ACTION = 3,
    ACCEPT_QUE_ACTION = 4,
    DECLINE_QUE_ACTION = 5,
    ASK_ME_LATER_ACTION = 6
};

enum Misc {
    MAP_ID = 1,
    //QUE_START_COUNTDOWN = 60,
    MIN_PLAYERS = 10,
    ASK_ME_LATER_DELAY = 30,
    CHECK_DISCONNECT_DELAY = 60,
    SEND_ACCEPT_DELAY = 60,
    WAIT_FOR_BG_START = 180
};

enum Spells {
    BG_SPELL_DESERTER = 26013
};

class ffa_que_npc : public CreatureScript {
public:
    ffa_que_npc() : CreatureScript("custom_script") {}

    struct ffa_que_npcAI : public ScriptedAI {
    public:
        ffa_que_npcAI(Creature* creature) : ScriptedAI(creature) {}

        struct QuedPlayer {
        public:
            QuedPlayer(Player* player) {
                if (player) {
                    this->_player = player;
                }
            }

            Player* GetPlayer() {
                return _player;
            }

            bool HasAcceptSent() {
                return _hasAcceptSent;
            }

            void SetHasAcceptSent(bool hasAcceptSent) {
                _hasAcceptSent = hasAcceptSent;
            }

            uint32 GetOfflineTime() {
                return _offlineTime;
            }

            uint32 GetAskMeLaterTime() {
                return _askMeLatertime;
            }

            uint32 GetSendAcceptAgainTime() {
                return _sendAcceptAgainTime;
            }

            void SetSendAcceptAgain(uint32 sendAcceptAgainTime) {
                _sendAcceptAgainTime = sendAcceptAgainTime;
            }

            void SetOfflineTime(uint32 offlineTime) {
                _offlineTime = offlineTime;
            }

            void SetAskMeLaterTime(uint32 askMeLatertime) {
                _askMeLatertime = askMeLatertime;
            }

            void SetHasPlayerAnswered(bool hasPlayerAnswered) {
                _hasPlayerAnswered = hasPlayerAnswered;
            }

            bool GetHasPlayerAnswered() {
                return _hasPlayerAnswered;
            }

            void SetSendAcceptAgainTime(uint32 sendAcceptAgainTime) {
                _sendAcceptAgainTime = sendAcceptAgainTime;
            }

            void SetHasEnteredBG(bool hasEnteredBG) {
                _hasEnteredBG = hasEnteredBG;
            }

            bool GetHasEnteredBG() {
                return _hasEnteredBG;
            }

        private:
            Player* _player;
            uint32 _offlineTime = 0;
            uint32 _askMeLatertime = 0;
            uint32 _sendAcceptAgainTime = 0;
            bool _hasEnteredBG = false;
            bool _hasAcceptSent = false;
            bool _hasPlayerAnswered = false;
        };

        void UpdateAI(uint32 diff) override {
            for (QuedPlayer* quedPlayer : _quedPlayers) {
                AddPlayerToVector(quedPlayer->GetPlayer());
                CheckDisconnectedPlayers(quedPlayer);
                if (IsQueReady()) {
                    CheckAcceptSent(quedPlayer);
                    CheckAskMeLater(quedPlayer);
                    CheckSendAcceptAgain(quedPlayer);
                }
            }
        }

        bool GossipHello(Player* player) override {
            if (!player) {
                return false;
            }

            me->SetFacingToObject((Unit*)player);

            // Join que gossip
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, JOIN_QUE, GOSSIP_SENDER_MAIN, JOIN_QUE_ACTION);
            SendGossipMenuFor(player, JOIN_QUE_ACTION, me->GetGUID());

            // Leave que gossip
            if (IsPlayerInQue(player)) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, LEAVE_QUE, GOSSIP_SENDER_MAIN, LEAVE_QUE_ACTION);
            }
            SendGossipMenuFor(player, LEAVE_QUE_ACTION, me->GetGUID());

            // Get current status
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, CURRENT_STATUS, GOSSIP_SENDER_MAIN, QUE_STATUS_ACTION);
            SendGossipMenuFor(player, QUE_STATUS_ACTION, me->GetGUID());
            return true;
        }

        bool GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override {
            if (!player) {
                return false;
            }

            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);
            switch (action) {
            case JOIN_QUE_ACTION:
                AddPlayerToQue(player);
                ClearGossipMenuFor(player);
                CloseGossipMenuFor(player);
                break;
            case LEAVE_QUE_ACTION:
                RemovePlayerFromQue(player);
                ClearGossipMenuFor(player);
                CloseGossipMenuFor(player);
                break;
            case QUE_STATUS_ACTION:
                if (AmountOfPlayersInQue() >= MIN_PLAYERS) {
                    ChatHandler(player->GetSession()).SendSysMessage("Battleground has already started.");
                }
                ChatHandler(player->GetSession()).SendSysMessage(("Amount of players in que: " + std::to_string(AmountOfPlayersInQue())).c_str());
                ClearGossipMenuFor(player);
                CloseGossipMenuFor(player);
                break;
            case ACCEPT_QUE_ACTION:
                TeleportPlayerToBG(player);
                GetQuedPlayer(player)->SetHasPlayerAnswered(true);
                break;
            case DECLINE_QUE_ACTION:
                RemovePlayerFromQue(player);
                ClearGossipMenuFor(player);
                CloseGossipMenuFor(player);
                break;
            case ASK_ME_LATER_ACTION:
                GetQuedPlayer(player)->SetAskMeLaterTime(std::time(0));
                ClearGossipMenuFor(player);
                CloseGossipMenuFor(player);
                break;
            }
            return true;
        }

        bool AddPlayerToQue(Player* player) {
            if (!player) {
                return false;
            }

            if (player->HasAura(BG_SPELL_DESERTER)) {
                ChatHandler(player->GetSession()).SendSysMessage("You're deserter and cannot join for que!");
                return false;
            }

            QuedPlayer* quedPlayer = GetQuedPlayer(player);
            if (!(std::find(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer) != _quedPlayers.end())) {
                _quedPlayers.push_back(new QuedPlayer(player));
                ChatHandler(player->GetSession()).SendSysMessage("You have been added to que successfully!");
                return true;
            }
            ChatHandler(player->GetSession()).SendSysMessage("Player already in que!");
            return false;
        }

        bool RemovePlayerFromQue(Player* player) {
            if (!player) {
                return false;
            }

            QuedPlayer* quedPlayer = GetQuedPlayer(player);
            if ((std::find(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer) != _quedPlayers.end())) {
                _quedPlayers.erase(std::remove(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer), _quedPlayers.end());
                ChatHandler(player->GetSession()).SendSysMessage("You have been removed from que successfully!");
                return true;
            }
            ChatHandler(player->GetSession()).SendSysMessage("Player not found in que!");
            return false;
        }

        bool RemovePlayerFromQueOnDisconnect(QuedPlayer* quedPlayer) {
            if (!quedPlayer->GetPlayer()) {
                return false;
            }

            if ((std::find(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer) != _quedPlayers.end())) {
                _quedPlayers.erase(std::remove(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer), _quedPlayers.end());
                return true;
            }
            return false;
        }

        void CheckSendAcceptAgain(QuedPlayer* quedPlayer) {
            if (!quedPlayer->GetPlayer()) {
                return;
            }

            if (!quedPlayer->GetHasPlayerAnswered() && quedPlayer->GetSendAcceptAgainTime() + SEND_ACCEPT_DELAY <= std::time(0)) {
                SendAcceptTo(quedPlayer);
            }
        }

        bool CheckAcceptSent(QuedPlayer* quedPlayer) {
            if (!quedPlayer->GetPlayer()) {
                return false;
            }

            if (!quedPlayer->HasAcceptSent()) {
                SendAcceptTo(quedPlayer);
                return true;
            }
            return false;
        }

        bool SendAcceptTo(QuedPlayer* quedPlayer) {
            if (!quedPlayer->GetPlayer()) {
                return false;
            }

            if (!quedPlayer->HasAcceptSent() || quedPlayer->GetAskMeLaterTime() != 0 || quedPlayer->GetSendAcceptAgainTime() != 0) {
                quedPlayer->SetHasAcceptSent(true);
                Player* player = quedPlayer->GetPlayer();
                ClearGossipMenuFor(player);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, ACCEPT_QUE, GOSSIP_SENDER_MAIN, ACCEPT_QUE_ACTION);
                SendGossipMenuFor(player, ACCEPT_QUE_ACTION, me->GetGUID());

                AddGossipItemFor(player, GOSSIP_ICON_CHAT, DECLINE_QUE, GOSSIP_SENDER_MAIN, DECLINE_QUE_ACTION);
                SendGossipMenuFor(player, DECLINE_QUE_ACTION, me->GetGUID());

                AddGossipItemFor(player, GOSSIP_ICON_CHAT, ASK_ME_LATER, GOSSIP_SENDER_MAIN, ASK_ME_LATER_ACTION);
                SendGossipMenuFor(player, ASK_ME_LATER_ACTION, me->GetGUID());
                quedPlayer->SetHasAcceptSent(true);
                quedPlayer->SetSendAcceptAgainTime(std::time(0));
                return true;
            }
            return false;
        }

        void PlayerDeclineQue(Player* player) {
            if (!player) {
                return;
            }

            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, ARE_YOU_SURE, GOSSIP_SENDER_MAIN, DECLINE_QUE_ACTION);
            SendGossipMenuFor(player, DECLINE_QUE_ACTION, me->GetGUID());

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, REGRET_DECLINE, GOSSIP_SENDER_MAIN, ACCEPT_QUE_ACTION);
            SendGossipMenuFor(player, ACCEPT_QUE_ACTION, me->GetGUID());
        }

        bool IsPlayerInQue(Player* player) {
            if (!player) {
                return false;
            }

            QuedPlayer* quedPlayer = GetQuedPlayer(player);
            return std::find(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer) != _quedPlayers.end();
        }

        QuedPlayer* GetQuedPlayer(Player* player) {
            for (QuedPlayer* quedPlayer : _quedPlayers) {
                if (quedPlayer->GetPlayer()->GetName() == player->GetName()) {
                    return quedPlayer;
                }
            }
            return nullptr;
        }

        bool TeleportPlayerToBG(Player* player) {
            if (!player) {
                return false;
            }

            QuedPlayer* quedPlayer = GetQuedPlayer(player);
            quedPlayer->SetHasEnteredBG(true);
            quedPlayer->SetHasAcceptSent(std::time(0));
            player->SaveRecallPosition();
            player->TeleportTo(MAP_ID, _teleportPosition.GetPositionX(), _teleportPosition.GetPositionY(), _teleportPosition.GetPositionZ(), _teleportPosition.GetOrientation());
            return true;
        }

        void CheckDisconnectedPlayers(QuedPlayer* quedPlayer) {
            if (!quedPlayer->GetPlayer()) {
                return;
            }

            if (!quedPlayer->GetPlayer()->IsInWorld() && quedPlayer->GetOfflineTime() == 0) {
                quedPlayer->SetOfflineTime(std::time(0));
            }
            if (quedPlayer->GetOfflineTime() > 0) {
                if (quedPlayer->GetOfflineTime() + CHECK_DISCONNECT_DELAY <= std::time(0)) {
                    quedPlayer->SetOfflineTime(std::time(0));
                    RemovePlayerFromQueOnDisconnect(quedPlayer);
                }
            }
        }

        void CheckAskMeLater(QuedPlayer* quedPlayer) {
            if (!quedPlayer->GetPlayer()) {
                return;
            }

            if (quedPlayer->GetAskMeLaterTime() != 0) {
                if (quedPlayer->GetAskMeLaterTime() + ASK_ME_LATER_DELAY <= std::time(0)) {
                    SendAcceptTo(quedPlayer);
                    quedPlayer->SetAskMeLaterTime(0);
                }
            }
        }

        uint32 AmountOfPlayersInQue() {
            return _quedPlayers.size();
        }

        bool IsQueReady() {
            if (AmountOfPlayersInQue() >= 2) {
                return true;
            }
            return false;
        }

        void Say(std::string str) {
            me->Say(str, LANG_UNIVERSAL);
        }

    private:
        std::vector<QuedPlayer*> _quedPlayers;
        float const _positionX = 4564.0f;
        float const _positionY = -3098.0f;
        float const _positionZ = 995.0f;
        float const _orientation = 0;
        Position const _teleportPosition = { _positionX, _positionY, _positionZ, _orientation };
    };

    CreatureAI* GetAI(Creature* creature) const {
        return new ffa_que_npcAI(creature);
    }

};

bool AddPlayerToVector(Player* player) {
    if (!(std::find(playersInQue.begin(), playersInQue.end(), player) != playersInQue.end())) {
        playersInQue.push_back(player);
        return true;
    }
    return false;
}

std::vector<Player*> playersInQue;

void AddSC_custom_script() {
    new ffa_que_npc();
}
