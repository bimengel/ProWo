/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2016 <root@ProWo2>
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

#ifndef _CHEIZUNG_H_
#define _CHEIZUNG_H_

#define ANZUHRTEMP      10

class CUhrTemp
{
public:
    CUhrTemp();
    int m_iUhr;
    int m_iTemp;
};

class CHeizKoerper
{
public:
    CHeizKoerper();
    int GetState();
    void Control(CUhrTemp * pUhrTemp);
    void SetSensor(CSensor *ptr, int iNr);
    int GetSensorNr();
    CSensor * GetSensor();
    void SetName(string str);
    string GetName();
    void SetImage(string str);
    string GetImage();
    int GetTemp();
    int GetTempSoll();
    
protected:
    string m_strName;
    string m_strImage;
    CSensor *m_pSensor;
    int m_iEinAus;
    int m_iTemp;
    int m_iTempSoll;
    int m_iSensorNr;
private:
	
};
class CHeizProgrammTag
{
public:
    CHeizProgrammTag();
    ~CHeizProgrammTag();
    void CreateUhrTemp(int iAnzahlKoerper);
    void SetUhrTemp(int iHeizKoerper, int iIdx, int iUhr, int iTemp);
    CUhrTemp * GetHKUhrTemp(int nr);
    void SetName(string str);
    string GetName();
    void SetGueltigkeitsTage(int iTag);
    int GetGueltigeTage();
    CUhrTemp * m_pUhrTemp;
protected:
    int m_iGueltigeTage; 
    string m_strName;

};

class CHeizProgramm
{
public: 
    CHeizProgramm();
    ~CHeizProgramm();

    void SetName(string str);
    string GetName();
    int GetAnzHeizProgrammTag();
    void AddHeizProgrammTag(CHeizProgrammTag *pHeizProgrammTag);
    int GetHeizProgrammGueltigeTag(int iWochenTag);
    CHeizProgrammTag * GetHeizProgrammTag(int nr);
    void DeleteHeizProgrammTag(int nr);
    int m_iDatum;
    int m_iUhrzeit;

protected:
    string m_strName;
    int m_iAnzHeizProgrammTag;
    vector<CHeizProgrammTag *> m_pHeizProgrammTag;
};

class CHeizung
{
public:
    CHeizung();
    ~CHeizung();
    int GetHVAnz();
    void AddHeizKoerper(CHeizKoerper * pHeizKoerper);
    void AddHeizProgramm(CHeizProgramm * pHeizProgramm);
    int GetHKAnz();
    CHeizKoerper * GetHKAddress(int nr);
    CHeizProgramm * GetHPAddress(int nr);
    void WriteDatei(char *pProgrammPath);
    int GetAktHeizProgramm();
    int GetAktHeizProgrammTag();
    int GetAnzHeizProgramme();
    int GetAnzHeizKoerper();
    void SetAnzHeizProgramme(int iAnz);
    void Control(); // wird 1x pro Minute aufgerufen und berechnet den Status
                    // aller Ventile
	
protected:
    int m_iAnzHeizKoerper;
    int m_iAnzHeizProgramme;
    vector<CHeizKoerper *> m_pHeizKoerper;
    vector<CHeizProgramm *> m_pHeizProgramme;
    int m_iAktHeizProgramm;
    int m_iAktHeizProgrammTag;
	
private:

};

#endif // _CHEIZUNG_H_
