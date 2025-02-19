/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#ifndef TRINITY_SCRIPTEDFOLLOWERAI_H
#define TRINITY_SCRIPTEDFOLLOWERAI_H

#include "ScriptedCreature.h"

class Quest;

enum FollowerState : uint32
{
    STATE_FOLLOW_NONE       = 0x000,
    STATE_FOLLOW_INPROGRESS = 0x001, // must always have this state for any follow
    STATE_FOLLOW_RETURNING  = 0x002, // when returning to combat start after being in combat
    STATE_FOLLOW_PAUSED     = 0x004, // disables following
    STATE_FOLLOW_COMPLETE   = 0x008, // follow is completed and may end
    STATE_FOLLOW_PREEVENT   = 0x010, // not implemented (allow pre event to run, before follow is initiated)
    STATE_FOLLOW_POSTEVENT  = 0x020  // can be set at complete and allow post event to run
};

class TC_GAME_API FollowerAI : public ScriptedAI
{
    public:
        explicit FollowerAI(Creature* creature);
        ~FollowerAI() { }

        void MovementInform(uint32 type, uint32 id) override;
        void AttackStart(Unit*) override;
        void MoveInLineOfSight(Unit*) override;
        void EnterEvadeMode(EvadeReason /*why*/ = EVADE_REASON_OTHER) override;
        void JustDied(Unit*) override;
        void JustAppeared() override;
        // the "internal" update, calls UpdateFollowerAI()
        void UpdateAI(uint32) override;

        // used when it's needed to add code in update (abilities, scripted events, etc)
        virtual void UpdateFollowerAI(uint32);

        void StartFollow(Player* player, uint32 factionForFollower = 0, Quest const* quest = nullptr);
        // if special event require follow mode to hold/resume during the follow
        void SetFollowPaused(bool paused);
        void SetFollowComplete(bool withEndEvent = false);

        bool HasFollowState(uint32 uiFollowState) { return (_followState & uiFollowState) != 0; }

    protected:
        Player* GetLeaderForFollower();

    private:
        void AddFollowState(uint32 followState) { _followState |= followState; }
        void RemoveFollowState(uint32 followState) { _followState &= ~followState; }
        bool AssistPlayerInCombatAgainst(Unit* who);

        ObjectGuid _leaderGUID;
        uint32 _updateFollowTimer;
        uint32 _followState;

        Quest const* _questForFollow;
};

#endif
