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
SDName: Boss_Mograine_And_Whitemane
SD%Complete: 75
SDComment: Event not implemented
SDCategory: Scarlet Monastery
EndScriptData */

#include "precompiled.h"
#include "def_scarlet_monastery.h"

#define SAY_MO_AGGRO                -1189005
#define SAY_MO_KILL                 -1189006
#define SAY_MO_RESSURECTED          -1189007

#define SAY_WH_INTRO                -1189008
#define SAY_WH_KILL                 -1189009
#define SAY_WH_RESSURECT            -1189010

#define SPELL_DIVINESHIELD2         1020
#define SPELL_CRUSADERSTRIKE5       35395
#define SPELL_HAMMEROFJUSTICE3      5589
#define SPELL_HOLYLIGHT6            3472
#define SPELL_CONSECRATION3         20922
#define SPELL_BLESSINGOFWISDOM      1044
#define SPELL_RETRIBUTIONAURA3      10299
#define SPELL_BLESSINGOFPROTECTION3 10278
#define SPELL_FLASHHEAL6            10916

struct boss_scarlet_commander_mograineAI : public ScriptedAI
{
    boss_scarlet_commander_mograineAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = (ScriptedInstance*)m_creature->GetInstanceData();
    }

    ScriptedInstance* pInstance;

    int32 Heal_Timer;
    int32 DivineShield2_Timer;
    int32 CrusaderStrike5_Timer;
    int32 HammerOfJustice3_Timer;
    int32 Consecration3_Timer;
    int32 BlessingOfWisdom_Timer;
    int32 BlessingOfProtection3_Timer;

    void Reset()
    {
        Heal_Timer = 80000;
        DivineShield2_Timer = 60000;
        CrusaderStrike5_Timer = 20000;
        HammerOfJustice3_Timer = 80000;
        Consecration3_Timer = 30000;
        BlessingOfWisdom_Timer = 45000;
        BlessingOfProtection3_Timer = 45000;

        if(pInstance)
            pInstance->HandleGameObject(pInstance->GetData64(DATA_DOOR_WHITEMANE), false);

    }

    void EnterCombat(Unit *who)
    {
        DoScriptText(SAY_MO_AGGRO, m_creature);
        DoCast(m_creature,SPELL_RETRIBUTIONAURA3);
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_MO_KILL, m_creature);
    }

    void JustDied(Unit *who)
    {
        if(!pInstance)
            return;

        pInstance->HandleGameObject(pInstance->GetData64(DATA_DOOR_WHITEMANE), true);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        //If we are <50% hp cast Arcane Bubble and start casting SPECIAL Arcane Explosion
        if (m_creature->GetHealth()*100 / m_creature->GetMaxHealth() <= 50 && !m_creature->IsNonMeleeSpellCast(false))
        {
            Heal_Timer -= diff;
            if (Heal_Timer <= diff)
            {
                DoCast(m_creature, RAND(SPELL_HOLYLIGHT6, SPELL_FLASHHEAL6));

                //60 seconds until we should cast this agian
                Heal_Timer += 60000;
            }
        }

        DivineShield2_Timer -= diff;
        if (DivineShield2_Timer <= diff)
        {
            DoCast(m_creature,SPELL_DIVINESHIELD2);
            DivineShield2_Timer += 60000;
        }
        

        CrusaderStrike5_Timer -= diff;
        if (CrusaderStrike5_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_CRUSADERSTRIKE5);
            CrusaderStrike5_Timer += 20000;
        }
        

        HammerOfJustice3_Timer -= diff;
        if (HammerOfJustice3_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_HAMMEROFJUSTICE3);
            HammerOfJustice3_Timer += 30000;
        }
        

        Consecration3_Timer -= diff;
        if (Consecration3_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_CONSECRATION3);
            Consecration3_Timer += 20000;
        }
        

        BlessingOfWisdom_Timer -= diff;
        if (BlessingOfWisdom_Timer <= diff)
        {
            DoCast(m_creature,SPELL_BLESSINGOFWISDOM);
            BlessingOfWisdom_Timer += 45000;
        }
        

        BlessingOfProtection3_Timer -= diff;
        if (BlessingOfProtection3_Timer <= diff)
        {
            DoCast(m_creature,SPELL_BLESSINGOFPROTECTION3);
            BlessingOfProtection3_Timer += 50000;
        }
        

        DoMeleeAttackIfReady();
    }
};

#define SPELL_DEEPSLEEP                 9256
#define SPELL_SCARLETRESURRECTION       9232

