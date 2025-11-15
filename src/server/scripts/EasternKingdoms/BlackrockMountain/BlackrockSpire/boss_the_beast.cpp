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

#include "AreaTriggerScript.h"
#include "CreatureScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "blackrock_spire.h"
#include "../scripts/Custom/Timewalking/10Man.h"

enum Spells
{
    SPELL_FLAMEBREAK = 16785,
    SPELL_IMMOLATE = 15570,
    SPELL_FIREBALL = 16788,
    SPELL_FIREBLAST = 16144,
    SPELL_SUICIDE = 8329,
    SPELL_CARAPACE_CTHUN = 26156,
    SPELL_EAT_AURA_ONE = 833829,
    SPELL_EAT_AURA_TWO = 833830,
    SPELL_CAST_ON_SELF_ONE = 891243,
    SPELL_CAST_ON_SELF_TWO = 891244,
    SPELL_EAT_TARGET = 5
};

enum Events
{
    EVENT_FLAME_BREAK = 1,
    EVENT_IMMOLATE = 2,
    EVENT_FIREBALL = 3,
    EVENT_FIREBLAST = 4,
    EVENT_SPAWN_CUSTOM_ELITE = 5,
    EVENT_SPAWN_CUSTOM_BEASTMASTER = 6,
    EVENT_CHECK_FOR_EAT = 7,
    EVENT_CHECK_HEALTH = 8,
};

enum BeastMisc
{
    DATA_BEAST_REACHED              = 1,
    DATA_BEAST_ROOM                 = 2,
    BEAST_MOVEMENT_ID               = 1379690,

    NPC_BLACKHAND_ELITE             = 10317,

    SAY_BLACKHAND_DOOMED            = 0
};

enum NpcEntries
{
    NPC_TOPPER_MCNABB = 881402,
    NPC_MORRIS_LAWRY = 881405,
    NPC_DIRTY_LARRY = 819720,
    NPC_HARRIS_PILTON = 818756,
    NPC_MANKRIKS_WIFE = 831434,
    NPC_MILLHOUSE = 820977,
    NPC_KARU = 814874,
    NPC_DWARF_REFUGEE = 819077
};

enum AdditionalNpcEntries
{
    NPC_BLACKHAND_CUSTOM_ELITE = 810317,
    NPC_BLACKHAND_CUSTOM_BEASTMASTER = 810762
};

Position const OrcsRunawayPosition = { 34.163567f, -536.852356f, 110.935196f, 6.056306f };

class OrcMoveEvent : public BasicEvent
{
public:
    OrcMoveEvent(Creature* me) : _me(me) {}

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        _me->SetReactState(REACT_PASSIVE);
        Position movePos = _me->GetRandomPoint(OrcsRunawayPosition, 10.0f);
        _me->GetMotionMaster()->MovePoint(1, movePos);
        return true;
    }

private:
    Creature* _me;
};

class OrcDeathEvent : public BasicEvent
{
public:
    OrcDeathEvent(Creature* me) : _me(me) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        _me->CastSpell(_me, SPELL_SUICIDE, true);
        _me->CastSpell(_me, 861618, true);
        return true;
    }

private:
    Creature* _me;
};

constexpr static float FirewalPositionY = -505.f;

