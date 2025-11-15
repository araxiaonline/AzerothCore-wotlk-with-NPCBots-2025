/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "InstanceMapScript.h"
#include "InstanceScript.h"
#include "maraudon.h"
//Dinkle
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "../../Custom/Timewalking/10Man.h"

enum Spells
{
    SPELL_THRASH = 10412,
    SPELL_BOULDER = 21832,
    SPELL_DUST_FIELD = 21909,
    SPELL_REPULSIVE_GAZE = 21869,
    SPELL_FRENZY = 8269,
    SPELL_EARTH_SHOCK = 10412,
};

enum Events
{
    EVENT_THRASH = 1,
    EVENT_BOULDER,
    EVENT_DUST_FIELD,
    EVENT_REPULSIVE_GAZE,
    EVENT_SUMMON_CREATURE,
    EVENT_EARTH_SHOCK,
};

//end Dinkle
class instance_maraudon : public InstanceMapScript
{
public:
    instance_maraudon() : InstanceMapScript("instance_maraudon", 349) { }

    struct instance_maraudon_InstanceMapScript : public InstanceScript
    {
        instance_maraudon_InstanceMapScript(Map* map) : InstanceScript(map)
        {
        }

        void Initialize() override
        {
            SetHeaders(DataHeader);
            memset(&_encounters, 0, sizeof(_encounters));
        }

        void OnGameObjectCreate(GameObject* gameobject) override
        {
            switch (gameobject->GetEntry())
            {
                case GO_CORRUPTION_SPEWER:
                    if (_encounters[TYPE_NOXXION] == DONE)
                        HandleGameObject(ObjectGuid::Empty, true, gameobject);
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case TYPE_NOXXION:
                    _encounters[type] = data;
                    break;
            }

            if (data == DONE)
                SaveToDB();
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            data >> _encounters[0];
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << _encounters[0];
        }

    private:
        uint32 _encounters[MAX_ENCOUNTERS];
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_maraudon_InstanceMapScript(map);
    }
};
//Dinkle
struct creature_princess_theradrasAI : public ScriptedAI
{
    creature_princess_theradrasAI(Creature* creature) : ScriptedAI(creature) {}

    
    void Reset() override
    {
        events.Reset();
        DoCast(me, SPELL_THRASH, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EVENT_BOULDER, urand(2000, 7000));
        events.ScheduleEvent(EVENT_DUST_FIELD, 15000);
        events.ScheduleEvent(EVENT_REPULSIVE_GAZE, 10000);
        events.ScheduleEvent(EVENT_EARTH_SHOCK, urand(3000, 6000));
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->SummonCreature(12238, 28.067f, 61.875f, -123.405f, 4.67f, TEMPSUMMON_CORPSE_DESPAWN);
        Map::PlayerList const& players = me->GetMap()->GetPlayers();
        if (players.begin() != players.end())
        {
            uint32 baseRewardLevel = 1;
            bool isDungeon = me->GetMap()->IsDungeon();

            Player* player = players.begin()->GetSource();
            if (player)
            {
                DistributeChallengeRewards(player, me, baseRewardLevel, isDungeon);
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (me->HealthBelowPct(30) && !me->HasAura(SPELL_FRENZY))
        {
            DoCast(me, SPELL_FRENZY, true);
        }

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_BOULDER:
                DoCastRandomTarget(SPELL_BOULDER, 0, 30.0f, false);
                events.ScheduleEvent(EVENT_BOULDER, urand(16000, 18000));
                break;
            case EVENT_DUST_FIELD:
                DoCast(me, SPELL_DUST_FIELD);
                events.ScheduleEvent(EVENT_DUST_FIELD, 30000);
                break;
            case EVENT_REPULSIVE_GAZE:
                DoCast(me, SPELL_REPULSIVE_GAZE);
                events.ScheduleEvent(EVENT_REPULSIVE_GAZE, 20000);
                break;
            case EVENT_EARTH_SHOCK:
                DoCastRandomTarget(SPELL_EARTH_SHOCK, 0, 25.0f, false);
                events.ScheduleEvent(EVENT_EARTH_SHOCK, urand(8000, 12000));
                break;
            }
        }

        DoMeleeAttackIfReady();
    }
};
//end Dinkle
void AddSC_instance_maraudon()
{
    new instance_maraudon();
    RegisterCreatureAI(creature_princess_theradrasAI);
}
