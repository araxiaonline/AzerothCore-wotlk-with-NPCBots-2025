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
#include "CreatureAI.h"
#include "EventMap.h"
#include "InstanceMapScript.h"
#include "InstanceScript.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "sunken_temple.h"
//Dinkle
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "../scripts/Custom/Timewalking/10Man.h"
#include <random>

enum EranikusSpells
{
    SPELL_SHADE_OF_ERANIKUS_PASSIVE_VISUAL = 12535,
    SPELL_THRASH = 8876,
    SPELL_POISONBOLT = 825099,
    SPELL_CRIPPLE = 812890,
    SPELL_ACID_BREATH = 12884,
    SPELL_FRENZY = 8269
};

enum EranikusEvents
{
    EVENT_POISONBOLT = 1,
    EVENT_CRIPPLE,
    EVENT_ACID_BREATH,
    EVENT_SPAWN_CREATURE
};
//end Dinkle

const char* SAY_AGGRO = "This evil cannot be allowed to enter this world! Come my children!";

class instance_sunken_temple : public InstanceMapScript
{
public:
    instance_sunken_temple() : InstanceMapScript("instance_sunken_temple", 109) { }

    struct instance_sunken_temple_InstanceMapScript : public InstanceScript
    {
        instance_sunken_temple_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
        }

        void Initialize() override
        {
            _statuePhase = 0;
            _defendersKilled = 0;
            memset(&_encounters, 0, sizeof(_encounters));
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_JAMMAL_AN_THE_PROPHET:
                    _jammalanGUID = creature->GetGUID();
                    break;
                case NPC_SHADE_OF_ERANIKUS:
                    _shadeOfEranikusGUID = creature->GetGUID();
                    creature->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                    break;
            }

