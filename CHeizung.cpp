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

#include "ProWo.h"

CUhrTemp::CUhrTemp()
{
    m_iUhr = 0;
    m_iTemp = 0;
}

CUhrTemp * CHeizProgrammTag::GetHKUhrTemp(int nr)
{
    return (m_pUhrTemp + (nr-1)*ANZUHRTEMP);
}

void CHeizKoerper::Control(CUhrTemp * pUhrTemp)
{
    int i;
    int iUhrMin;

    // Tagesuhrzeit in Minuten
    iUhrMin = m_pUhr->getUhrmin();

    for(i=1; i < ANZUHRTEMP; i++)
    {
        if(!pUhrTemp[i].m_iUhr || pUhrTemp[i].m_iUhr > iUhrMin)
            break;
    }
    if(i == ANZUHRTEMP)
        i = 1;
    m_iTempSoll = pUhrTemp[i-1].m_iTemp;

    if(m_pSensor == NULL) // Warmwasser !!
    {
        if(m_iTempSoll > 40)
            m_iEinAus = 1;
        else
            m_iEinAus = 0;
    }
    else
    {
        m_iTemp = m_pSensor->GetTemp();

        if(m_iEinAus)
        {   // ist eingeschaltet
            if(m_iTemp > m_iTempSoll) // wird ausgeschaltet
                m_iEinAus = 0;
        }
        else // ist nicht eingeschaltet
        {	
            if(m_iTemp < m_iTempSoll - HYSTERESE) // wird eingeschaltet 
                m_iEinAus =1;
        }
    }
 
}

CHeizKoerper::CHeizKoerper()
{
    m_pSensor = NULL;
    m_iEinAus = 1;
    m_iTemp = 0;
    m_iTempSoll = STDTEMP;
    m_iSensorNr = 0;
}

int CHeizKoerper::GetState()
{
    return m_iEinAus;
}

int CHeizKoerper::GetTemp()
{
    return m_iTemp;
}
int CHeizKoerper::GetTempSoll()
{
    return m_iTempSoll;
}
void CHeizKoerper::SetSensor(CSensor *ptr, int iNr)
{
    m_pSensor = ptr;
    m_iSensorNr = iNr;
}
CSensor * CHeizKoerper::GetSensor()
{
    return m_pSensor;
}

int CHeizKoerper::GetSensorNr()
{
    return m_iSensorNr;
}
CHeizKoerper * CHeizung::GetHKAddress(int nr)
{
    return m_pHeizKoerper[nr - 1];
}
void CHeizKoerper::SetName(string str)
{
    m_strName = str;
}

string CHeizKoerper::GetName()
{
    return m_strName;
}

void::CHeizKoerper::SetImage(string str)
{
    m_strImage = str;
}

string CHeizKoerper::GetImage()
{
    return m_strImage;
}
//
// Klasse Heizung mit den verschiedenen Heizungskoerper
//
CHeizung::CHeizung()
{
    m_iAnzHeizKoerper = 0;
    m_iAnzHeizProgramme = 0;
    m_iAktHeizProgramm = 0;
    m_iAktHeizProgrammTag = 0;
}

int CHeizung::GetHKAnz()
{
    return m_iAnzHeizKoerper;
}

CHeizung::~CHeizung()
{
    int i;
    for(i=0; i < m_iAnzHeizKoerper; i++)
        delete m_pHeizKoerper[i];
    for(i=0; i < m_iAnzHeizProgramme; i++)
        delete m_pHeizProgramme[i];
}

void CHeizung::SetAnzHeizProgramme(int iAnz)
{
    m_iAnzHeizProgramme = iAnz;
}

void CHeizung::AddHeizKoerper(CHeizKoerper *pHeizKoerper)
{
    m_pHeizKoerper.push_back(pHeizKoerper);
    m_iAnzHeizKoerper++;
    
}

void CHeizung::AddHeizProgramm(CHeizProgramm* pHeizProgramm)
{
    m_pHeizProgramme.push_back(pHeizProgramm);
    m_iAnzHeizProgramme++;
}
int CHeizung::GetAktHeizProgramm()
{
    return m_iAktHeizProgramm;
}

int CHeizung::GetAktHeizProgrammTag()
{
    return m_iAktHeizProgrammTag;
}

int CHeizung::GetAnzHeizProgramme()
{
    return m_iAnzHeizProgramme;
}

int CHeizung::GetAnzHeizKoerper()
{
    return m_iAnzHeizKoerper;
}

CHeizProgramm * CHeizung::GetHPAddress(int nr)
{
    if(nr <= 0)
        return m_pHeizProgramme[0];
    else
        return m_pHeizProgramme[nr - 1];
}

