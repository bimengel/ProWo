/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2014 <root@raspberrypi>
 * 
ProWo is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ProWo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ProWo.h"

//
// Class CBrowserEntity
//
CBrowserEntity::CBrowserEntity()
{
    m_pText = NULL;             // Bezeichnung
    m_pImage = NULL;            // .png Grafik
    m_pNextMenu = NULL;
    m_pOperState = NULL;
    m_pOperChange = NULL;
    m_iTyp = 0;
    m_iNiv1 = 0;
    m_iNiv2 = 0;
    m_iNiv3 = 0;
    m_iNiv4 = 0;
    m_bSammelSchalter = false;
}

void CBrowserEntity::SetText(char *strPtr)
{
    char *ptr;

    ptr = new char [strlen(strPtr)+1];
    sprintf(ptr, strPtr);
    m_pText = ptr;
}

void CBrowserEntity::SetImage(char *strPtr)
{
    char *ptr;

    ptr = new char [strlen(strPtr)+1];
    sprintf(ptr, strPtr);
    m_pImage = ptr;
}

class CBrowserEntity * CBrowserEntity::SearchMenu(class CBrowserEntity *pMenu, int Niv1, int Niv2,
                                      int Niv3, int Niv4)
{
    pMenu = pMenu->m_pNextMenu;
    while(pMenu != NULL)
    {
        if(!Niv2)
        {
            if(pMenu->m_iNiv1 == Niv1 && pMenu->m_iNiv3 == 0)
                break;
        }
        else if(!Niv3)
        {
            if(pMenu->m_iNiv1 == Niv1 && pMenu->m_iNiv2 == Niv2 && pMenu->m_iNiv4 == 0)
                break;
        }
        else
        {
            if(pMenu->m_iNiv1 == Niv1 && pMenu->m_iNiv2 == Niv2 && pMenu->m_iNiv3 == Niv3)
                break;
        }

        pMenu = pMenu->m_pNextMenu;

    }

    return pMenu;
}

//
//	Speichert die Werte der verschiedenen Niveaus
//  Wird nur beim Anlegen (Lesen) der Menustruktur benötigt
//
void CBrowserEntity::SetNiv(int Niv1, int Niv2, int Niv3, int Niv4)
{
    m_iNiv1 = Niv1;
    m_iNiv2 = Niv2;
    m_iNiv3 = Niv3;
    m_iNiv4 = Niv4;
}
