//
//#include "ScriptMgr.h"
//#include "ScriptedCreature.h"
//#include "ScriptedGossip.h"
//#include "Player.h"
//#include "LFGMgr.h"
//#include "Chat.h"
//#include "Position.h"
//#include "WorldSession.h"
//#include "World.h"
//#include <ctime>
//
//#define JOIN_QUE         "Join que"
//#define LEAVE_QUE        "Leave que"
//#define CURRENT_STATUS   "Que status"
//#define ACCEPT_QUE       "Yes"
//#define DECLINE_QUE      "No"
//#define ASK_ME_LATER     "Ask me later"
//
//enum Options {
//    JOIN_QUE_ACTION = 1,
//    LEAVE_QUE_ACTION = 2,
//    QUE_STATUS_ACTION = 3,
//    ACCEPT_QUE_ACTION = 4,
//    DECLINE_QUE_ACTION = 5,
//    ASK_ME_LATER_ACTION = 6
//};
//
//enum Misc {
//    MAP_ID = 1,
//    QUE_START_COUNTDOWN = 60,
//    MIN_PLAYERS = 10,
//    ASK_ME_LATER_DELAY = 30,
//    CHECK_DISCONNECT_DELAY = 60,
//    SEND_ACCEPT_DELAY = 60,
//    WAIT_FOR_BG_START = 180
//};
//
//class custom_script : public CreatureScript {
//public:
//    custom_script() : CreatureScript("custom_script") {}
//
//    struct custom_scriptAI : public ScriptedAI {
//    public:
//        custom_scriptAI(Creature* creature) : ScriptedAI(creature) {}
//
//        class QuedPlayers {
//        public:
//            QuedPlayers(Player const* player) {
//                this->_player = player;
//            }
//
//            Player const* GetPlayer() {
//                return this->_player;
//            } 
//
//            uint32 GetCheckDisconnectTime() {
//                return _checkDisconnectTime;
//            }
//
//            uint32 GetAskMeLaterTime() {
//                return _askMeLaterTime;
//            }
//
//            uint32 GetSendGossipAgainTime() {
//                return _sendGossipAgainTime;
//            }
//
//            bool IsDisconnectCheckReady() {
//                if (_checkDisconnectTime + CHECK_DISCONNECT_DELAY <= std::time(0)) {
//                    return true;
//                }
//                return false;
//            }
//
//            bool IsAskMeLaterReady() {
//                if (_askMeLaterTime + ASK_ME_LATER_DELAY <= std::time(0)) {
//                    return true;
//                }
//                return false;
//            }
//
//            bool IsSendGossipReady() {
//                if (_sendGossipAgainTime + SEND_ACCEPT_DELAY <= std::time(0)) {
//                    return true;
//                }
//                return false;
//            }
//
//        private:
//            Player const* _player;
//            uint32 _checkDisconnectTime = std::time(0);
//            uint32 _askMeLaterTime = std::time(0);
//            uint32 _sendGossipAgainTime = std::time(0);
//        };
//
//        void UpdateAI(uint32 diff) override {
//            RemovePlayerFromQueOnDisconnect(diff);
//            if (AmountOfPlayersInQue() >= 1) {
//                //if (!_isAcceptGossipSent) {
//                    //_isAcceptGossipSent = true;
//                SendAcceptToAll();
//                //}
//            }
//        }
//
//        bool GossipHello(Player* player) override {
//            me->SetFacingToObject((Unit*)player);
//
//            // Join que gossip
//            AddGossipItemFor(player, GOSSIP_ICON_CHAT, JOIN_QUE, GOSSIP_SENDER_MAIN, JOIN_QUE_ACTION);
//            SendGossipMenuFor(player, JOIN_QUE_ACTION, me->GetGUID());
//
//            // Leave que gossip
//            if (IsPlayerInQue(player)) {
//                AddGossipItemFor(player, GOSSIP_ICON_CHAT, LEAVE_QUE, GOSSIP_SENDER_MAIN, LEAVE_QUE_ACTION);
//            }
//            SendGossipMenuFor(player, LEAVE_QUE_ACTION, me->GetGUID());
//
//            // Get current status
//            AddGossipItemFor(player, GOSSIP_ICON_CHAT, CURRENT_STATUS, GOSSIP_SENDER_MAIN, QUE_STATUS_ACTION);
//            SendGossipMenuFor(player, QUE_STATUS_ACTION, me->GetGUID());
//            return true;
//        }
//
//        bool GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override {
//            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
//            ClearGossipMenuFor(player);
//            switch (action) {
//            case JOIN_QUE_ACTION:
//                player->SetHealth(2000);
//                SendGossipMenuFor(player, JOIN_QUE_ACTION, me->GetGUID());
//                AddPlayerToQue(player);
//                CloseGossipMenuFor(player);
//                break;
//            case LEAVE_QUE_ACTION:
//                player->SetHealth(1);
//                SendGossipMenuFor(player, LEAVE_QUE_ACTION, me->GetGUID());
//                RemovePlayerFromQue(player);
//                CloseGossipMenuFor(player);
//                break;
//            case QUE_STATUS_ACTION:
//                player->SetHealth(player->GetMaxHealth());
//                SendGossipMenuFor(player, QUE_STATUS_ACTION, me->GetGUID());
//                me->Say("Amount of players in que: " + std::to_string(_quedPlayers.size()) + std::string("/10"), LANG_UNIVERSAL);
//                CloseGossipMenuFor(player);
//                break;
//            case ACCEPT_QUE_ACTION:
//                me->Say("Clicked yes", LANG_UNIVERSAL);
//                CloseGossipMenuFor(player);
//                TeleportPlayersToBG();
//                break;
//            case DECLINE_QUE_ACTION:
//                me->Say("Clicked no", LANG_UNIVERSAL);
//                // Remove player from que.
//                RemovePlayerFromQue(player);
//                break;
//            case ASK_ME_LATER_ACTION:
//                break;
//
//            }
//            return true;
//        }
//
//        bool AddPlayerToQue(Player* player) {
//            if (!(std::find(_quedPlayers.begin(), _quedPlayers.end(), player) != _quedPlayers.end())) {
//                _quedPlayers.push_back(player);
//                ChatHandler(player->GetSession()).SendSysMessage("You have been added to que successfully!");
//                return true;
//            }
//            ChatHandler(player->GetSession()).SendSysMessage("Player already in que!");
//            return false;
//        }
//
//        bool RemovePlayerFromQue(Player* player) {
//            if ((std::find(_quedPlayers.begin(), _quedPlayers.end(), player) != _quedPlayers.end())) {
//                _quedPlayers.erase(std::remove(_quedPlayers.begin(), _quedPlayers.end(), player), _quedPlayers.end());
//                ChatHandler(player->GetSession()).SendSysMessage("You have been removed from que successfully!");
//                return true;
//            }
//            ChatHandler(player->GetSession()).SendSysMessage("Player was not found!");
//            return false;
//        }
//
//        void SendAcceptToAll() {
//            if (_sendGossipMenuTime + SEND_ACCEPT_DELAY <= std::time(0) || !_isAcceptGossipSent) {
//                _isAcceptGossipSent = true;
//                ChatHandler(_quedPlayers.at(0)->GetSession()).SendSysMessage("Que is ready");
//                _sendGossipMenuTime = std::time(0);
//                for (Player* player : _quedPlayers) {
//                    SendAcceptTo(player);
//                }
//            }
//        }
//
//        bool SendAcceptTo(Player* player) {
//
//            ClearGossipMenuFor(player);
//            AddGossipItemFor(player, GOSSIP_ICON_CHAT, ACCEPT_QUE, GOSSIP_SENDER_MAIN, ACCEPT_QUE_ACTION);
//            SendGossipMenuFor(player, ACCEPT_QUE_ACTION, me->GetGUID());
//
//            AddGossipItemFor(player, GOSSIP_ICON_CHAT, DECLINE_QUE, GOSSIP_SENDER_MAIN, DECLINE_QUE_ACTION);
//            SendGossipMenuFor(player, DECLINE_QUE_ACTION, me->GetGUID());
//
//            AddGossipItemFor(player, GOSSIP_ICON_CHAT, ASK_ME_LATER, GOSSIP_SENDER_MAIN, ASK_ME_LATER_ACTION);
//            SendGossipMenuFor(player, ASK_ME_LATER_ACTION, me->GetGUID());
//            return true;
//        }
//
//        uint32 AmountOfPlayersInQue() {
//            return _quedPlayers.size();
//        }
//
//        bool TeleportPlayerToBG(Player* player) {
//            if (player) {
//                player->TeleportTo(MAP_ID, _teleportPosition.GetPositionX(), _teleportPosition.GetPositionY(), _teleportPosition.GetPositionZ(), _teleportPosition.GetOrientation());
//                return true;
//            }
//            return false;
//        }
//
//        void TeleportPlayersToBG() {
//            for (Player* player : _quedPlayers) {
//                TeleportPlayerToBG(player);
//            }
//        }
//
//        void RemovePlayerFromQueOnDisconnect(uint32 diff) {
//            if (_checkDisconnectTime <= diff) {
//
//            } else {
//            }
//
//
//            if (_checkDisconnectTime + CHECK_DISCONNECT_DELAY <= std::time(0)) {
//                _checkDisconnectTime = std::time(0);
//                for (Player* player : _quedPlayers) {
//                    if (!player->IsInWorld()) {
//                        RemovePlayerFromQue(player);
//                    }
//                }
//            }
//        }
//
//        void SendGossipMenuToAll() {
//            if (_sendGossipMenuTime + CHECK_DISCONNECT_DELAY <= std::time(0)) {
//                _sendGossipMenuTime = std::time(0);
//                for (Player* player : _quedPlayers) {
//                }
//            }
//        }
//
//        void CancelQue() {
//            for (Player* player : _quedPlayers) {
//                SendCancelQueTo(player);
//            }
//        }
//
//        bool SendCancelQueTo(Player* player) {
//            CloseGossipMenuFor(player);
//            return true;
//        }
//
//        bool IsPlayerInQue(Player* player) {
//            return (std::find(_quedPlayers.begin(), _quedPlayers.end(), player) != _quedPlayers.end());
//        }
//
//    private:
//        std::vector<Player*> _quedPlayers;
//        std::vector<QuedPlayers*> _quedPlayerss;
//        bool _isAcceptGossipSent = false;
//        bool _isQueStarted = false;
//        uint32 _checkDisconnectTime = CHECK_DISCONNECT_DELAY;
//        uint32 _sendGossipMenuTime = std::time(0);
//        uint32 _mapId = 1;
//        float _positionX = 4564.0f;
//        float _positionY = -3098.0f;
//        float _positionZ = 995.0f;
//        float _orientation = 0;
//        Position _teleportPosition = { _positionX, _positionY, _positionZ, _orientation };
//    };
//
//    CreatureAI* GetAI(Creature* creature) const {
//        return new custom_scriptAI(creature);
//    }
//
//};
//
//void AddSC_custom_script() {
//    new custom_script();
//}