void CHeizung::Control()
{
    int i, j;
    int iUhrZeit, iDatumTag, iWochenTag, jWochenTag;
    CHeizProgramm *pHeizProgramm;
    CHeizProgrammTag *pHeizProgrammTag;
    
    // Tagesuhrzeit in Minuten
    iUhrZeit = m_pUhr->getUhrmin();
    // Datum in Tagen
    iDatumTag = m_pUhr->getDatumTag();

    for(i=1; i < m_iAnzHeizProgramme; i++)
    {
        if(iDatumTag < m_pHeizProgramme[i]->m_iDatum)
            break;
        if(iDatumTag == m_pHeizProgramme[i]->m_iDatum &&
                            iUhrZeit < m_pHeizProgramme[i]->m_iUhrzeit)
            break;
        m_pHeizProgramme[i]->m_iDatum = 0;
        m_pHeizProgramme[i]->m_iUhrzeit = 0;
    }   
    // Wenn kein mehr gültiges Datum und Uhrzeit eines Heizprogramms definiert
    // ist, dann wir das erste aktiviert.
    if(i == m_iAnzHeizProgramme)
        i = 0;
    pHeizProgramm = m_pHeizProgramme[i];
    m_iAktHeizProgramm = i + 1;
    
    iWochenTag = m_pUhr->getWochenTag();
    m_iAktHeizProgrammTag = pHeizProgramm->GetHeizProgrammGueltigeTag(iWochenTag);
    pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(m_iAktHeizProgrammTag);
    for(i=0; i < m_iAnzHeizKoerper; i++)
        m_pHeizKoerper[i]->Control(&pHeizProgrammTag->m_pUhrTemp[i*ANZUHRTEMP]);
}

//
//              HEIZPROGRAMMTAG
//
CHeizProgrammTag::CHeizProgrammTag()
{
    m_pUhrTemp = NULL;
    m_iGueltigeTage = 0;
    m_strName.clear();
}

CHeizProgrammTag::~CHeizProgrammTag()
{
    delete [] m_pUhrTemp;
}
void CHeizProgrammTag::SetName(string str)
{
    m_strName = str;
}

string CHeizProgrammTag::GetName()
{
    return m_strName;
}
void CHeizProgrammTag::SetGueltigkeitsTage(int iTag)
{
    m_iGueltigeTage = iTag;
}
int CHeizProgrammTag::GetGueltigeTage()
{
    return m_iGueltigeTage;
}

void CHeizProgrammTag::CreateUhrTemp(int iAnzHeizKoerper)
{
    m_pUhrTemp = new CUhrTemp[iAnzHeizKoerper*ANZUHRTEMP];
    for(int i=0; i < iAnzHeizKoerper; i++)
        m_pUhrTemp[i*ANZUHRTEMP].m_iTemp = STDTEMP;
}

void CHeizProgrammTag::SetUhrTemp(int iHeizKoerper, int iIdx, int iUhr, int iTemp)
{
    m_pUhrTemp[(iHeizKoerper-1)*ANZUHRTEMP + iIdx].m_iUhr = iUhr;
    m_pUhrTemp[(iHeizKoerper-1)*ANZUHRTEMP + iIdx].m_iTemp = iTemp;
}

//
//              HEIZPROGRAMM
//
CHeizProgramm::CHeizProgramm()
{
    m_iDatum = 0;
    m_iUhrzeit = 0;
    m_strName.clear();
    m_iAnzHeizProgrammTag = 0;
}

CHeizProgramm::~CHeizProgramm()
{
    int i;
    for(i=0; i < m_iAnzHeizProgrammTag; i++)
        delete m_pHeizProgrammTag[i];
}

void CHeizProgramm::DeleteHeizProgrammTag(int nr)
{
    CHeizProgrammTag *pHeizProgrammTag;
    
    if(nr > 0 && nr <= m_iAnzHeizProgrammTag)
    {
        pHeizProgrammTag =  m_pHeizProgrammTag[nr-1];
        m_pHeizProgrammTag.erase(m_pHeizProgrammTag.begin() + nr - 1);
        delete pHeizProgrammTag;
        m_iAnzHeizProgrammTag--;
    }
}
int CHeizProgramm::GetHeizProgrammGueltigeTag(int iWochenTag)
{
    int i, iTag, iGueltigeTage;
    
    iTag = 0x40;
    while(--iWochenTag >= 0)
        iTag /= 2;
    for(i = 0; i < m_iAnzHeizProgrammTag; i++)
    {
        iGueltigeTage = m_pHeizProgrammTag[i]->GetGueltigeTage();
        if(iTag & iGueltigeTage)
            break;
    }    
    if(i == m_iAnzHeizProgrammTag)
        i = 0;
    return i+1;
}

CHeizProgrammTag * CHeizProgramm::GetHeizProgrammTag(int nr)
{
    CHeizProgrammTag * pTag;
    
    if(nr > 0 && nr <= m_iAnzHeizProgrammTag)
        pTag = m_pHeizProgrammTag[nr-1];
    else
        pTag = NULL;
    return pTag;
}
void CHeizProgramm::SetName(string str)
{
    m_strName = str;
}