#define SPELL_CRUSADERSTRIKE            17281
#define SPELL_HAMMEROFJUSTICE           13005
#define SPELL_HOLYSMITE6                9481
#define SPELL_HOLYFIRE5                 15265
#define SPELL_MINDBLAST6                8106

#define SPELL_POWERWORDSHIELD           6065

#define SPELL_RENEW                     6078
#define SPELL_FLASHHEAL6                10916

struct boss_high_inquisitor_whitemaneAI : public ScriptedAI
{
    boss_high_inquisitor_whitemaneAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = (ScriptedInstance*)m_creature->GetInstanceData();
    }

    ScriptedInstance* pInstance;

    int32 Healing_Timer;
    int32 Renew_Timer;
    int32 PowerWordShield_Timer;
    int32 CrusaderStrike_Timer;
    int32 HammerOfJustice_Timer;
    int32 HolySmite6_Timer;
    int32 HolyFire5_Timer;
    int32 MindBlast6_Timer;

    void Reset()
    {
        Healing_Timer = 0;
        Renew_Timer= 0;
        PowerWordShield_Timer = 2000;
        CrusaderStrike_Timer = 12000;
        HammerOfJustice_Timer = 18000;
        HolySmite6_Timer = 10000;
        HolyFire5_Timer = 20000;
        MindBlast6_Timer = 6000;
    }

    void EnterCombat(Unit *who)
    {
        DoScriptText(SAY_WH_INTRO, m_creature);
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_WH_KILL, m_creature);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        /*
        //This is going to be a routine to make the resurrection event...
        if (m_creature->isAlive && m_creature->isAlive)
        {
        m_creature->Relocate(1163.113370,1398.856812,32.527786,3.171014);

        DoScriptText(SAY_WH_RESSURECT, m_creature);

        DoCast(m_creature->GetVictim(),SPELL_DEEPSLEEP);
        DoCast(m-creature->GetGUID(51117),SPELL_SCARLETRESURRECTION)
        }
        */

        //If we are <75% hp cast healing spells at self and Mograine
        if (m_creature->GetHealth()*100 / m_creature->GetMaxHealth() <= 75 )
        {
            Healing_Timer -= diff;
            if (Healing_Timer <= diff)
            {
                DoCast(m_creature,SPELL_FLASHHEAL6);
                return;

                //22-32 seconds until we should cast this agian
                Healing_Timer += 22000 + rand()%10000;
            }
        }

        if (m_creature->GetHealth()*100 / m_creature->GetMaxHealth() <= 30)
        {
            Renew_Timer -= diff;
            if (Renew_Timer <= diff)
            {
                DoCast(m_creature,SPELL_RENEW);
                Renew_Timer += 30000;
            }
        }

        PowerWordShield_Timer -= diff;
        if (PowerWordShield_Timer <= diff)
        {
            DoCast(m_creature,SPELL_POWERWORDSHIELD);
            PowerWordShield_Timer += 25000;
        }
        

        CrusaderStrike_Timer -= diff;
        if (CrusaderStrike_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_CRUSADERSTRIKE);
            CrusaderStrike_Timer += 15000;
        }
        

        HammerOfJustice_Timer -= diff;
        if (HammerOfJustice_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_HAMMEROFJUSTICE);
            HammerOfJustice_Timer += 12000;
        }
        

        HolySmite6_Timer -= diff;
        if (HolySmite6_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_HOLYSMITE6);
            HolySmite6_Timer += 10000;
        }
        

        HolyFire5_Timer -= diff;
        if (HolyFire5_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_HOLYFIRE5);
            HolyFire5_Timer += 15000;
        }
        

        MindBlast6_Timer -= diff;
        if (MindBlast6_Timer <= diff)
        {
            DoCast(m_creature->GetVictim(),SPELL_MINDBLAST6);
            MindBlast6_Timer += 8000;
        }
        

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_scarlet_commander_mograine(Creature *_Creature)
{
    return new boss_scarlet_commander_mograineAI (_Creature);
}

CreatureAI* GetAI_boss_high_inquisitor_whitemane(Creature *_Creature)
{
    return new boss_high_inquisitor_whitemaneAI (_Creature);
}

void AddSC_boss_mograine_and_whitemane()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_scarlet_commander_mograine";
    newscript->GetAI = &GetAI_boss_scarlet_commander_mograine;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_high_inquisitor_whitemane";
    newscript->GetAI = &GetAI_boss_high_inquisitor_whitemane;
    newscript->RegisterSelf();
}

