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
SDName: Boss_Kruul
SD%Complete: 100
SDComment: Highlord Kruul are presumably no longer in-game on regular bases, however future events could bring him back.
SDCategory: Bosses
EndScriptData */

#include "precompiled.h"

#define SPELL_SHADOWVOLLEY          21341
#define SPELL_CLEAVE                20677
#define SPELL_THUNDERCLAP           23931
#define SPELL_TWISTEDREFLECTION     21063
#define SPELL_VOIDBOLT              21066
#define SPELL_RAGE                  21340
#define SPELL_CAPTURESOUL           21054

struct boss_kruulAI : public ScriptedAI
{
    boss_kruulAI(Creature *c) : ScriptedAI(c) {}

    int32 ShadowVolley_Timer;
    int32 Cleave_Timer;
    int32 ThunderClap_Timer;
    int32 TwistedReflection_Timer;
    int32 VoidBolt_Timer;
    int32 Rage_Timer;
    int32 Hound_Timer;
    int Rand;
    int RandX;
    int RandY;
    Creature* Summoned;

    void Reset()
    {
        ShadowVolley_Timer = 10000;
        Cleave_Timer = 14000;
        ThunderClap_Timer = 20000;
        TwistedReflection_Timer = 25000;
        VoidBolt_Timer = 30000;
        Rage_Timer = 60000;                                 //Cast rage after 1 minute
        Hound_Timer = 8000;
    }

    void EnterCombat(Unit *who)
    {
    }

    void KilledUnit()
    {
        // When a player, pet or totem gets killed, Lord Kazzak casts this spell to instantly regenerate 70,000 health.
        DoCast(m_creature,SPELL_CAPTURESOUL);

    }

    void SummonHounds(Unit* victim)
    {
        Rand = rand()%10;
        switch (rand()%2)
        {
            case 0: RandX = 0 - Rand; break;
            case 1: RandX = 0 + Rand; break;
        }
        Rand = 0;
        Rand = rand()%10;
        switch (rand()%2)
        {
            case 0: RandY = 0 - Rand; break;
            case 1: RandY = 0 + Rand; break;
        }
        Rand = 0;
        Summoned = DoSpawnCreature(19207, RandX, RandY, 0, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 300000);
        if(Summoned)
            ((CreatureAI*)Summoned->AI())->AttackStart(victim);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!UpdateVictim() )
            return;

        ShadowVolley_Timer -= diff;
        if (ShadowVolley_Timer <= diff)
        {
            if (rand()%100 < 46)
            {
                DoCast(m_creature->GetVictim(),SPELL_SHADOWVOLLEY);
            }

            ShadowVolley_Timer += 5000;
        }

        Cleave_Timer -= diff;
        if (Cleave_Timer <= diff)
        {
            if (rand()%100 < 50)
            {
                DoCast(m_creature->GetVictim(),SPELL_CLEAVE);
            }

            Cleave_Timer += 10000;
        }

        ThunderClap_Timer -= diff;
        if (ThunderClap_Timer <= diff)
        {
            if (rand()%100 < 20)
            {
                DoCast(m_creature->GetVictim(),SPELL_THUNDERCLAP);
            }

            ThunderClap_Timer += 12000;
        }

        TwistedReflection_Timer -= diff;
        if (TwistedReflection_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_TWISTEDREFLECTION);
            TwistedReflection_Timer += 30000;
        }

        VoidBolt_Timer -= diff;
        if (VoidBolt_Timer <= diff)
        {
            if (rand()%100 < 40)
            {
                DoCast(m_creature->GetVictim(),SPELL_VOIDBOLT);
            }

            VoidBolt_Timer += 18000;
        }

        Rage_Timer -= diff;
        if (Rage_Timer <= diff)
        {
            DoCast(m_creature,SPELL_RAGE);
            Rage_Timer += 70000;
        }

        Hound_Timer -= diff;
        if (Hound_Timer <= diff)
        {
            SummonHounds(m_creature->GetVictim());
            SummonHounds(m_creature->GetVictim());
            SummonHounds(m_creature->GetVictim());

            Hound_Timer += 45000;
        }

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_kruul(Creature *_Creature)
{
    return new boss_kruulAI (_Creature);
}

void AddSC_boss_kruul()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_kruul";
    newscript->GetAI = &GetAI_boss_kruul;
    newscript->RegisterSelf();
}

