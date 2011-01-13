/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef TRINITYCORE_PROGRESSBAR_H
#define TRINITYCORE_PROGRESSBAR_H

#include <stdio.h>
#include "Platform/Define.h"

class TRINITY_DLL_SPEC barGoLink
{
    public:
        barGoLink(int);
        ~barGoLink();

        void step();
        static void SetOutputState(bool on) { m_showOutput = on; }

    private:
        static char const * const empty;
        static char const * const full;

        static bool m_showOutput;

        int rec_no;
        int rec_pos;
        int num_rec;
        int indic_len;
};
#endif
