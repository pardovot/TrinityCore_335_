
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "Map.h"
#include "RefManager.h"
#include <ctime>
#include "Chat.h"
#include "ffa_que_npc.cpp"

enum test : uint32 {
    MIN_NUMBER_OF_PLAYERS = 10,
    TIME_UNTIL_BG_STARTS = 300
};

enum whisper {
    MY_NUM = 2
};

class ffa_bg_handler : public CreatureScript {
public:
    ffa_bg_handler() : CreatureScript("custom_handler") {}

    struct ffa_bg_handlerAI : public ScriptedAI {
        ffa_bg_handlerAI(Creature* creature) : ScriptedAI(creature) {
            _map = me->GetMap();
        }

        void Reset() override {
            me->SetVisible(false);
        }

        void UpdateAI(uint32 diff) override {
            if (GetAmountOfPlayersInMap() >= 1) {
                Initialize();
            }

            if (_initializeTimer > 0 && _players.size() != playersInQue.size() && !_hasBGStarted) {
                AddPlayersToVector();
            }

            //if (!IsReadyToStart() || !_hasBGStarted) {
                //WaitingForPlayersToJoin();
            //}
        }

        bool StartBG() {
            return _players.size() == playersInQue.size();
        }

        void Initialize() {
            _initializeTimer = std::time(0);
        }

        bool WaitForPlayersToJoin() {
            return std::time(0) <= _initializeTimer + WAIT_FOR_BG_START && !_hasBGStarted;
        }

        void AddPlayersToVector() {
            Map::PlayerList const& players = _map->GetPlayers();
            for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter) {
                Player* player = iter->GetSource();
                if (!(std::find(_players.begin(), _players.end(), player) != _players.end()) && (std::find(playersInQue.begin(), playersInQue.end(), player) != playersInQue.end())) {
                    _players.push_back(player);
                }
            }
        }

        bool IsReadyToStart() {
            if (GetAmountOfPlayersInMap() >= MIN_NUMBER_OF_PLAYERS) {
                return true;
            }
            return false;
        }

        uint32 GetAmountOfPlayersInMap() {
            return _map->GetPlayersCountExceptGMs();
        }

    private:
        std::vector<Player*> _players;
        bool _hasBGStarted = false;
        bool _hasEnoughPlayers = false;
        Map* _map;
        uint32 _initializeTimer;
    };

    CreatureAI* GetAI(Creature* creature) const {
        return new ffa_bg_handlerAI(creature);
    }
};

void AddSC_ffa_bg_handler() {
    new ffa_bg_handler;
}


            //if (me->GetMap()) {
            //    map = me->GetMap();
            //    Map::PlayerList const& players = map->GetPlayers();
            //    for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter) {
            //        Player* player = iter->GetSource();
            //        me->Say(player->GetName(), LANG_UNIVERSAL);
            //    }
            //}