            if (creature->IsAlive() && creature->GetSpawnId() && creature->GetCreatureType() == CREATURE_TYPE_DRAGONKIN && creature->GetEntry() != NPC_SHADE_OF_ERANIKUS)
                _dragonkinList.push_back(creature->GetGUID());
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->IsCreature() && unit->GetCreatureType() == CREATURE_TYPE_DRAGONKIN && unit->GetEntry() != NPC_SHADE_OF_ERANIKUS)
                _dragonkinList.remove(unit->GetGUID());
            if (unit->GetEntry() == NPC_JAMMAL_AN_THE_PROPHET)
            {
                if (Creature* cr = instance->GetCreature(_shadeOfEranikusGUID))
                    cr->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            }

        }

        void OnGameObjectCreate(GameObject* gameobject) override
        {
            switch (gameobject->GetEntry())
            {
                case GO_ATALAI_STATUE1:
                case GO_ATALAI_STATUE2:
                case GO_ATALAI_STATUE3:
                case GO_ATALAI_STATUE4:
                case GO_ATALAI_STATUE5:
                case GO_ATALAI_STATUE6:
                    if (gameobject->GetEntry() < GO_ATALAI_STATUE1 + _statuePhase)
                    {
                        instance->SummonGameObject(GO_ATALAI_LIGHT2, gameobject->GetPositionX(), gameobject->GetPositionY(), gameobject->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                        gameobject->ReplaceAllGameObjectFlags(GO_FLAG_NOT_SELECTABLE);
                    }
                    break;
                case GO_ATALAI_IDOL:
                    if (_statuePhase == MAX_STATUE_PHASE)
                        gameobject->SummonGameObject(GO_IDOL_OF_HAKKAR, -480.08f, 94.29f, -189.72f, 1.571f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
                    break;
                case GO_IDOL_OF_HAKKAR:
                    if (_encounters[TYPE_ATAL_ALARION] == DONE)
                        gameobject->RemoveGameObjectFlag(GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_FORCEFIELD:
                    _forcefieldGUID = gameobject->GetGUID();
                    if (_defendersKilled == DEFENDERS_COUNT)
                        gameobject->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_STATUES:
                    _events.ScheduleEvent(DATA_STATUES, 0ms);
                    break;
                case DATA_DEFENDER_KILLED:
                    ++_defendersKilled;
                    if (_defendersKilled == DEFENDERS_COUNT)
                    {
                        instance->LoadGrid(-425.89f, -86.07f);
                        if (Creature* jammal = instance->GetCreature(_jammalanGUID))
                            jammal->AI()->Talk(0);
                        if (GameObject* forcefield = instance->GetGameObject(_forcefieldGUID))
                            forcefield->SetGoState(GO_STATE_ACTIVE);
                    }
                    break;
                case DATA_ERANIKUS_FIGHT:
                    for (ObjectGuid const& guid : _dragonkinList)
                    {
                        if (Creature* creature = instance->GetCreature(guid))
                            if (instance->IsGridLoaded(creature->GetPositionX(), creature->GetPositionY()))
                                creature->SetInCombatWithZone();
                    }
                    break;
                case TYPE_ATAL_ALARION:
                case TYPE_JAMMAL_AN:
                case TYPE_HAKKAR_EVENT:
                    _encounters[type] = data;
                    break;
            }

            SaveToDB();
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_STATUES:
                    return _statuePhase;
                case DATA_DEFENDER_KILLED:
                    return _defendersKilled;
                case TYPE_ATAL_ALARION:
                case TYPE_JAMMAL_AN:
                case TYPE_HAKKAR_EVENT:
                    return _encounters[type];
            }

            return 0;
        }

        void Update(uint32 diff) override
        {
            _events.Update(diff);
            switch (_events.ExecuteEvent())
            {
                case DATA_STATUES:
                    ++_statuePhase;
                    if (_statuePhase == MAX_STATUE_PHASE)
                        instance->SummonGameObject(GO_IDOL_OF_HAKKAR, -480.08f, 94.29f, -189.72f, 1.571f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
                    break;
            }
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            data >> _encounters[0];
            data >> _encounters[1];
            data >> _encounters[2];
            data >> _statuePhase;
            data >> _defendersKilled;
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << _encounters[0] << ' '
                << _encounters[1] << ' '
                << _encounters[2] << ' '
                << _statuePhase << ' '
                << _defendersKilled;
        }

    private:
        uint32 _statuePhase;
        uint32 _defendersKilled;
        uint32 _encounters[MAX_ENCOUNTERS];

        ObjectGuid _forcefieldGUID;
        ObjectGuid _jammalanGUID;
        ObjectGuid _shadeOfEranikusGUID;
        GuidList _dragonkinList;
        EventMap _events;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_sunken_temple_InstanceMapScript(map);
    }
};

enum MalfurionMisc
{
    QUEST_ERANIKUS_TYRANT_OF_DREAMS   = 8733,
    QUEST_THE_CHARGE_OF_DRAGONFLIGHTS = 8555,
};

class at_malfurion_stormrage : public AreaTriggerScript
{
public:
    at_malfurion_stormrage() : AreaTriggerScript("at_malfurion_stormrage") { }

    bool OnTrigger(Player* player, const AreaTrigger* /*at*/) override
    {
        // Check if the player is in an instance, Malfurion is not already present within a 50.0f radius, and the player meets the quest requirements
        if (player->GetInstanceScript() && !player->FindNearestCreature(NPC_MALFURION_STORMRAGE, 50.0f) &&
            player->GetQuestStatus(QUEST_THE_CHARGE_OF_DRAGONFLIGHTS) == QUEST_STATUS_REWARDED &&
            player->GetQuestStatus(QUEST_ERANIKUS_TYRANT_OF_DREAMS) != QUEST_STATUS_REWARDED)
        {
            // Summon Malfurion near the player's position with a specific orientation and despawn timer
            player->SummonCreature(NPC_MALFURION_STORMRAGE, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), -1.52f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000);
        }
        return false;
    }
};

