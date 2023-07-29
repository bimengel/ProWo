/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CBrowserMenu.h
 * Author: josefengel
 *
 * Created on January 20, 2020, 7:05 PM
 */

#ifndef CBROWSERMENU_H
#define CBROWSERMENU_H

class CBrowserMenu {
public:
    CBrowserMenu(int nbr);
    virtual ~CBrowserMenu();
    void InsertEntity(CBrowserEntity *pBrowserEntity);
    CBrowserEntity * SearchGroup(int pos);
    CBrowserEntity * SearchTitel(int Niv1, int Niv2, int Niv3, int Niv4);
    int GetAnzahlGroup();
private:
    int m_iGroupIdx;
    int m_iAnzGroup;
    CBrowserEntity ** m_pGroup;
};

#endif /* CBROWSERMENU_H */

