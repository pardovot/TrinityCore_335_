
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
    QUE_START_COUNTDOWN = 60,
    MIN_PLAYERS = 10,
    ASK_ME_LATER_DELAY = 30,
    CHECK_DISCONNECT_DELAY = 60,
    SEND_ACCEPT_DELAY = 60,
    WAIT_FOR_BG_START = 180
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
                this->_player = player;
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

            void SetOfflineTime(uint32 offlineTime) {
                _offlineTime = offlineTime;
            }

            void SetAskMeLaterTime(uint32 askMeLatertime) {
                _askMeLatertime = askMeLatertime;
            }

        private:
            Player* _player;
            uint32 _offlineTime = 0;
            uint32 _askMeLatertime = 0;
            bool _hasAcceptSent = false;
        };

        void UpdateAI(uint32 diff) override {
            for (QuedPlayer* quedPlayer : _quedPlayers) {
                CheckDisconnectedPlayers(quedPlayer);
                if (IsQueReady()) {
                    CheckAskMeLater(quedPlayer);
                    if (!quedPlayer->HasAcceptSent()) {
                    SendAcceptTo(quedPlayer);
                    }
                }
            }
        }

        bool GossipHello(Player* player) override {
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
                char const* test;
                ChatHandler(player->GetSession()).SendSysMessage(("Amount of players in que: " + std::to_string(AmountOfPlayersInQue())).c_str());
                ClearGossipMenuFor(player);
                CloseGossipMenuFor(player);
                break;
            case ACCEPT_QUE_ACTION:
                TeleportPlayerToBG(player);
                break;
            case DECLINE_QUE_ACTION:
                RemovePlayerFromQue(player);
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
            if ((std::find(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer) != _quedPlayers.end())) {
                _quedPlayers.erase(std::remove(_quedPlayers.begin(), _quedPlayers.end(), quedPlayer), _quedPlayers.end());
                return true;
            }
            return false;
        }

        //void SendAcceptToAll() {
        //    for (QuedPlayer* quedPlayer : _quedPlayers) {
        //        if (!quedPlayer->HasAcceptSent()) {
        //            SendAcceptTo(quedPlayer);
        //        }
        //    }
        //}

        bool SendAcceptTo(QuedPlayer* quedPlayer) {
            if (!quedPlayer->HasAcceptSent()) {
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
                return true;
            }
            return false;
        }

        void PlayerDeclineQue(Player* player) {
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, ARE_YOU_SURE, GOSSIP_SENDER_MAIN, DECLINE_QUE_ACTION);
            SendGossipMenuFor(player, DECLINE_QUE_ACTION, me->GetGUID());

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, REGRET_DECLINE, GOSSIP_SENDER_MAIN, ACCEPT_QUE_ACTION);
            SendGossipMenuFor(player, ACCEPT_QUE_ACTION, me->GetGUID());
        }

        bool IsPlayerInQue(Player* player) {
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
            if (player) {
                player->TeleportTo(MAP_ID, _teleportPosition.GetPositionX(), _teleportPosition.GetPositionY(), _teleportPosition.GetPositionZ(), _teleportPosition.GetOrientation());
                return true;
            }
            return false;
        }

        void CheckDisconnectedPlayers(QuedPlayer* quedPlayer) {
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
        float _positionX = 4564.0f;
        float _positionY = -3098.0f;
        float _positionZ = 995.0f;
        float _orientation = 0;
        Position _teleportPosition = { _positionX, _positionY, _positionZ, _orientation };
    };

    CreatureAI* GetAI(Creature* creature) const {
        return new ffa_que_npcAI(creature);
    }

};

void AddSC_custom_script() {
    new ffa_que_npc();
}