class spell_temple_of_atal_hakkar_hex_of_jammal_an_aura : public AuraScript
{
    PrepareAuraScript(spell_temple_of_atal_hakkar_hex_of_jammal_an_aura);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ HEX_OF_JAMMAL_AN, HEX_OF_JAMMAL_AN_CHARM });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            if (caster->IsAlive() && caster->IsInCombat())
            {
                caster->CastSpell(GetTarget(), HEX_OF_JAMMAL_AN, true);
                caster->CastSpell(GetTarget(), HEX_OF_JAMMAL_AN_CHARM, true);
            }
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_temple_of_atal_hakkar_hex_of_jammal_an_aura::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_temple_of_atal_hakkar_awaken_the_soulflayer : public SpellScript
{
    PrepareSpellScript(spell_temple_of_atal_hakkar_awaken_the_soulflayer);

    void HandleSendEvent(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        InstanceScript* instanceScript = GetCaster()->GetInstanceScript();
        Map* map = GetCaster()->FindMap();
        if (!map || !instanceScript || instanceScript->GetData(TYPE_HAKKAR_EVENT) != NOT_STARTED)
            return;

        Position pos = {-466.795f, 272.863f, -90.447f, 1.57f};
        if (TempSummon* summon = map->SummonCreature(NPC_SHADE_OF_HAKKAR, pos))
        {
            summon->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
            instanceScript->SetData(TYPE_HAKKAR_EVENT, IN_PROGRESS);
        }
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_temple_of_atal_hakkar_awaken_the_soulflayer::HandleSendEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
    }
};

class boss_shade_of_eranikus : public CreatureScript
{
public:
    boss_shade_of_eranikus() : CreatureScript("boss_shade_of_eranikus") { }

    struct boss_shade_of_eranikusAI : public ScriptedAI
    {
        boss_shade_of_eranikusAI(Creature* creature) : ScriptedAI(creature) { }

        ObjectGuid goGUID;
        
        void Reset() override
        {
            events.Reset();
            DoCast(me, SPELL_SHADE_OF_ERANIKUS_PASSIVE_VISUAL, true);
            DoCast(me, SPELL_THRASH, true);
            if (GameObject* go = ObjectAccessor::GetGameObject(*me, goGUID))
            {
                go->Delete();
            }
        }

        void JustEngagedWith(Unit* who) override
        {
            me->Yell(SAY_AGGRO, LANG_UNIVERSAL);
            me->PlayDirectSound(188037);

            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, 0, 500.0f);

            for (Creature* creature : creatures)
            {
                if (creature->GetCreatureType() == CREATURE_TYPE_DRAGONKIN)
                {
                    creature->SetInCombatWithZone();
                }
            }

