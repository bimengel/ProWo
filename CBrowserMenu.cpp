/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CBrowserMenu.cpp
 * Author: josefengel
 * 
 * Created on January 20, 2020, 7:05 PM
 */

#include "ProWo.h"

CBrowserMenu::CBrowserMenu(int nbr) {
    
    m_iAnzGroup = nbr+1;
    m_pGroup = new CBrowserEntity *[nbr+1];
    m_iGroupIdx = -1;
}

CBrowserMenu::~CBrowserMenu() {
    if(m_pGroup)
        delete [] m_pGroup;
}

void CBrowserMenu::InsertEntity(CBrowserEntity *pBrowserEntity)
{
    CBrowserEntity *pMenu;
    
    if(!pBrowserEntity->GetNiv2() && !pBrowserEntity->GetNiv3()  && !pBrowserEntity->GetNiv4())
    {
        m_iGroupIdx++;
        m_pGroup[m_iGroupIdx] = pBrowserEntity;
    }
    else
    {
        pMenu = m_pGroup[m_iGroupIdx];
        while(pMenu->GetNextMenu() != NULL)
            pMenu = pMenu->GetNextMenu();
        pMenu->SetNextMenu(pBrowserEntity);
    }
    
}
CBrowserEntity * CBrowserMenu::SearchGroup(int pos)
{

   CBrowserEntity * pRet = NULL;
   
    if(pos >= 0 && pos < m_iAnzGroup)
        pRet = m_pGroup[pos];

    return pRet;
}

class CBrowserEntity * CBrowserMenu::SearchTitel(int iNiv1, int iNiv2, int iNiv3, int iNiv4)
{
    int i;
    CBrowserEntity * pBrowserEntity;
    
    for(i=1; i < m_iAnzGroup; i++)
    {
        if(m_pGroup[i]->GetNiv1() == iNiv1)
        {
            pBrowserEntity = m_pGroup[i];
            break;
        }    
    }    
    if(i == m_iAnzGroup)
        pBrowserEntity = NULL;
    else
    {
        for(; pBrowserEntity != NULL; pBrowserEntity = pBrowserEntity->GetNextMenu())
        {
            if(iNiv1 == pBrowserEntity->GetNiv1() && iNiv2 == pBrowserEntity->GetNiv2() &&
                            iNiv3 == pBrowserEntity->GetNiv3() && iNiv4 == pBrowserEntity->GetNiv4())
                break;

        }
    }
    return pBrowserEntity;
}

int CBrowserMenu::GetAnzahlGroup()
{
    return m_iAnzGroup;
}