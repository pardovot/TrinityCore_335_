#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "Chat.h"
#include "Group.h"
#include "WorldSession.h"
#include "Player.h"
#include "RBAC.h"
#include "Position.h"
#include "Language.h"
#include "SpellAuras.h"

enum misc {
    LEAVE_COMMAND = 200,
    BG_ZONE_ID = 616,
    LEAVE_SICKNESS_TIME = 900
};

enum Spells {
    BG_SPELL_DESERTER = 26013
};

class ffa_bg_commands : public CommandScript
{
public:
    ffa_bg_commands() : CommandScript("test_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override {

        static std::vector<ChatCommand> commands =
        {
            { "leave", LEAVE_COMMAND, false, &HandleLeaveCommand, ""}
        };

        return commands;
    }

    static bool HandleLeaveCommand(ChatHandler* handler, const char* args) {
        Player* player = GetPlayer(handler);
        if (player->GetZoneId() == BG_ZONE_ID) {
            Say(player, std::string("Leaving BG").c_str());
            HandleDeserterAdd(handler);
            return true;
        } else {
            ThereIsNoSuchCommand(player);
            return true;
        }
        return false;
    }

private:
    static Player* GetPlayer(ChatHandler* handler) {
        return handler->GetSession()->GetPlayer();
    }

    static void Say(Player* player, std::string str) {
        player->Say(str, LANG_UNIVERSAL);
    }

    static void ThereIsNoSuchCommand(Player* player) {
        ChatHandler(player->GetSession()).SendSysMessage("There is no such command");
    }

    static bool HandleDeserterAdd(ChatHandler* handler) {
        Player* player = GetPlayer(handler);
        if (!player) {
            handler->SendSysMessage(LANG_UNIVERSAL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Aura* aura = player->AddAura(BG_SPELL_DESERTER, player);

        if (!aura) {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }
        aura->SetDuration(LEAVE_SICKNESS_TIME * IN_MILLISECONDS);

        return true;
    }

};

void AddSC_ffa_bg_commands() {
    new ffa_bg_commands();
}

