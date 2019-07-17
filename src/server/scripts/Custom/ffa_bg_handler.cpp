
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "Map.h"
#include "RefManager.h"
#include <ctime>
#include "Chat.h"

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
            //if (me->GetMap()) {
            //    map = me->GetMap();
            //    Map::PlayerList const& players = map->GetPlayers();
            //    for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter) {
            //        Player* player = iter->GetSource();
            //        me->Say(player->GetName(), LANG_UNIVERSAL);
            //    }
            //}
        }

        void Reset() override {
            me->SetVisible(false);
        }

        void UpdateAI(uint32 diff) override {
            if (!_map->isEmpty()) {
                _initializeTimer = std::time(0);
            }
            if (!IsReadyToStart() || !_hasBGStarted) {
                addPlayersToVector();
                //WaitingForPlayersToJoin();
            }
        }

        void WaitingForPlayersToJoin() {

        }

        void addPlayersToVector() {
            Map::PlayerList const& players = _map->GetPlayers();
            for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter) {
                Player* player = iter->GetSource();
                if (!(std::find(_players.begin(), _players.end(), player) != _players.end())) {
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
