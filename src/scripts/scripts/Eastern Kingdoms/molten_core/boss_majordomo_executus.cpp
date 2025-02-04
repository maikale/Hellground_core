/* 
 * Copyright (C) 2006-2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2008-2015 Hellground <http://hellground.net/>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: Boss_Majordomo_Executus
SD%Complete: 30
SDComment: Correct spawning and Event NYI
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

#define SAY_AGGRO           -1409003
#define SAY_SPAWN           -1409004
#define SAY_SLAY            -1409005
#define SAY_SPECIAL         -1409006
#define SAY_DEFEAT          -1409007

#define SAY_SUMMON_MAJ      -1409008
#define SAY_ARRIVAL1_RAG    -1409009
#define SAY_ARRIVAL2_MAJ    -1409010
#define SAY_ARRIVAL3_RAG    -1409011
#define SAY_ARRIVAL5_RAG    -1409012

#define SPAWN_RAG_X         838.51
#define SPAWN_RAG_Y         -829.84
#define SPAWN_RAG_Z         -232.00
#define SPAWN_RAG_O         1.70

#define SPELL_MAGIC_REFLECTION      20619
#define SPELL_DAMAGE_REFLECTION     21075

#define SPELL_BLASTWAVE             20229
#define SPELL_AEGIS                 20620                   //This is self cast whenever we are below 50%
#define SPELL_TELEPORT              20618
#define SPELL_SUMMON_RAGNAROS       19774
#define SPELL_SHADOW_BOLT           21077
#define SPELL_SHADOW_SHOCK          20603
#define SPELL_FIRE_BLAST            20623
#define SPELL_FIREBALL              20420

#define SPELL_TELEPORT_VISUAL       19484
#define ENTRY_FLAMEWALKER_HEALER    11663
#define ENTRY_FLAMEWALKER_ELITE     11664

#define CACHE_OF_THE_FIRELORD       179703

static float AddLocations[8][4] =
{
    { 753.044f, -1186.86f, -118.333f, 2.54516f },
    { 755.028f, -1172.57f, -118.636f, 3.3227f  },
    { 748.181f, -1161.33f, -118.807f, 3.99422f },
    { 742.781f, -1197.79f, -118.008f, 1.91291f },
    { 752.798f, -1166.29f, -118.766f, 3.63844f },
    { 743.136f, -1157.67f, -119.021f, 4.16779f },
    { 748.553f, -1193.14f, -118.099f, 2.22158f },
    { 736.539f, -1199.47f, -118.334f, 1.69143f }
};

struct boss_majordomoAI : public BossAI
{
    boss_majordomoAI(Creature *c) : BossAI(c, 1)
    {
        pInstance = (c->GetInstanceData());
    }

    ScriptedInstance* pInstance;

    int32 MagicReflection_Timer;
    int32 DamageReflection_Timer;
    int32 Blastwave_Timer;
    int32 TeleportVisual_Timer;
    int32 Teleport_Timer;
    bool Teleport_Use;
    int32 SummonRag_Timer;
    uint64 AddGUID[8];

    void Reset()
    {
        MagicReflection_Timer = 45000;                     //Damage reflection first so we alternate
        DamageReflection_Timer = 15000;
        Blastwave_Timer = 10000;
        TeleportVisual_Timer = 30000;
        Teleport_Timer = 20000;

        SummonRag_Timer = 20000;

        ClearCastQueue();

        if (pInstance && pInstance->GetData(DATA_MAJORDOMO_EXECUTUS_EVENT) < DONE)
        {
            pInstance->SetData(DATA_MAJORDOMO_EXECUTUS_EVENT, NOT_STARTED);
            me->SetVisibility(VISIBILITY_OFF);
            SpawnAdds();
        }

        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);
    }

    void KilledUnit(Unit* victim)
    {
        if (rand() % 5)
            return;

        DoScriptText(SAY_SLAY, m_creature);
    }

    void EnterCombat(Unit *who)
    {
        if (me->GetVisibility() == VISIBILITY_OFF || me->GetReactState() == REACT_PASSIVE)
            return;

        DoZoneInCombat();
        for (uint8 i = 0; i < 8; ++i)
        {
            Creature* Temp = Unit::GetCreature((*m_creature), AddGUID[i]);
            if (Temp && Temp->IsAlive())
            {
                Temp->SetReactState(REACT_AGGRESSIVE);
                Temp->AI()->AttackStart(m_creature->GetVictim());
            }
            else
            {
                EnterEvadeMode();
                break;
            }
        }
        DoAction(2);
    }

    void AttackStart(Unit *who)
    {
        if (me->GetVisibility() == VISIBILITY_ON || me->GetReactState() == REACT_AGGRESSIVE)
            BossAI::AttackStart(who);
    }

    void DoAction(const int32 action)
    {
        switch (action)
        {
            case 1:
            {
                DoScriptText(SAY_SPAWN, m_creature);
                me->SetVisibility(VISIBILITY_ON);
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);

                for (uint8 i = 0; i < 8; ++i)
                {
                    if (Creature* add = Unit::GetCreature((*m_creature), AddGUID[i]))
                    {
                        add->SetVisibility(VISIBILITY_ON);
                    }
                }
                break;
            }
            case 2:
            {
                pInstance->SetData(DATA_MAJORDOMO_EXECUTUS_EVENT, IN_PROGRESS);
                DoScriptText(SAY_AGGRO, m_creature);
                break;
            }
            case 3:
            {
                pInstance->SetData(DATA_MAJORDOMO_EXECUTUS_EVENT, DONE);
                EnterEvadeMode();
                DoScriptText(SAY_DEFEAT, m_creature);

                break;
            }
            case 4:
            {
                me->CastSpell(me, SPELL_TELEPORT_VISUAL, false);
                me->SetPosition(847.636, -814.667725, -229.79, 1.7, true);
                pInstance->SetData(DATA_SUMMON_RAGNAROS, IN_PROGRESS);
                break;
            }
            case 5:
            {
                me->CastSpell(me, SPELL_SUMMON_RAGNAROS, false);
                DoScriptText(SAY_SUMMON_MAJ, m_creature);
                pInstance->SetData(DATA_SUMMON_RAGNAROS, DONE);
                break;
            }
            default:
                break;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (pInstance->GetData(DATA_RUNES) != RUNES_COMPLETE)
            return;

        if (pInstance->GetData(DATA_RUNES) == RUNES_COMPLETE && pInstance->GetData(DATA_MAJORDOMO_EXECUTUS_EVENT) == NOT_STARTED && me->GetVisibility() == VISIBILITY_OFF)
        {
            DoAction(1);
            return;
        }

        if (pInstance->GetData(DATA_MAJORDOMO_EXECUTUS_EVENT) == DONE && pInstance->GetData(DATA_SUMMON_RAGNAROS) == NOT_STARTED)
        {
            TeleportVisual_Timer -= diff;
            if (TeleportVisual_Timer <= diff)
            {
                DoAction(4);
                TeleportVisual_Timer += 1000000;
            }
            return;
        }

        if (pInstance->GetData(DATA_MAJORDOMO_EXECUTUS_EVENT) == DONE && pInstance->GetData(DATA_SUMMON_RAGNAROS) == IN_PROGRESS)
        {
            SummonRag_Timer -= diff;
            if (SummonRag_Timer <= diff)
            {
                DoAction(5);
                SummonRag_Timer += 1000000;
            }
            return;
        }

        if (!UpdateVictim())
            return;

        if (pInstance->GetData(DATA_MAJORDOMO_EXECUTUS_EVENT) == IN_PROGRESS)
        {
            if (AddsKilled())
            {
                me->GetVictim()->SummonGameObject(CACHE_OF_THE_FIRELORD, 752.492, -1188.51, -118.296, 2.4288, 0, 0, 0.93716, 0.348899, 0);
                DoAction(3);
                return;
            }
        }

        //Cast Ageis if less than 50% hp
        if (m_creature->GetHealth() * 100 / m_creature->GetMaxHealth() < 50)
        {
            DoCast(m_creature, SPELL_AEGIS);
        }

        Teleport_Timer -= diff;
        if (Teleport_Timer <= diff)
        {
            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, GetSpellMaxRange(SPELL_TELEPORT), true))
            {
                ForceSpellCast(target, SPELL_TELEPORT, DONT_INTERRUPT, false, true);
                target->SetPosition(736.767456, -1176.591797, -118.948753, 0, true);
                ((Player*) target)->TeleportTo(me->GetMapId(), 736.767456, -1176.591797, -118.948753, 0);
                Teleport_Timer += 20000;
            }
        }

        MagicReflection_Timer -= diff;
        if (MagicReflection_Timer <= diff)
        {
            AddSpellToCast(m_creature, SPELL_MAGIC_REFLECTION, false);
            MagicReflection_Timer += 30000;
        }

        DamageReflection_Timer -= diff;
        if (DamageReflection_Timer <= diff)
        {
            AddSpellToCast(m_creature, SPELL_DAMAGE_REFLECTION, false);
            DamageReflection_Timer += 30000;
        }

        Blastwave_Timer -= diff;
        if (Blastwave_Timer <= diff)
        {
            AddSpellToCast(m_creature, SPELL_BLASTWAVE, false);
            Blastwave_Timer += 10000;
        }

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }

    bool AddsKilled()
    {
        for (uint8 i = 0; i < 8; ++i)
        {
            Unit* add = Unit::GetUnit((*m_creature), AddGUID[i]);
            if (add && add->IsAlive())
                return false;
        }
        return true;
    }

    void SpawnAdds()
    {
        for (uint8 i = 0; i < 8; ++i)
        {
            Creature *pCreature = (Unit::GetCreature((*m_creature), AddGUID[i]));
            if (!pCreature || !pCreature->IsAlive())
            {
                if (pCreature)
                    pCreature->setDeathState(DEAD);
                pCreature = me->SummonCreature(i < 4 ? ENTRY_FLAMEWALKER_HEALER : ENTRY_FLAMEWALKER_ELITE, AddLocations[i][0], AddLocations[i][1], AddLocations[i][2], AddLocations[i][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                if (pCreature)
                {
                    AddGUID[i] = pCreature->GetGUID();
                }
            }
            else
            {
                pCreature->AI()->Reset();
            }
        }
    }
};

CreatureAI* GetAI_boss_majordomo(Creature *_Creature)
{
    return new boss_majordomoAI(_Creature);
}

struct MCflamewakerAI : public ScriptedAI
{
    MCflamewakerAI(Creature *c) : ScriptedAI(c)
    {
        me->SetVisibility(VISIBILITY_OFF);
        me->SetReactState(REACT_PASSIVE);
    }

    Unit *owner;

    void Reset()
    {
        me->SetVisibility(VISIBILITY_OFF);
        me->SetReactState(REACT_PASSIVE);
        me->AI()->EnterEvadeMode();
        ScriptedAI::Reset();
    }

    void EnterCombat(Unit *who)
    {
        me->AI()->AttackStart(who);
    }

    void AttackStart(Unit *who)
    {
        if (me->GetReactState() == REACT_AGGRESSIVE)
            ScriptedAI::AttackStart(who);
        else if (me->GetReactState() == REACT_PASSIVE && owner && who)
            ((Creature*) owner)->AI()->AttackStart(who);

    }

    void IsSummonedBy(Unit *summoner)
    {
        owner = summoner;
    }
};

CreatureAI* GetAI_MCflamewaker(Creature *_Creature)
{
    return new MCflamewakerAI(_Creature);
}

struct flamewaker_healerAI: public MCflamewakerAI
{
    flamewaker_healerAI(Creature *c) : MCflamewakerAI(c)
    {
    }

    int32 ShadownBolt_Timer;
    int32 ShadownShock_Timer;

    void Reset()
    {
        ShadownBolt_Timer = 1000;
        ShadownShock_Timer = 8000;
        MCflamewakerAI::Reset();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!me->GetVictim())
            return;

        ShadownBolt_Timer -= diff;
        if (ShadownBolt_Timer <= diff)
        {
            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, GetSpellMaxRange(SPELL_SHADOW_BOLT), true))
            {
                AddSpellToCast(target, SPELL_SHADOW_BOLT, false);
                ShadownBolt_Timer += 2000;
            }
        }

        ShadownShock_Timer -= diff;
        if (ShadownShock_Timer <= diff)
        {
            AddSpellToCast(m_creature, SPELL_SHADOW_SHOCK, false);
            ShadownShock_Timer += 9000;
        }

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_flamewaker_healer(Creature *_Creature)
{
    return new flamewaker_healerAI(_Creature);
}

struct flamewaker_eliteAI: public MCflamewakerAI
{
    flamewaker_eliteAI(Creature *c) : MCflamewakerAI(c)
    {
    }

    int32 BlastWave_Timer;
    int32 FireBlast_Timer;
    int32 Fireball_Timer;

    void Reset()
    {
        BlastWave_Timer = 12000;
        FireBlast_Timer = 5000;
        Fireball_Timer = 1000;
        MCflamewakerAI::Reset();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!me->GetVictim())
            return;

        BlastWave_Timer -= diff;
        if (BlastWave_Timer <= diff)
        {
            AddSpellToCast(m_creature, SPELL_BLASTWAVE, false);
            BlastWave_Timer += 12000;
        }

        FireBlast_Timer -= diff;
        if (FireBlast_Timer <= diff)
        {
            AddSpellToCast(m_creature->GetVictim(), SPELL_FIRE_BLAST, false);
            FireBlast_Timer += 15000;
        }

        Fireball_Timer -= diff;
        if (Fireball_Timer <= diff)
        {
            AddSpellToCast(m_creature->GetVictim(), SPELL_FIREBALL, false);
            Fireball_Timer += 8000;
        }

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_flamewaker_elite(Creature *_Creature)
{
    return new flamewaker_eliteAI(_Creature);
}

void AddSC_boss_majordomo()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_majordomo";
    newscript->GetAI = &GetAI_boss_majordomo;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "flamewaker_healer";
    newscript->GetAI = &GetAI_flamewaker_healer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "flamewaker_elite";
    newscript->GetAI = &GetAI_flamewaker_elite;
    newscript->RegisterSelf();
}