string CHeizProgramm::GetName()
{
    return m_strName;
}

int CHeizProgramm::GetAnzHeizProgrammTag()
{
    return m_iAnzHeizProgrammTag;
}
void CHeizProgramm::AddHeizProgrammTag(CHeizProgrammTag* pHeizProgrammTag)
{
    m_pHeizProgrammTag.push_back(pHeizProgrammTag);
    m_iAnzHeizProgrammTag++;
}

void CHeizung::WriteDatei(char * pProgrammPath)
{
    char buf[MSGSIZE+1];
    int len, i, j;
    int iHeizKoerper, iUhrTemp, iStunde, iMinute, iPrgTag;
    int iHeizProgramm, iHeizProgrammTag;
    CUhrTemp * pUhrTemp;
    CReadFile *pReadFile;
    CHeizProgramm *pHeizProgramm;
    CHeizProgrammTag * pHeizProgrammTag;
    bool bFirst;
    string strTag, str;
    
    pReadFile = new CReadFile;
    pReadFile->OpenWrite (pProgramPath, 6, 0);
 
    for(iHeizKoerper = 0; iHeizKoerper < m_iAnzHeizKoerper; iHeizKoerper++)
    {
        bFirst = true;
        if(iHeizKoerper == 0)
        {
            str = "HEIZKOERPER\n";
            pReadFile->WriteLine(str.c_str());
        }
        if(m_pHeizKoerper[iHeizKoerper]->GetSensorNr())
        {
            str = " \"" + m_pHeizKoerper[iHeizKoerper]->GetName() + "\";" 
                            + to_string( m_pHeizKoerper[iHeizKoerper]->GetSensorNr()) + "\n";
            pReadFile->WriteLine(str.c_str());
        }
        else
        {
            if(bFirst)
            {
                bFirst = false;
                str = "WARMWASSER\n";
                pReadFile->WriteLine(str.c_str());
            }
            str = " \"" + m_pHeizKoerper[iHeizKoerper]->GetName() + "\"\n";
            pReadFile->WriteLine(str.c_str());
        }   
    }
    for(iHeizProgramm = 0; iHeizProgramm < m_iAnzHeizProgramme; iHeizProgramm++)
    {
        str = "HEIZPROGRAMM\n";
        pReadFile->WriteLine(str.c_str());
        pHeizProgramm = m_pHeizProgramme[iHeizProgramm];
        str = " \"" + pHeizProgramm->GetName() + "\"";
        if(pHeizProgramm->m_iDatum)
        {
            int iTag, iMonat, iJahr, iStunde, iMinute, iUhrzeit;
            iUhrzeit = pHeizProgramm->m_iUhrzeit;
            m_pUhr->getDatumTag(pHeizProgramm->m_iDatum, &iJahr, &iMonat, &iTag);
            iStunde = iUhrzeit / 60;
            iMinute = iUhrzeit % 60;
            str += ";" + to_string(iTag) + ":" + to_string(iMonat) + ":" + to_string(iJahr) 
                        + "-" + to_string(iStunde) + "h" + to_string(iMinute) + "\n";
        }
        else
            str += "\n";
        pReadFile->WriteLine(str.c_str());
        for(iHeizProgrammTag = 0; iHeizProgrammTag < pHeizProgramm->GetAnzHeizProgrammTag(); iHeizProgrammTag++)
        {
            pHeizProgrammTag = pHeizProgramm->GetHeizProgrammTag(iHeizProgrammTag+1);
            i = pHeizProgrammTag->GetGueltigeTage();
            strTag.clear();
            for(j = 64; j ; )
            {
                if(j & i)
                    strTag += "1";
                else
                    strTag += "0";
                j /= 2;
            }
            str = "  " + strTag + ";\"" + pHeizProgrammTag->GetName() + "\"\n";
            pReadFile->WriteLine(str.c_str());
            str.clear();
            for(iHeizKoerper = 0 ; iHeizKoerper < m_iAnzHeizKoerper; iHeizKoerper++) 
            {
                pUhrTemp = pHeizProgrammTag->GetHKUhrTemp(iHeizKoerper+1);
                for(iUhrTemp = 0; iUhrTemp < ANZUHRTEMP; iUhrTemp++) {
                    if(iUhrTemp)
                        str += ";";
                    else
                        str += "    ";
                    iStunde = (pUhrTemp + iUhrTemp)->m_iUhr / 60;
                    iMinute = (pUhrTemp + iUhrTemp)->m_iUhr % 60;
                    str += to_string(iStunde) + "h" + to_string(iMinute) + "-" + to_string((pUhrTemp + iUhrTemp)->m_iTemp);
                }
                    str += "\n";
            }
            pReadFile->WriteLine(str.c_str());          
        }
    }
    pReadFile->Close();
    delete pReadFile;
 
}