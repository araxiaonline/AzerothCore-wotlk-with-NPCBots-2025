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

#include "CreatureScript.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "blackrock_spire.h"

enum Text
{
    EMOTE_ONE_STACK                 = 0,
    EMOTE_TEN_STACK                 = 1,
    EMOTE_FREE_OF_BONDS             = 2,
    YELL_FREE_OF_BONDS              = 3
};

enum Spells
{
    SPELL_ENCAGED_EMBERSEER         = 15282, // Self on spawn
    SPELL_FIRE_SHIELD_TRIGGER       = 13377, // Self on spawn missing from 335 dbc triggers SPELL_FIRE_SHIELD every 3 sec
    SPELL_FIRE_SHIELD               = 13376, // Triggered by SPELL_FIRE_SHIELD_TRIGGER
    SPELL_FREEZE_ANIM               = 16245, // Self on event start
    SPELL_EMBERSEER_GROWING         = 16048, // Self on event start
    SPELL_EMBERSEER_GROWING_TRIGGER = 16049, // Triggered by SPELL_EMBERSEER_GROWING
    SPELL_EMBERSEER_FULL_STRENGTH   = 16047, // Emberseer Full Strength
    SPELL_FIRENOVA                  = 23462, // Combat
    SPELL_FLAMEBUFFET               = 823341, // Combat
    SPELL_PYROBLAST                 = 17274, // Combat
    SPELL_BLAST_WAVE                = 833918,
    // Blackhand Incarcerator Spells
    SPELL_ENCAGE_EMBERSEER          = 15281, // Emberseer on spawn
    SPELL_STRIKE                    = 15580, // Combat
    SPELL_ENCAGE                    = 16045, // Combat
    // Cast on player by altar
    SPELL_EMBERSEER_OBJECT_VISUAL   = 16532,
    SPELL_FIREBALL_VOLLEY           = 815285
};

enum Events
{
    // Respawn
    EVENT_RESPAWN                   = 1,
    // Pre fight
    EVENT_PRE_FIGHT_1               = 2,
    EVENT_PRE_FIGHT_2               = 3,
    // Combat
    EVENT_FIRENOVA                  = 4,
    EVENT_FLAMEBUFFET               = 5,
    EVENT_PYROBLAST                 = 6,
    // Hack due to trigger spell not in dbc
    EVENT_FIRE_SHIELD               = 7,
    EVENT_PRE_ENTER_COMBAT_1        = 8,
    EVENT_PRE_ENTER_COMBAT_2        = 9,
    EVENT_ENTER_COMBAT              = 10,
    EVENT_BLAST_WAVE                = 11, // Blast Wave
    EVENT_CHECK_MELEE               = 12, // Check for melee range
    EVENT_SPAWN_STABILIZER          = 13
};

class boss_pyroguard_emberseer : public CreatureScript
{
public:
    boss_pyroguard_emberseer() : CreatureScript("boss_pyroguard_emberseer") { }

    struct boss_pyroguard_emberseerAI : public BossAI
    {
        boss_pyroguard_emberseerAI(Creature* creature) : BossAI(creature, DATA_PYROGAURD_EMBERSEER) { }

        void Reset() override
        {
            RemoveAuraFromMobs(819753);
            DespawnCreatures(810316);
            me->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
            me->SetImmuneToPC(true);
            events.Reset();
            // Apply auras on spawn and reset
            // DoCast(me, SPELL_FIRE_SHIELD_TRIGGER); // Need to find this in old DBC if possible
            me->RemoveAura(SPELL_EMBERSEER_FULL_STRENGTH);
            me->RemoveAura(SPELL_EMBERSEER_GROWING);
            me->RemoveAura(SPELL_EMBERSEER_GROWING_TRIGGER);
            events.ScheduleEvent(EVENT_RESPAWN, 5s);
            // Hack for missing trigger spell
            events.ScheduleEvent(EVENT_FIRE_SHIELD, 3s);

            // Open doors on reset
            if (instance->GetBossState(DATA_PYROGAURD_EMBERSEER) == IN_PROGRESS)
                OpenDoors(false); // Opens 2 entrance doors
        }