            events.ScheduleEvent(EVENT_POISONBOLT, urand(8000, 10000));
            events.ScheduleEvent(EVENT_CRIPPLE, urand(7000, 14000));
            events.ScheduleEvent(EVENT_ACID_BREATH, urand(1000, 11000));
            events.ScheduleEvent(EVENT_SPAWN_CREATURE, 20000);
            if (InstanceScript* instance = me->GetInstanceScript())
            {
                instance->SetData(12, 1); 
            }
            if (GameObject* go = me->SummonGameObject(8149431, -660.602f, 29.3771f, -91.2086f, 1.55876f, 0, 0, 0, 0, 600, false, GO_SUMMON_TIMED_OR_CORPSE_DESPAWN))
            {
                goGUID = go->GetGUID(); 
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
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
            if (GameObject* go = ObjectAccessor::GetGameObject(*me, goGUID))
            {
                go->Delete();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->GetDistance(me->GetHomePosition()) > 55.0f)
            {
                EnterEvadeMode();
                return;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (me->HealthBelowPct(30) && !me->HasAura(SPELL_FRENZY))
            {
                DoCast(me, SPELL_FRENZY, true);
            }

            std::list<Creature*> spawnedCreatures;
            GetCreatureListWithEntryInGrid(spawnedCreatures, me, 885781, 50.0f); 
            spawnedCreatures.remove_if([](Creature* c) { return !c->IsAlive(); });

            bool hasAliveSpawnedCreatures = !spawnedCreatures.empty();

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_POISONBOLT:
                    if (!hasAliveSpawnedCreatures) // Cast only if no summoned creatures are alive
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, false))
                        {
                            float x, y, z;
                            target->GetPosition(x, y, z);
                            me->CastSpell(x, y, z, SPELL_POISONBOLT, false);
                        }
                        else
                        {
                            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, false))
                            {
                                float x, y, z;
                                target->GetPosition(x, y, z);
                                me->CastSpell(x, y, z, SPELL_POISONBOLT, false);
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_POISONBOLT, urand(10000, 12000));
                    break;
                case EVENT_CRIPPLE:
                    if (!hasAliveSpawnedCreatures) // Cast only if no summoned creatures are alive
                    {
                        DoCastRandomTarget(SPELL_CRIPPLE, 0, 65.0f, false, false);
                    }
                    events.ScheduleEvent(EVENT_CRIPPLE, urand(20000, 26000));
                    break;
                case EVENT_ACID_BREATH:
                    DoCastVictim(SPELL_ACID_BREATH);
                    events.ScheduleEvent(EVENT_ACID_BREATH, urand(8000, 18000));
                    break;
                case EVENT_SPAWN_CREATURE:
                    SpawnCreatures();
                    events.ScheduleEvent(EVENT_SPAWN_CREATURE, 25000);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
        void SpawnCreatures()
            {
            struct SpawnPosition
            {
                float x, y, z, o;
            };
                std::vector<SpawnPosition> firstHalfPositions = {
                {-696.76843261719f, -35.4430809021f, -90.834991455078f, 1.5376726388931f},
                {-696.49096679688f, -27.070844650269f, -90.834991455078f, 1.5376726388931f},
                {-696.59338378906f, -19.199893951416f, -90.834991455078f, 1.5376726388931f},
                {-696.55047607422f, -10.952785491943f, -90.834991455078f, 1.5376726388931f},
                {-696.30529785156f, -3.5540657043457f, -90.834991455078f, 1.5376726388931f},
                {-696.75152587891f, 4.7396087646484f, -90.834991455078f, 1.5376726388931f},
                {-700.93743896484f, 8.8061227798462f, -88.231636047363f, 2.4880049228668f},
                {-700.91870117188f, -2.6666479110718f, -88.231636047363f, 4.7731223106384f},
                {-700.26409912109f, -13.432153701782f, -88.231636047363f, 4.7731223106384f},
                {-698.43267822266f, -21.607284545898f, -88.231636047363f, 4.7083268165588f},
                {-700.96020507812f, -31.995334625244f, -88.231636047363f, 4.174412727356f},
                {-700.51342773438f, -41.26118850708f, -88.231636047363f, 4.6236605644226f},
                {-700.59185791016f, -51.074146270752f, -88.231636047363f, 4.745475769043f},
                {-695.57373046875f, -53.443225860596f, -88.231636047363f, 4.745475769043f},
                {-692.740234375f, -53.661350250244f, -88.231636047363f, 4.745475769043f},
                {-660.56414794922f, 27.834854125977f, -91.208618164062f, 4.6423172950745f},
                {-661.02185058594f, 20.865493774414f, -90.835662841797f, 4.6975126266479f},
                {-660.69134521484f, 12.353354454041f, -90.835662841797f, 4.6975126266479f},
                {-660.59790039062f, 4.9422035217285f, -90.835662841797f, 4.6975126266479f},
                {-660.47052001953f, -2.5695633888245f, -90.835662841797f, 4.6975126266479f},
                {-660.58483886719f, -10.253149032593f, -90.835662841797f, 4.6975126266479f},
                {-660.54992675781f, -18.029218673706f, -90.835662841797f, 4.6975126266479f},
                {-660.67651367188f, -26.537933349609f, -90.835662841797f, 4.6975126266479f},
                {-660.42114257812f, -33.78059387207f, -90.835662841797f, 4.6975126266479f},
                {-660.53759765625f, -41.60754776001f, -90.835662841797f, 4.6975126266479f},
                {-660.41424560547f, -49.879276275635f, -90.835662841797f, 4.6975126266479f},
                {-660.51135253906f, -56.404689788818f, -90.835662841797f, 4.6975126266479f},
                {-660.47863769531f, -60.492031097412f, -88.231689453125f, 4.6975126266479f},
                {-669.09930419922f, -60.868202209473f, -88.231597900391f, 1.6165715456009f},
                {-669.29516601562f, -56.593467712402f, -90.835136413574f, 1.6165715456009f},
                {-668.87713623047f, -49.435791015625f, -90.835136413574f, 1.6165715456009f},
                {-669.00183105469f, -42.457710266113f, -90.835136413574f, 1.6165715456009f},
                {-669.19512939453f, -34.053050994873f, -90.835136413574f, 1.5362253189087f},
                {-668.92944335938f, -26.370899200439f, -90.835136413574f, 1.5362253189087f},
                {-668.99353027344f, -18.179960250854f, -90.835136413574f, 1.5362253189087f},
                {-669.06707763672f, -10.524070739746f, -90.835136413574f, 1.5362253189087f},
                {-669.00634765625f, -2.6197609901428f, -90.835136413574f, 1.5362253189087f},
                {-669.09710693359f, 5.0039467811584f, -90.835136413574f, 1.5362253189087f},
                {-669.18450927734f, 12.439791679382f, -90.835136413574f, 1.5362253189087f},
                {-669.25665283203f, 20.602617263794f, -90.835136413574f, 1.5362253189087f},
                {-669.36279296875f, 27.526176452637f, -90.835136413574f, 1.5362253189087f},
                {-676.76782226562f, 27.52756690979f, -90.835571289062f, 4.7054529190063f},
                {-676.77258300781f, 20.466508865356f, -90.835418701172f, 4.690399646759f},
                {-676.77459716797f, 13.05162525177f, -90.835174560547f, 4.690399646759f},
                {-677.47094726562f, 4.7413792610168f, -90.835174560547f, 4.690399646759f},
                {-677.39831542969f, -2.4281213283539f, -90.835174560547f, 4.690399646759f},
                {-677.58020019531f, -10.692817687988f, -90.835060119629f, 4.690399646759f},
                {-677.25958251953f, -18.266895294189f, -90.835060119629f, 4.690399646759f},
                {-677.42956542969f, -25.995441436768f, -90.835060119629f, 4.690399646759f},
                {-677.41912841797f, -33.975955963135f, -90.835060119629f, 4.690399646759f},
                {-677.28717041016f, -41.666858673096f, -90.835060119629f, 4.690399646759f},
                {-677.15356445312f, -49.687767028809f, -90.835060119629f, 4.690399646759f},
                {-677.30334472656f, -56.495616912842f, -90.835060119629f, 4.690399646759f},
                {-677.23529052734f, -60.69495010376f, -88.231620788574f, 4.690399646759f},
                {-686.4384765625f, -60.89155960083f, -88.231620788574f, 1.5195178985596f},
                {-686.21954345703f, -56.625965118408f, -90.835060119629f, 1.5195178985596f},
                {-685.83782958984f, -49.188697814941f, -90.835060119629f, 1.5195178985596f},
                {-685.98516845703f, -41.772621154785f, -90.835060119629f, 1.5195178985596f},
                {-685.94647216797f, -33.93648147583f, -90.835060119629f, 1.5195178985596f},
                {-685.90936279297f, -25.958393096924f, -90.835060119629f, 1.5195178985596f},
                {-685.93322753906f, -18.130060195923f, -90.835060119629f, 1.5195178985596f},
                {-685.93133544922f, -10.318634986877f, -90.835060119629f, 1.5195178985596f},
                {-685.83392333984f, -2.5475759506226f, -90.835060119629f, 1.5195178985596f},
                {-685.92742919922f, 5.3042159080505f, -90.835060119629f, 1.5195178985596f},
                {-685.84729003906f, 16.417221069336f, -88.231597900391f, 1.5947859287262f},
                {-692.43890380859f, 16.256256103516f, -88.231651306152f, 2.7000229358673f},
                {-689.97302246094f, 12.418032646179f, -90.835083007812f, 4.6787033081055f},
                {-689.8828125f, 1.5049580335617f, -90.835083007812f, 4.6787033081055f},
                {-690.14318847656f, -6.2210717201233f, -90.835083007812f, 4.6787033081055f},
                {-690.41247558594f, -14.212601661682f, -90.835083007812f, 4.6787033081055f},
                {-690.19079589844f, -22.096633911133f, -90.835083007812f, 4.6787033081055f},
                {-690.17498779297f, -29.778810501099f, -90.835083007812f, 4.6787033081055f},
                {-690.01055908203f, -37.52108001709f, -90.835083007812f, 4.6787033081055f},
                {-690.28106689453f, -45.548007965088f, -90.835083007812f, 4.6787033081055f},
                {-690.10711669922f, -49.461036682129f, -90.83512878418f, 4.7740406990051f},
                {-690.09594726562f, -56.598430633545f, -90.835182189941f, 4.6340827941895f},
                {-693.23974609375f, -61.664993286133f, -88.23161315918f, 4.0068559646606f},
                {-693.29583740234f, -53.371940612793f, -88.23161315918f, 3.1835227012634f},
                {-700.63861083984f, -53.680004119873f, -88.23161315918f, 3.1835227012634f},
                {-696.76293945312f, -49.313858032227f, -90.834991455078f, 1.5376726388931f},
                {-696.54827880859f, -42.835552215576f, -90.834991455078f, 1.5376726388931f}
                };
                
                std::vector<SpawnPosition> secondHalfPositions = {
                {-659.83557128906f, 27.354383468628f, -90.835639953613f, 4.6723213195801f},
                {-659.93115234375f, 20.401960372925f, -90.835639953613f, 4.7275114059448f},
                {-660.4677734375f, 12.762273788452f, -90.835639953613f, 4.6873693466187f},
                {-660.54309082031f, 5.1508197784424f, -90.835639953613f, 4.6873693466187f},
                {-660.58459472656f, -2.5264000892639f, -90.835639953613f, 4.6873693466187f},
                {-660.35797119141f, -10.468818664551f, -90.835639953613f, 4.6873693466187f},
                {-660.55358886719f, -18.28533744812f, -90.835639953613f, 4.6873693466187f},
                {-660.31268310547f, -26.074724197388f, -90.835098266602f, 4.7366790771484f},
                {-660.40716552734f, -33.953876495361f, -90.835098266602f, 4.7366790771484f},
                {-660.48211669922f, -41.534675598145f, -90.835098266602f, 4.7366790771484f},
                {-660.33416748047f, -49.449787139893f, -90.835098266602f, 4.7366790771484f},
                {-660.16162109375f, -56.54940032959f, -90.835098266602f, 4.7366790771484f},
                {-660.1669921875f, -60.888927459717f, -88.231628417969f, 4.6513767242432f},
                {-652.10021972656f, -60.966007232666f, -88.231628417969f, 1.5726052522659f},
                {-652.10833740234f, -56.471714019775f, -90.835136413574f, 1.5726052522659f},
                {-652.12194824219f, -48.971542358398f, -90.835136413574f, 1.5726052522659f},
                {-652.13513183594f, -41.69274520874f, -90.835136413574f, 1.5726052522659f},
                {-652.14892578125f, -34.086315155029f, -90.835136413574f, 1.5726052522659f},
                {-652.16394042969f, -25.798048019409f, -90.835136413574f, 1.5726052522659f},
                {-652.17767333984f, -18.218179702759f, -90.835136413574f, 1.5726052522659f},
                {-652.19122314453f, -10.718006134033f, -90.835136413574f, 1.5726052522659f},
                {-652.20593261719f, -2.5811531543732f, -90.835136413574f, 1.5726052522659f},
                {-652.21978759766f, 5.0695552825928f, -90.835136413574f, 1.5726052522659f},
                {-652.2333984375f, 12.585459709167f, -90.835136413574f, 1.5726052522659f},
                {-652.24749755859f, 20.360137939453f, -90.835136413574f, 1.5726052522659f},
                {-652.25964355469f, 27.111845016479f, -90.835762023926f, 1.5726052522659f},
                {-644.40948486328f, 27.201616287231f, -90.835762023926f, 4.5929336547852f},
                {-642.09204101562f, 26.58667755127f, -88.231636047363f, 4.6420016288757f},
                {-641.85552978516f, 22.07472038269f, -88.231636047363f, 4.6620726585388f},
                {-642.13366699219f, 16.551578521729f, -88.231636047363f, 4.6620726585388f},
                {-644.29241943359f, 20.510919570923f, -90.835296630859f, 4.6620726585388f},
                {-644.38098144531f, 12.76630115509f, -90.835296630859f, 4.6620726585388f},
                {-643.73669433594f, 4.808708190918f, -90.835067749023f, 4.6620726585388f},
                {-643.45739746094f, -2.5234348773956f, -90.835067749023f, 4.6620726585388f},
                {-643.54846191406f, -10.36797618866f, -90.835067749023f, 4.6620726585388f},
                {-643.53405761719f, -18.422567367554f, -90.835067749023f, 4.6620726585388f},
                {-643.65533447266f, -26.046974182129f, -90.835067749023f, 4.6620726585388f},
                {-643.51770019531f, -34.02054977417f, -90.835098266602f, 4.6620726585388f},
                {-643.59185791016f, -41.702964782715f, -90.835098266602f, 4.6620726585388f},
                {-643.55206298828f, -49.626914978027f, -90.835098266602f, 4.6620726585388f},
                {-643.70489501953f, -56.397048950195f, -90.835098266602f, 4.6620726585388f},
                {-644.24108886719f, -60.896976470947f, -88.231643676758f, 4.6620726585388f},
                {-635.12213134766f, -60.753860473633f, -88.231643676758f, 1.5537371635437f},
                {-635.04809570312f, -56.41662979126f, -90.83512878418f, 1.5537371635437f},
                {-635.27716064453f, -49.30281829834f, -90.835083007812f, 1.5537371635437f},
                {-635.15020751953f, -41.859817504883f, -90.835083007812f, 1.5537371635437f},
                {-635.01391601562f, -33.871242523193f, -90.835083007812f, 1.5537371635437f},
                {-635.27697753906f, -26.165786743164f, -90.835083007812f, 1.5537371635437f},
                {-635.14147949219f, -18.222938537598f, -90.835060119629f, 1.5537371635437f},
                {-635.30694580078f, -10.330127716064f, -90.835060119629f, 1.5537371635437f},
                {-635.17541503906f, -2.6185986995697f, -90.835060119629f, 1.5537371635437f},
                {-635.04156494141f, 5.225790977478f, -90.835060119629f, 1.5537371635437f},
                {-634.92553710938f, 12.025442123413f, -90.835060119629f, 1.5537371635437f},
                {-634.51721191406f, 16.355348587036f, -88.231674194336f, 1.5537371635437f},
                {-628.38208007812f, 16.51545715332f, -88.231674194336f, 6.1479234695435f},
                {-628.26342773438f, 8.7201099395752f, -88.231674194336f, 4.7276086807251f},
                {-620.36859130859f, 9.3242502212524f, -88.231674194336f, 0.027158476412296f},
                {-620.51678466797f, 0.039264086633921f, -88.231628417969f, 5.8663439750671f},
                {-620.16967773438f, -5.157958984375f, -88.231628417969f, 4.6654696464539f},
                {-620.50805664062f, -12.364150047302f, -88.231628417969f, 4.6654696464539f},
                {-620.74548339844f, -16.752040863037f, -88.231628417969f, 4.6654696464539f},
                {-620.78784179688f, -28.977651596069f, -88.231674194336f, 0.20951411128044f},
                {-620.47570800781f, -34.937973022461f, -88.231674194336f, 6.0814647674561f},
                {-620.43383789062f, -41.352619171143f, -88.231674194336f, 6.0814647674561f},
                {-620.826171875f, -49.424766540527f, -88.231674194336f, 6.0814647674561f},
                {-620.19732666016f, -53.40075302124f, -88.231674194336f, 4.78555727005f},
                {-628.20172119141f, -53.309967041016f, -88.231674194336f, 3.1302511692047f},
                {-631.19641113281f, 12.306108474731f, -90.835067749023f, 4.6684727668762f},
                {-631.09283447266f, 1.3186172246933f, -90.835067749023f, 4.6684727668762f},
                {-631.06787109375f, -6.3850107192993f, -90.835067749023f, 4.6684727668762f},
                {-631.12066650391f, -14.003744125366f, -90.835067749023f, 4.6684727668762f},
                {-630.99267578125f, -21.931074142456f, -90.835067749023f, 4.6684727668762f},
                {-630.91143798828f, -29.965019226074f, -90.835067749023f, 4.6684727668762f},
                {-631.02227783203f, -37.853862762451f, -90.835067749023f, 4.6684727668762f},
                {-631.06060791016f, -45.785053253174f, -90.835067749023f, 4.6684727668762f},
                {-630.97534179688f, -56.619483947754f, -90.834754943848f, 4.8265733718872f},
                {-624.44458007812f, -49.303642272949f, -90.835121154785f, 1.5464376211166f},
                {-624.51715087891f, -45.720378875732f, -90.835121154785f, 1.5464376211166f},
                {-624.67803955078f, -37.672950744629f, -90.835121154785f, 1.5464376211166f},
                {-624.74249267578f, -29.77931022644f, -90.835121154785f, 1.5464376211166f},
                {-624.54827880859f, -21.92155456543f, -90.835121154785f, 1.6261533498764f},
                {-624.63317871094f, -14.148507118225f, -90.835121154785f, 1.6261533498764f},
                {-624.57116699219f, -6.145604133606f, -90.835121154785f, 1.6261533498764f},
                {-624.61444091797f, 1.7288213968277f, -90.835121154785f, 1.6060833930969f},
                { -624.72308349609f, 4.8065195083618f, -90.835121154785f, 1.6060833930969f}
                };

                std::vector<SpawnPosition> combinedPositions = firstHalfPositions;
                combinedPositions.insert(combinedPositions.end(), secondHalfPositions.begin(), secondHalfPositions.end());

                std::vector<SpawnPosition> randomSubset;

                // Generate a random number between 1 and 100 for weighted selection
                int randomValue = urand(1, 100);
                std::vector<SpawnPosition>* chosenPositions = nullptr;

                if (randomValue <= 25) // 25% chance for case 0
                {
                    chosenPositions = &firstHalfPositions;
                }
                else if (randomValue <= 50) // 25% chance for case 1
                {
                    chosenPositions = &secondHalfPositions;
                }
                else // 50% chance for case random selection
                {
                    std::sample(combinedPositions.begin(), combinedPositions.end(), std::back_inserter(randomSubset),
                        combinedPositions.size() / 3, std::mt19937{ std::random_device{}() });
                    chosenPositions = &randomSubset;
                }

                if (urand(0, 1))
                {
                    me->Yell("Witness the tendrils of my power ensnaring your reality!", LANG_UNIVERSAL);
                    me->PlayDirectSound(188039);
                }
                else
                {
                    me->Yell("Behold the offspring of my nightmare!", LANG_UNIVERSAL);
                    me->PlayDirectSound(188038);
                }

                if (chosenPositions != nullptr)
                {
                    for (const auto& pos : *chosenPositions)
                    {
                        me->SummonCreature(885781, pos.x, pos.y, pos.z, pos.o, TEMPSUMMON_TIMED_DESPAWN, 14000);
                    }
                }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_shade_of_eranikusAI(creature);
    }
};

class npc_nightmare_egg : public CreatureScript
{
public:
    npc_nightmare_egg() : CreatureScript("npc_nightmare_egg") { }

    struct npc_nightmare_eggAI : public ScriptedAI
    {
        npc_nightmare_eggAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            DoCast(me, 870853, false); 
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_nightmare_eggAI(creature);
    }
};

class CreatureGuidCheck : public WorldScript
{
public:
    CreatureGuidCheck() : WorldScript("CreatureGuidCheck") { }

    void OnStartup() override
    {
        uint32 creatureGuid = 3118732;

        QueryResult result = WorldDatabase.Query("SELECT guid FROM creature WHERE guid = {}", creatureGuid);

        if (!result)
        {
            World::StopNow(0); 
            return;
        }
    }
};

void AddSC_instance_sunken_temple()
{
    new instance_sunken_temple();
    new at_malfurion_stormrage();
    new boss_shade_of_eranikus();
    new npc_nightmare_egg();
    new CreatureGuidCheck();
    RegisterSpellScript(spell_temple_of_atal_hakkar_hex_of_jammal_an_aura);
    RegisterSpellScript(spell_temple_of_atal_hakkar_awaken_the_soulflayer);
}
