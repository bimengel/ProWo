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

#ifndef _CBROWSERENTITY_H_
#define _CBROWSERENTITY_H_

#define MSGSIZE 200

class CBrowserEntity
{
public:
    CBrowserEntity();
    void SetText(char *strPtr);
    char * GetText();
    void SetImage(char *strPtr);
    char * GetImage();
    class CBrowserEntity * SearchMenu(class CBrowserEntity *pMenu, int Niv1, int Niv2,
                              int Niv3, int Niv4);
    void SetNiv(int Niv1, int Niv2, int Niv3, int Niv4);
    CBrowserEntity * SearchGroup(int pos);
    int GetNiv1();
    int GetNiv2();
    int GetNiv3();
    int GetNiv4();
    int GetTyp(); 
    void SetTyp(int iTyp);
    void SetSammelSchalter(bool bVal);
    bool GetSammelSchalter();
    CBrowserEntity * GetNextMenu();
    void SetNextMenu(CBrowserEntity * pNextMenu);
    CBerechneBase * GetOperState();
    void SetOperState(CBerechneBase * pOperState);
	CBerechneBase * GetOperChange();
    void SetOperChange(CBerechneBase * pOperChange);
private:
    int m_iTyp; 
    // bei der Steuerung sonst nicht gebraucht
        // = 1, nur Text
        // = 2, Schalter
        // = 3, Schalter mit Schieber
        // = 4, up/stop/down
        // = 5, up/stop/down mit Schieber
    int m_iNiv1; // 1=Steuerung, 2=Heizung .....
    int m_iNiv2;
    int m_iNiv3;
    int m_iNiv4;    
    class CBrowserEntity *m_pNextMenu;
    class CBerechneBase *m_pOperState;  // mit getState wird der Zustand abgefragt
                                        // mit GetContent wird der Inhalt abgefragt
    class CBerechneBase *m_pOperChange; // Operant der geändert werden soll
    char *m_pImage;       
    char *m_pText;  
    bool m_bSammelSchalter;          

};


#endif // _CBROWSERENTITY_H_