        void DespawnCreatures(uint32 entry)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, entry, 100.0f);  // 100 units range
            for (Creature* creature : creatures)
            {
                creature->DespawnOrUnsummon();
            }
        }

        void SetData(uint32 /*type*/, uint32 data) override
        {
            switch (data)
            {
                case 1:
                    events.ScheduleEvent(EVENT_PRE_FIGHT_1, 2s);
                    instance->SetBossState(DATA_PYROGAURD_EMBERSEER, IN_PROGRESS);
                    break;
                case 2:
                    // Close these two doors on Blackhand Incarcerators aggro
                    if (GameObject* door1 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_IN)))
                        if (door1->GetGoState() == GO_STATE_ACTIVE)
                            door1->SetGoState(GO_STATE_READY);
                    if (GameObject* door2 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_DOORS)))
                        if (door2->GetGoState() == GO_STATE_ACTIVE)
                            door2->SetGoState(GO_STATE_READY);
                    break;
                case 3:
                    Reset();
                    break;
                default:
                    break;
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            // ### TODO Check combat timing ###
            ApplyAuraToMobs(819753);
            events.ScheduleEvent(EVENT_FIRENOVA,    7s);
            events.ScheduleEvent(EVENT_FLAMEBUFFET, 5s);
            events.ScheduleEvent(EVENT_PYROBLAST,  14s);
            events.ScheduleEvent(EVENT_BLAST_WAVE, 20s); 
            events.ScheduleEvent(EVENT_CHECK_MELEE, 1s);
            events.ScheduleEvent(EVENT_SPAWN_STABILIZER, 10s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            RemoveAuraFromMobs(819753);
            DespawnCreatures(810316);
            // Activate all the runes
            UpdateRunes(GO_STATE_READY);
            // Opens all 3 doors
            OpenDoors(true);
            // Complete encounter
            instance->SetBossState(DATA_PYROGAURD_EMBERSEER, DONE);
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

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_ENCAGE_EMBERSEER)
            {
                if (!me->GetAuraCount(SPELL_ENCAGED_EMBERSEER))
                    me->CastSpell(me, SPELL_ENCAGED_EMBERSEER);
            }

            if (spell->Id == SPELL_EMBERSEER_GROWING_TRIGGER)
            {
                if (me->GetAuraCount(SPELL_EMBERSEER_GROWING_TRIGGER) == 10)
                    Talk(EMOTE_TEN_STACK);

                if (me->GetAuraCount(SPELL_EMBERSEER_GROWING_TRIGGER) == 20)
                {
                    events.CancelEvent(EVENT_FIRE_SHIELD); // temporarily cancel fire shield to keep it from interrupting combat start

                    // Schedule out the pre-combat scene
                    events.ScheduleEvent(EVENT_PRE_ENTER_COMBAT_1, 0s);
                    events.ScheduleEvent(EVENT_PRE_ENTER_COMBAT_2, 2s);
                    events.ScheduleEvent(EVENT_ENTER_COMBAT, 5s);
                }
            }
        }

        void OpenDoors(bool Boss_Killed)
        {
            // These two doors reopen on reset or boss kill
            if (GameObject* door1 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_IN)))
                door1->SetGoState(GO_STATE_ACTIVE);
            if (GameObject* door2 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_DOORS)))
                door2->SetGoState(GO_STATE_ACTIVE);

            // This door opens on boss kill
            if (Boss_Killed)
                if (GameObject* door3 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_OUT)))
                    door3->SetGoState(GO_STATE_ACTIVE);
        }

        void UpdateRunes(GOState state)
        {
            // update all runes
            if (GameObject* rune1 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_1)))
                rune1->SetGoState(state);
            if (GameObject* rune2 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_2)))
                rune2->SetGoState(state);
            if (GameObject* rune3 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_3)))
                rune3->SetGoState(state);
            if (GameObject* rune4 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_4)))
                rune4->SetGoState(state);
            if (GameObject* rune5 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_5)))
                rune5->SetGoState(state);
            if (GameObject* rune6 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_6)))
                rune6->SetGoState(state);
            if (GameObject* rune7 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_EMBERSEER_RUNE_7)))
                rune7->SetGoState(state);
        }

        void ApplyAuraToMobs(uint32 spellId)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, 9819, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 10899, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 9818, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 9817, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 10083, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 10317, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 9096, 60.0f);
            for (Creature* creature : creatures)
            {
                creature->AddAura(spellId, creature);
            }
        }

        void RemoveAuraFromMobs(uint32 spellId)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, 9819, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 10899, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 9818, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 9817, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 10083, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 10317, 60.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 9096, 60.0f);
            for (Creature* creature : creatures)
            {
                creature->RemoveAurasDueToSpell(spellId);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_RESPAWN:
                            {
                                // Respawn all Blackhand Incarcerators
                                std::list<Creature*> creatureList;
                                GetCreatureListWithEntryInGrid(creatureList, me, NPC_BLACKHAND_INCARCERATOR, 35.0f);
                                for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
                                    if (Creature* creature = *itr)
                                    {
                                        if (!creature->IsAlive())
                                            creature->Respawn();

                                        creature->AI()->SetData(1, 2);
                                    }
                                me->AddAura(SPELL_ENCAGED_EMBERSEER, me);
                                instance->SetBossState(DATA_PYROGAURD_EMBERSEER, NOT_STARTED);
                                break;
                            }
                        case EVENT_PRE_FIGHT_1:
                            {
                                // Set data on all Blackhand Incarcerators
                                std::list<Creature*> creatureList;
                                GetCreatureListWithEntryInGrid(creatureList, me, NPC_BLACKHAND_INCARCERATOR, 35.0f);
                                for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
                                {
                                    if (Creature* creature = *itr)
                                        creature->AI()->SetData(1, 1);
                                }
                                events.ScheduleEvent(EVENT_PRE_FIGHT_2, 2s);
                                break;
                            }
                        case EVENT_PRE_FIGHT_2:
                            me->CastSpell(me, SPELL_FREEZE_ANIM);
                            me->CastSpell(me, SPELL_EMBERSEER_GROWING);
                            Talk(EMOTE_ONE_STACK);
                            break;
                        case EVENT_FIRE_SHIELD:
                            // #### Spell isn't doing any damage ??? ####
                            DoCast(me, SPELL_FIRE_SHIELD);
                            events.ScheduleEvent(EVENT_FIRE_SHIELD, 3s);
                            break;
                        case EVENT_PRE_ENTER_COMBAT_1:
                            me->RemoveAura(SPELL_ENCAGED_EMBERSEER);
                            me->RemoveAura(SPELL_FREEZE_ANIM);
                            me->CastSpell(me, SPELL_EMBERSEER_FULL_STRENGTH);
                            Talk(EMOTE_FREE_OF_BONDS);
                            break;
                        case EVENT_PRE_ENTER_COMBAT_2:
                            Talk(YELL_FREE_OF_BONDS);
                            break;
                        case EVENT_ENTER_COMBAT:
                            me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                            me->SetImmuneToPC(false);
                            DoZoneInCombat();
                            // re-enable fire shield
                            events.ScheduleEvent(EVENT_FIRE_SHIELD, 0s);
                            break;
                        default:
                            break;
                    }
                }
                return;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FIRE_SHIELD:
                        DoCast(me, SPELL_FIRE_SHIELD);
                        events.ScheduleEvent(EVENT_FIRE_SHIELD, 3s);
                        break;
                    case EVENT_FIRENOVA:
                        DoCast(me, SPELL_FIRENOVA);
                        events.ScheduleEvent(EVENT_FIRENOVA, 6s);
                        break;
                    case EVENT_FLAMEBUFFET:
                        DoCast(me, SPELL_FLAMEBUFFET);
                        events.ScheduleEvent(EVENT_FLAMEBUFFET, 14s);
                        break;
                    case EVENT_PYROBLAST:
                        DoCastRandomTarget(SPELL_PYROBLAST, 0, 100.0f);
                        events.ScheduleEvent(EVENT_PYROBLAST, 15s);
                        break;
                    case EVENT_BLAST_WAVE:
                        DoCast(me, SPELL_BLAST_WAVE, true);
                        events.ScheduleEvent(EVENT_BLAST_WAVE, 30s); 
                        break;
                    case EVENT_CHECK_MELEE:
                    {
                        Unit* victim = me->GetVictim();
                        bool hasMelee = victim && me->IsWithinMeleeRange(victim);

                        if (!hasMelee)
                        {
                            switch (urand(0, 2))  
                            {
                            case 0:
                                me->Yell("Keep close or burn!", LANG_UNIVERSAL);
                                break;
                            case 1:
                                me->Yell("My flames will find you!", LANG_UNIVERSAL);
                                break;
                            case 2:
                                me->Yell("Too far! The fire consumes you!", LANG_UNIVERSAL);
                                break;
                            }

                            DoCast(me, SPELL_FIREBALL_VOLLEY, true);
                        }

                        events.ScheduleEvent(EVENT_CHECK_MELEE, 5s);
                        break;
                    }
                    case EVENT_SPAWN_STABILIZER:
                    {
                        std::vector<Position> runePositions = {
                            { 126.28002166748f, -276.84283447266f, 91.553497314453f, 1.5628784894943f },
                            { 126.07306671143f, -258.66790771484f, 91.553512573242f, 1.5628784894943f },
                            { 126.21448516846f, -240.80790710449f, 91.538856506348f, 1.5628784894943f },
                            { 144.01034545898f, -240.55877685547f, 91.538055419922f, 6.1873021125793f },
                            { 162.36396789551f, -240.90676879883f, 91.547370910645f, 6.2261791229248f },
                            { 162.49409484863f, -258.45318603516f, 91.536605834961f, 4.7384777069092f },
                            { 162.78369140625f, -276.68872070312f, 91.613441467285f, 4.5276770591736f }
                        };

                        if (!runePositions.empty())
                        {
                            // Randomly select a rune position
                            Position const& spawnPos = runePositions[urand(0, runePositions.size() - 1)];

                            // Spawn the stabilizer
                            if (Creature* stabilizer = me->SummonCreature(810316, spawnPos, TEMPSUMMON_TIMED_DESPAWN, 300000)) 
                            {
                                stabilizer->SetHomePosition(spawnPos);
                                stabilizer->SetInCombatWithZone(); 
                            }
                        }

                        events.ScheduleEvent(EVENT_SPAWN_STABILIZER, 16s); 
                        break;
                    }
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockSpireAI<boss_pyroguard_emberseerAI>(creature);
    }
};