class boss_the_beast : public CreatureScript
{
public:
    boss_the_beast() : CreatureScript("boss_the_beast") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockSpireAI<boss_thebeastAI>(creature);
    }

    struct boss_thebeastAI : public BossAI
    {
        boss_thebeastAI(Creature* creature) : BossAI(creature, DATA_THE_BEAST), _beastReached(false), _orcYelled(false) {}
        
        GameObject* _firewall = nullptr;
        
        void Reset() override
        {
            _Reset();
            DespawnFirewall();
            ResetNearbyCreatures();
            me->SetObjectScale(1.0f);
            events.Reset();
            
            if (_beastReached)
            {
                me->GetMotionMaster()->MovePath(BEAST_MOVEMENT_ID, true);
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            SpawnFirewall();
            DoCastSelf(SPELL_CARAPACE_CTHUN, true);
            events.ScheduleEvent(EVENT_FLAME_BREAK, 12s);
            events.ScheduleEvent(EVENT_IMMOLATE, 3s);
            events.ScheduleEvent(EVENT_FIREBALL, 8s, 21s);
            events.ScheduleEvent(EVENT_FIREBLAST, 5s, 8s);
            events.ScheduleEvent(EVENT_SPAWN_CUSTOM_ELITE, 15s);
            events.ScheduleEvent(EVENT_SPAWN_CUSTOM_BEASTMASTER, 15s);
            events.ScheduleEvent(EVENT_CHECK_FOR_EAT, 1s);
            events.ScheduleEvent(EVENT_CHECK_HEALTH, 1s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            DespawnFirewall();
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (!players.IsEmpty())
            {
                uint32 baseRewardLevel = 1;
                bool isDungeon = me->GetMap()->IsDungeon();

                for (auto const& playerPair : players)
                {
                    if (Player* player = playerPair.GetSource())
                    {
                        DistributeChallengeRewards(player, me, baseRewardLevel, isDungeon);
                    }
                }
            }
        }

        void SpawnFirewall()
        {
            if (!_firewall)
            {
                _firewall = me->SummonGameObject(8186728, 25.9118f, -508.278f, 110.948f, 4.74139f, 0, 0, 0, 0, 600000, false, GO_SUMMON_TIMED_OR_CORPSE_DESPAWN);
            }
        }

        void DespawnFirewall()
        {
            if (_firewall)
            {
                _firewall->Delete();
                _firewall = nullptr;
            }
        }

        void ResetNearbyCreatures()
        {
            std::vector<uint32> creatures = {
                NPC_TOPPER_MCNABB, NPC_MORRIS_LAWRY, NPC_DIRTY_LARRY, NPC_HARRIS_PILTON,
                NPC_MANKRIKS_WIFE, NPC_MILLHOUSE, NPC_KARU, NPC_DWARF_REFUGEE
            };

            for (uint32 entry : creatures)
            {
                std::list<Creature*> foundCreatures;
                GetCreatureListWithEntryInGrid(foundCreatures, me, entry, 200.0f); // Search within 200f radius
                for (Creature* creature : foundCreatures)
                {
                    creature->DespawnOrUnsummon();
                    creature->Respawn();
                }
            }
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
                case DATA_BEAST_ROOM:
                {
                    if (!_orcYelled)
                    {
                        if (_nearbyOrcsGUIDs.empty())
                        {
                            FindNearbyOrcs();
                        }

                        //! vector still empty, creatures are missing
                        if (_nearbyOrcsGUIDs.empty())
                        {
                            return;
                        }

                        _orcYelled = true;

                        bool yelled = false;
                        for (ObjectGuid guid : _nearbyOrcsGUIDs)
                        {
                            if (Creature* orc = ObjectAccessor::GetCreature(*me, guid))
                            {
                                if (!yelled)
                                {
                                    yelled = true;
                                    orc->AI()->Talk(SAY_BLACKHAND_DOOMED);
                                    me->GetMotionMaster()->MovePath(BEAST_MOVEMENT_ID, true); // Beast starts moving
                                }

                                orc->m_Events.AddEvent(new OrcMoveEvent(orc), me->m_Events.CalculateTime(1 * IN_MILLISECONDS));
                                orc->m_Events.AddEvent(new OrcDeathEvent(orc), me->m_Events.CalculateTime(5 * IN_MILLISECONDS));
                            }
                        }
                    }
                    break;
                }
                case DATA_BEAST_REACHED:
                {
                    if (!_beastReached)
                    {
                        _beastReached = true;
                        me->GetMotionMaster()->MovePath(BEAST_MOVEMENT_ID, true);

                        // There is a chance player logged in between areatriggers (realm crash or restart)
                        // executing part of script which happens when player enters boss room
                        // otherwise we will see weird behaviour when someone steps on the previous areatrigger (dead mob yelling/moving)
                        SetData(DATA_BEAST_ROOM, DATA_BEAST_ROOM);
                    }
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                return;
            }

            if (me->GetPositionY() > FirewalPositionY)
            {
                EnterEvadeMode();
                return;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
            {
                return;
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FLAME_BREAK:
                        DoCastVictim(SPELL_FLAMEBREAK);
                        events.ScheduleEvent(EVENT_FLAME_BREAK, 10s);
                        break;
                    case EVENT_IMMOLATE:
                        DoCastRandomTarget(SPELL_IMMOLATE, 0, 100.0f);
                        events.ScheduleEvent(EVENT_IMMOLATE, 8s);
                        break;
                    case EVENT_FIREBALL:
                        DoCastVictim(SPELL_FIREBALL, true);
                        events.ScheduleEvent(EVENT_FIREBALL, 8s, 21s);
                        if (events.GetNextEventTime(EVENT_FIREBLAST) < 3 * IN_MILLISECONDS)
                        {
                            events.RescheduleEvent(EVENT_FIREBLAST, 3s);
                        }
                        break;
                    case EVENT_FIREBLAST:
                        DoCastVictim(SPELL_FIREBLAST);
                        events.ScheduleEvent(EVENT_FIREBLAST, 5s, 8s);
                        if (events.GetNextEventTime(EVENT_FIREBALL) < 3 * IN_MILLISECONDS)
                        {
                            events.RescheduleEvent(EVENT_FIREBALL, 3s);
                        }
                        break;
                    case EVENT_SPAWN_CUSTOM_ELITE:
                        SpawnNPC(NPC_BLACKHAND_CUSTOM_ELITE, 23.1709f, -514.253f, 110.946f, 4.5574f);
                        SpawnNPC(NPC_BLACKHAND_CUSTOM_ELITE, 28.8684f, -514.442f, 110.946f, 4.5574f);
                        events.ScheduleEvent(EVENT_SPAWN_CUSTOM_ELITE, 35s);
                        break;
                    case EVENT_SPAWN_CUSTOM_BEASTMASTER:
                        SpawnNPC(NPC_BLACKHAND_CUSTOM_BEASTMASTER, 26.2728f, -514.362f, 110.946f, 4.5574f);
                        events.ScheduleEvent(EVENT_SPAWN_CUSTOM_BEASTMASTER, 35s);
                        break;
                    case EVENT_CHECK_FOR_EAT:
                        CheckForEat();
                        events.ScheduleEvent(EVENT_CHECK_FOR_EAT, 1s);
                        break;
                    case EVENT_CHECK_HEALTH:
                        if (me->HealthBelowPct(20) && me->HasAura(891244))
                        {
                            HandleLowHealthEvent();
                        }
                        events.ScheduleEvent(EVENT_CHECK_HEALTH, 1s); 
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                {
                    return;
                }
            }

            DoMeleeAttackIfReady();
        }

        void SpawnNPC(uint32 entry, float x, float y, float z, float o)
        {
            Creature* npc = me->SummonCreature(entry, x, y, z, o, TEMPSUMMON_TIMED_DESPAWN, 600000);
            if (npc)
            {
                npc->CastSpell(npc, 51347, true); 
                npc->SetInCombatWithZone();

                if (entry == NPC_BLACKHAND_CUSTOM_BEASTMASTER)
                {
                    std::vector<std::string> dialogues = {
                        "The chains are broken! Rally to contain The Beast!",
                        "Foolish intruders, your meddling has unleashed doom upon us all!",
                        "Containment has failed! All units, converge and suppress The Beast!",
                        "To arms! The Beast must not escape the confines of these walls!",
                        "Secure the perimeter! Do not let The Beast gain ground!",
                        "This monstrosity will destroy everything! Stop it at all costs!"
                    };

                    int randomIndex = urand(0, dialogues.size() - 1);
                    npc->Yell(dialogues[randomIndex].c_str(), LANG_UNIVERSAL);
                }
            }
        }
        
        void FindNearbyOrcs()
        {
            std::list<Creature*> temp;
            me->GetCreatureListWithEntryInGrid(temp, NPC_BLACKHAND_ELITE, 50.0f);
            for (Creature* creature : temp)
            {
                if (creature->IsAlive())
                {
                    _nearbyOrcsGUIDs.push_back(creature->GetGUID());
                }
            }
        }

        void CheckForEat()
        {
            std::vector<uint32> npcEntries = { NPC_TOPPER_MCNABB, NPC_MORRIS_LAWRY, NPC_DIRTY_LARRY, NPC_HARRIS_PILTON, NPC_MANKRIKS_WIFE, NPC_MILLHOUSE, NPC_KARU, NPC_DWARF_REFUGEE };

            for (uint32 npcEntry : npcEntries)
            {
                std::list<Creature*> list;
                GetCreatureListWithEntryInGrid(list, me, npcEntry, 7.0f);

                for (Creature* npc : list)
                {
                    if (npc->HasAura(SPELL_EAT_AURA_ONE) || npc->HasAura(SPELL_EAT_AURA_TWO))
                    {
                        me->GetMotionMaster()->MoveCharge(npc->GetPositionX(), npc->GetPositionY(), npc->GetPositionZ());

                        if (npc->HasAura(SPELL_EAT_AURA_ONE))
                        {
                            npc->CastSpell(npc, 861618, true);
                            me->CastSpell(npc, SPELL_EAT_TARGET, true);
                            me->CastSpell(me, SPELL_CAST_ON_SELF_ONE, true);
                            if (npc->GetEntry() == NPC_MANKRIKS_WIFE)
                            {
                                npc->Say("Tell my husband... I love him...", LANG_UNIVERSAL, nullptr);
                            }
                            EmoteEating(npc);
                        }
                        else if (npc->HasAura(SPELL_EAT_AURA_TWO))
                        {
                            npc->DespawnOrUnsummon();
                            me->CastSpell(me, SPELL_CAST_ON_SELF_TWO, true);
                            EmoteEating(npc);
                            me->PlayDirectSound(188053);
                        }

                        events.DelayEvents(500); 
                        break; 
                    }
                }
            }
        }

        void EmoteEating(Creature* npc)
        {
            if (npc)
            {
                std::string name = npc->GetName();
                std::ostringstream ss;
                ss << "Beast eats " << name << " and grows in size...";
                me->TextEmote(ss.str(), nullptr, true);

                float currentScale = me->GetFloatValue(OBJECT_FIELD_SCALE_X);
                me->SetObjectScale(currentScale * 1.24f); 
            }
        }

        void HandleLowHealthEvent()
        {
            me->CastSpell(me, 5, true); 
            me->CastSpell(me, 861618, true); 

            if (Creature* summoned = me->SummonCreature(820978, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 600000)) // Summon NPC at his location for 10 minutes
            {
                summoned->CastSpell(summoned, 1467, true); // Immediately cast a spell when summoned

                summoned->m_Events.AddEvent(new DelayedSpeechEvent(summoned), summoned->m_Events.CalculateTime(2000));
            }
        }

        class DelayedSpeechEvent : public BasicEvent
        {
            Creature* _creature;

        public:
            explicit DelayedSpeechEvent(Creature* creature) : _creature(creature) {}

            bool Execute(uint64 /*execTime*/, uint32 /*diff*/)
            {
                if (_creature && _creature->IsAlive())
                {
                    _creature->Say("And I thought they smelled bad on the outside... We never speak of this, you hear me?", LANG_UNIVERSAL, nullptr);
                    _creature->PlayDirectSound(188052);  
                }
                return true;  
            }
        };

    private:
        bool _beastReached;
        bool _orcYelled;
        GuidVector _nearbyOrcsGUIDs;
    };
};

//! The beast room areatrigger, this one triggers boss pathing. (AT Id 2066)
class at_trigger_the_beast_movement : public AreaTriggerScript
{
public:
    at_trigger_the_beast_movement() : AreaTriggerScript("at_trigger_the_beast_movement") { }

    bool OnTrigger(Player* player, AreaTrigger const* /*at*/) override
    {
        if (player->IsGameMaster())
        {
            return false;
        }

        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Creature* beast = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_THE_BEAST)))
            {
                beast->AI()->SetData(DATA_BEAST_REACHED, DATA_BEAST_REACHED);
            }

            return true;
        }

        return false;
    }
};

class at_the_beast_room : public AreaTriggerScript
{
public:
    at_the_beast_room() : AreaTriggerScript("at_the_beast_room") { }

    bool OnTrigger(Player* player, AreaTrigger const* /*at*/) override
    {
        if (player->IsGameMaster())
        {
            return false;
        }

        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Creature* beast = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_THE_BEAST)))
            {
                beast->AI()->SetData(DATA_BEAST_ROOM, DATA_BEAST_ROOM);
            }

            return true;
        }

        return false;
    }
};

void AddSC_boss_thebeast()
{
    new boss_the_beast();
    new at_trigger_the_beast_movement();
    new at_the_beast_room();
}
