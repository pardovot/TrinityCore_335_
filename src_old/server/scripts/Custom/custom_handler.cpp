
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "Map.h"
#include "RefManager.h"
#include <ctime>
#include "Chat.h"

enum test : uint32 {
    MIN_NUMBER_OF_PLAYERS = 10,
    DELAY_PER_PLAYERS_CHECK = 15
};

enum whisper {
    MY_NUM = 2
};

class custom_handler : public CreatureScript {
public:
    custom_handler() : CreatureScript("custom_handler") {}

    struct custom_handlerAI : public ScriptedAI {
        custom_handlerAI(Creature* creature) : ScriptedAI(creature) {
            if (me->GetMap()) {
                map = me->GetMap();
                Map::PlayerList const& players = map->GetPlayers();
                for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter) {
                    Player* player = iter->GetSource();
                    me->Say(player->GetName(), LANG_UNIVERSAL);
                }
            }
        }

        void Reset() override {
            //if (me->GetMap()) {
            //    map = me->GetMap();
            //    Map::PlayerList const& players = map->GetPlayers();
            //    for (Map::PlayerList::const_iterator iter = players.begin(); iter != players.end(); ++iter) {
            //        Player* player = iter->GetSource();
            //        me->Say(player->GetName(), LANG_UNIVERSAL);
            //    }
            //}
            me->SetVisible(false);
        }

        void UpdateAI(uint32 diff) override {
            if (!IsReadyToStart()) {
                WaitingForPlayersToJoin();
            }
        }

        void WaitingForPlayersToJoin() {
            if (start + DELAY_PER_PLAYERS_CHECK <= std::time(0)) {
                start = std::time(0);
                me->Say(std::to_string(DELAY_PER_PLAYERS_CHECK) + std::string(" have passed."), LANG_UNIVERSAL);
                me->Whisper("asdasda", LANG_UNIVERSAL, map->GetPlayers().getFirst()->GetSource(), true);
            }
        }

        bool IsReadyToStart() {
            if (GetAmountOfPlayersInMap() < MIN_NUMBER_OF_PLAYERS) {
                return false;
            }
            return true;
        }

        void collectAllPlayersInMap() {
        }

        uint32 GetAmountOfPlayersInMap() {
            return map->GetPlayersCountExceptGMs();
        }

    private:
        std::vector<Player*> players;
        Map* map;
        int start = std::time(0);
    };

    CreatureAI* GetAI(Creature* creature) const {
        return new custom_handlerAI(creature);
    }
};

void AddSC_custom_handler() {
    new custom_handler;
}