/*####
## npc_blackhand_incarcerator
####*/

enum IncarceratorData
{
    // OOC
    EVENT_ENCAGED_EMBERSEER         = 1,
    // Combat
    EVENT_STRIKE                    = 2,
    EVENT_ENCAGE                    = 3,

    EMOTE_FLEE                      = 0
};

class npc_blackhand_incarcerator : public CreatureScript
{
public:
    npc_blackhand_incarcerator() : CreatureScript("npc_blackhand_incarcerator") { }

    struct npc_blackhand_incarceratorAI : public ScriptedAI
    {
        npc_blackhand_incarceratorAI(Creature* creature) : ScriptedAI(creature)
        {
            _fleedForAssistance = false;
        }

        void Reset() override
        {
            me->SetImmuneToAll(true);
            if (Creature* Emberseer = me->FindNearestCreature(NPC_PYROGAURD_EMBERSEER, 30.0f, true))
                Emberseer->AI()->SetData(1, 3);

            _fleedForAssistance = false;
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*school*/) override
        {
            if (!_fleedForAssistance && me->HealthBelowPctDamaged(30, damage))
            {
                _fleedForAssistance = true;
                me->DoFleeToGetAssistance();
                Talk(EMOTE_FLEE);
            }
        }

        void SetData(uint32 data, uint32 value) override
        {
            if (data == 1 && value == 1)
            {
                me->SetImmuneToAll(false);
                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                DoZoneInCombat();
                _events.CancelEvent(EVENT_ENCAGED_EMBERSEER);
            }

            if (data == 1 && value == 2)
                _events.ScheduleEvent(EVENT_ENCAGED_EMBERSEER, 1s);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            // Used to close doors
            if (Creature* Emberseer = me->FindNearestCreature(NPC_PYROGAURD_EMBERSEER, 30.0f, true))
                Emberseer->AI()->SetData(1, 2);

            // Had to do this because CallForHelp will ignore any npcs without LOS
            std::list<Creature*> creatureList;
            GetCreatureListWithEntryInGrid(creatureList, me, NPC_BLACKHAND_INCARCERATOR, 60.0f);
            for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
            {
                if (Creature* creature = *itr)
                    creature->SetInCombatWithZone();    // AI()->AttackStart(me->GetVictim());
            }

            _events.ScheduleEvent(EVENT_STRIKE, 8s, 16s);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ENCAGED_EMBERSEER:
                            {
                                if (me->GetPositionX() == me->GetHomePosition().GetPositionX())
                                    if (!me->HasAura(SPELL_ENCAGE_EMBERSEER))
                                        if (Creature* Emberseer = me->FindNearestCreature(NPC_PYROGAURD_EMBERSEER, 30.0f, true))
                                            DoCast(Emberseer, SPELL_ENCAGE_EMBERSEER);
                                break;
                            }
                    }
                }
                return;
            }

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_STRIKE:
                        DoCastVictim(SPELL_STRIKE, true);
                        _events.ScheduleEvent(EVENT_STRIKE, 14s, 23s);
                        break;
                    case EVENT_ENCAGE:
                        DoCast(SelectTarget(SelectTargetMethod::Random, 0, 100, true), EVENT_ENCAGE, true);
                        _events.ScheduleEvent(EVENT_ENCAGE, 6s, 12s);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _fleedForAssistance;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockSpireAI<npc_blackhand_incarceratorAI>(creature);
    }
};

class npc_blackhand_stabilizer : public CreatureScript
{
public:
    npc_blackhand_stabilizer() : CreatureScript("npc_blackhand_stabilizer") {}

    struct npc_blackhand_stabilizerAI : public ScriptedAI
    {
        npc_blackhand_stabilizerAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            if (Creature* emberseer = GetEmberseer())
            {
                me->CastSpell(emberseer, 835207, false);  
            }
        }

        Creature* GetEmberseer()
        {
            return me->FindNearestCreature(NPC_PYROGAURD_EMBERSEER, 100.0f);  
        }

        void UpdateAI(uint32 diff) override
        {
            if (!me->IsNonMeleeSpellCast(false))
            {
                if (Creature* emberseer = GetEmberseer())
                {
                    me->CastSpell(emberseer, 835207, false); 
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_blackhand_stabilizerAI(creature);
    }
};

void AddSC_boss_pyroguard_emberseer()
{
    new npc_blackhand_stabilizer();
    new boss_pyroguard_emberseer();
    new npc_blackhand_incarcerator();
}
