/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CStatistic.cpp
 * Author: josefengel
 * 
 * Created on April 26, 2020, 12:23 PM
 */

#include "ProWo.h"
#include <fstream>

CStatistic::CStatistic() {
    m_pDaten = NULL;
    m_iFirstYear = 0;
    m_iAktYear = 0;
    m_iError = 0;  
    m_iFileTyp =0;
    m_iValTyp = VALTYPTOT;
    m_iFactor = 1;
}

CStatistic::~CStatistic() {

    if(m_pDaten != NULL)
    {
        delete [] m_pDaten;
        m_pDaten = NULL;
    }
}

void CStatistic::LesDatei(int nr)
{
    CReadFile *pReadFile;
    int year, mon, i, tot, y, min, max, help;
    struct tm t;
    bool bEnd = false;
    string str;

    m_iNummer = nr;

    m_pUhr->getUhrzeitStruct(&t);

    pReadFile = new CReadFile;
    switch(m_iFileTyp) {
        case 5:
            i = 16;
            break;
        case 12:
            i = 17;
            break;
        default:
            i = 0;
            break;
    }
    // JEN 1.09.23
    // iTest = 1 angefügt, denn es wird getestet ob die Datei im
    // Standardverzeichnis existiert !!
    if(i && pReadFile->OpenRead (pProgramPath, i, nr, 1))
    {
        while(!bEnd)
        {	
            if(pReadFile->ReadLine())
            {
                year = pReadFile->ReadNumber ();
                if(year < 2000 || year > 2100 || year > t.tm_year+1900)
                    pReadFile->Error (60);
                if(!pReadFile->ReadSeparator ())
                    pReadFile->Error (62);
                if(year == 2000)    // letzte Zeile !!
                {
                    ReadSpecificLine(pReadFile);
                    m_strName = pReadFile->ReadText(';');
                    if(!pReadFile->ReadSeparator ())
                        pReadFile->Error (62);
                    m_strEinheit = pReadFile->ReadText(';');
                    bEnd = true;
                    continue;
                }
                mon = pReadFile->ReadZahl ();
                if(mon < 1 || mon > 12)
                    pReadFile->Error (61);
                if(!pReadFile->ReadSeparator ())
                    pReadFile->Error (62);
                if(!m_iFirstYear)
                {
                    m_iFirstYear = year;
                    m_pDaten = AppendYear((struct _yearDaten *)NULL, t.tm_year+1900);
                }
                y = year - m_iFirstYear;
                for(i=0, tot = 0, min=INT_MAX, max=INT_MIN; i < 31; )
                {
                    help = pReadFile->ReadZahl ();
                    m_pDaten[y].mon[mon-1].val[i] = help;
                    tot += help;
                    if(min > help)
                        min = help;
                    if(max < help)
                        max = help;
                    i++;
                    if(!pReadFile->ReadSeparator ())
                        break;
                }
                if(i > 0)
                {
                    m_pDaten[y].mon[mon-1].average = tot / i;
                    m_pDaten[y].mon[mon-1].total = tot;
                    m_pDaten[y].mon[mon-1].min = min;
                    m_pDaten[y].mon[mon-1].max = max;
                }
                else
                    pReadFile->Error (63);
            }
            else
            {
                if(!bEnd) // Zeile 2000 (letzte Zeile) nicht gefunden !!
                    pReadFile->Error (125);
                break;
            }
        }
        switch(m_iFileTyp) {
            case 5:
                str = "Zaehler" + to_string(m_iNummer) + ".dat has been read";
                break;
            case 12:
                str = "WSDaten" + to_string(m_iNummer) + ".dat has been read";
                break;
            default:
                str = "CStatistic unknow file has been read !!!!";
                break;
        }
    }
    else // Datei existiert noch nicht
    {
        m_iFirstYear = t.tm_year+1900;
        m_pDaten = AppendYear((struct _yearDaten *)NULL, t.tm_year+1900);
        switch(m_iFileTyp) {
            case 5:
                str = "Zaehler" + to_string(m_iNummer) + ".dat has been created";
                break;
            case 12:
                str = "WSDaten" + to_string(m_iNummer) + ".dat has been created";
                break;
            default:
                str = "CStatistic unknow file has been created !!!!";
                break;
        }
    }

    pReadFile->Close ();
    delete pReadFile;
    syslog(LOG_INFO, str.c_str());
}

void CStatistic::SaveValue(int iVal)
{
    int i, tot, min=INT_MAX, max=INT_MIN, help;
    int day, mon, year;

    day = m_pUhr->getLetzteTag();
    mon = m_pUhr->getLetzteMonat();
    year = m_pUhr->getLetzteJahr();

    //
    // wenn das letzte Jahr < 
    // wenn das letzte Jahr < als das erste Jahr ist oder > als das Aktuelle +1
    // dann ist es unlogisch !!!
    //  JEN 1.05.20
    //
    if(year < m_iFirstYear || year > m_iAktYear + 1)
    {
        syslog(LOG_ERR, "SaveValue not a logic year!!");
        return;
    }
    if(year > m_iAktYear)
    {
        m_pDaten = AppendYear (m_pDaten, year);
        m_iAktYear = year;
    }

    if(day > 0 && day < 32 && mon > 0 && mon < 13)
    {
        m_pDaten[year-m_iFirstYear].mon[mon-1].val[day-1] = iVal;
        for(i=0, tot=0; i < day; i++)
        {
            help = m_pDaten[year-m_iFirstYear].mon[mon-1].val[i];
            tot += help;
            if(min > help)
                min = help;
            if(max < help)
                max = help;
        }
        m_pDaten[year-m_iFirstYear].mon[mon-1].total = tot;
        m_pDaten[year-m_iFirstYear].mon[mon-1].average = tot / i;
        m_pDaten[year-m_iFirstYear].mon[mon-1].min = min;
        m_pDaten[year-m_iFirstYear].mon[mon-1].max = max;
    }

    SchreibDatei();
}

void CStatistic::SetName(string str)
{
    m_strName = str;
    SchreibDatei();
}

string CStatistic::GetName()
{
    return m_strName;
}

string CStatistic::GetEinheit()
{
    return m_strEinheit;
}

int CStatistic::GetFirstYear()
{
    return m_iFirstYear;
}

int CStatistic::GetAktYear()
{
    return m_iAktYear;
}

struct _yearDaten * CStatistic::AppendYear(struct _yearDaten *pAlt, int endYear)
{
    struct _yearDaten * pNeu;
    int y, i, j;

    if(endYear > m_iAktYear)
    {   
        pNeu = new struct _yearDaten[endYear - m_iFirstYear + 1];

        y = endYear - m_iFirstYear;
        for(j=0; j < 12; j++)
        {
            for(i=0; i < 31; i++)
                pNeu[y].mon[j].val[i] = 0;
            pNeu[y].mon[j].average = 0;
            pNeu[y].mon[j].total = 0;
            pNeu[y].mon[j].min = 0;
            pNeu[y].mon[j].max = 0;
        }   
        pNeu[y].year = endYear;
        if(pAlt != NULL)
        {
            i = sizeof(struct _yearDaten) * (endYear - m_iFirstYear);
            memcpy((void *)pNeu, (void *)pAlt, i);
            delete [] pAlt;
        }
        m_iAktYear = endYear;
    }
    else
        return (struct _yearDaten *)NULL;

    return pNeu;
}

void CStatistic::SchreibDatei()
{
    CReadFile *pReadFile = NULL;
    string strName, strPfadJahr, strPfadDatei, strPfadData, strBuffer;
    struct stat status;
    int result, pos;
    int iYear, iMon, iDay, iAnz, iStand;
    int year, mon, day;
    bool bOK;

    day = m_pUhr->getLetzteTag();
    mon = m_pUhr->getLetzteMonat();
    year = m_pUhr->getLetzteJahr ();

    try 
    {
        pReadFile = new CReadFile;
        strName = pReadFile->GetFileName (pProgramPath, m_iFileTyp, m_iNummer);
        pos = strName.find_last_of("/");
        strPfadDatei = strName.substr(0, pos);
        pReadFile->ControlPath(strName);

        // ZaehlerX.dat umbenennen in JJJJMMMTT
        strBuffer = strPfadDatei + "/" + to_string(year) + strZweiStellen(mon) + strZweiStellen(day);
        result = rename(strName.c_str(), strBuffer.c_str());

        // ZaehlerX.dat neu erstellen und alle Daten speichern
        pReadFile->OpenWrite(pProgramPath, m_iFileTyp, m_iNummer);
        for(bOK=true, iYear = 0; bOK && iYear <= year - m_iFirstYear; iYear++)
        {
            for(iMon = 0; bOK && iMon < 12; iMon++)
            {
                strBuffer = to_string(iYear + m_iFirstYear) + ";" + to_string(iMon+1);
                iAnz = m_pUhr->getAnzahlTage (iYear + m_iFirstYear, iMon+1);
                if(iYear+m_iFirstYear == year && iMon+1 == mon)
                {
                    iAnz = day;
                    bOK = false;
                }
                for(iDay = 0; iDay < iAnz; iDay++)
                    strBuffer += ";" + to_string(m_pDaten[iYear].mon[iMon].val[iDay]);
                strBuffer += "\n";
                pReadFile->WriteLine(strBuffer.c_str());
            }
        }

        strBuffer = WriteSpecificLine();
        strBuffer += "\"" + m_strName + "\";\""  + m_strEinheit + "\"\n";
        pReadFile->WriteLine(strBuffer.c_str());

        pReadFile->Close();
        delete pReadFile;
        pReadFile = NULL;

        switch(m_iFileTyp) {
            case 5:
                result = 16;
                break;
            case 12:
                result = 17;
                break;
            default:
                result = 0;
                break;
        }
        if(result) {
            pReadFile = new CReadFile;
            strBuffer = pReadFile->GetFileName (pProgramPath, result, m_iNummer);
            copyFile(strName.c_str(), strBuffer.c_str());
            delete pReadFile;
        }
        else {
            strName = "Can't copy file " + strName;
            syslog(LOG_ERR, strName.c_str());
        }

    }
    catch(int e)
    {
        syslog(LOG_ERR, "Error counter save day values !!");
        if(pReadFile != NULL)
        {
            pReadFile->Close();
            delete pReadFile;
        }
    }
}

bool CStatistic::copyFile(const char *SRC, const char* DEST)
{
    std::ifstream src(SRC, std::ios::binary);
    std::ofstream dest(DEST, std::ios::binary);
    dest << src.rdbuf();
    return src && dest;
}
//
//  Rechnet den Mittelwert PRO TAG des entsprechenden Monats aller gespeicherten Jahre
//  NB : der aktuelle Monat wird nicht berücksichtigt !!!
//
int CStatistic::GetAverageMonDay(int mon)
{
    int year, average, anz, iMon;
    struct tm t;

    m_pUhr->getUhrzeitStruct(&t);
    for(average=0, year=0, anz=0; year <= m_iAktYear - m_iFirstYear; year++)
    {
        if(year == m_iAktYear - m_iFirstYear && mon == t.tm_mon+1)
            break;
        if(m_pDaten[year].mon[mon-1].average)
        {   
            average += m_pDaten[year].mon[mon-1].average;
            anz++;
        }
    }
    if(average)
        average /= anz;

    return average;
}

//
//  Rechnet den Mittelwert des entsprechenden Monats bezogen auf gespeicherte Daten
//  NB : der aktuelle Monat wird nicht berücksichtigt !!!
//
int CStatistic::GetAverageMonTotal(int mon)
{
    int year, average, anz, iMon;
    struct tm t;

    m_pUhr->getUhrzeitStruct(&t);
    for(average=0, year=0, anz=0; year <= m_iAktYear - m_iFirstYear; year++)
    {
        if(year == m_iAktYear - m_iFirstYear && mon == t.tm_mon+1)
            break;
        if(m_pDaten[year].mon[mon-1].total)
        {   
            average += m_pDaten[year].mon[mon-1].total;
            anz++;
        }
    }
    if(average)
        average /= anz;

    return average;
}

int CStatistic::GetIntWert(int iJahr, int iMonat, int iTag, int iWert)
{
    int iRet;
    struct tm t;
    
    m_pUhr->getUhrzeitStruct (&t);    
    if(iJahr >= m_iFirstYear && iJahr <= m_pUhr->getJahr() && iMonat > 0 && iMonat <= 12 && iTag > 0 && iTag <= 31 )
    {
        if(iJahr == t.tm_year+1900 && iMonat == t.tm_mon+1 && iTag == t.tm_mday)
            iRet = iWert;
        else
            iRet = m_pDaten[iJahr-m_iFirstYear].mon[iMonat-1].val[iTag-1]; 
    }
    else
        iRet = 0;
    
    return iRet;
}
double CStatistic::GetDblWert(int iJahr, int iMonat, int iTag, int iWert)
{
    return (double)GetIntWert(iJahr, iMonat, iTag, iWert) / (double)m_iFactor;
}
string CStatistic::GetStringWert(int iJahr, int iMonat, int iTag, int iWert)
{
    char buf[MSGSIZE];
    string strRet;
    
    snprintf(buf, MSGSIZE, "%0.2f", GetDblWert(iJahr, iMonat, iTag, iWert));
    strRet = buf;
    
    return strRet;
}

int CStatistic::GetIntWert(int iJahr, int iMonat, int iWert)
{
    int iRet;
    struct tm t;
    
    m_pUhr->getUhrzeitStruct (&t);
    if(iJahr >= m_iFirstYear && iJahr <= m_pUhr->getJahr() && iMonat > 0 && iMonat <= 12)
    {
        switch(m_iValTyp) {
            case VALTYPTOT:
                iRet = m_pDaten[iJahr-m_iFirstYear].mon[iMonat-1].total;
                if(iJahr == t.tm_year + 1900 && iMonat == t.tm_mon+1)
                    iRet += iWert;
                break;
            case VALTYPMIN:
                iRet = m_pDaten[iJahr-m_iFirstYear].mon[iMonat-1].min;
                if(iJahr == t.tm_year + 1900 && iMonat == t.tm_mon+1 && iRet > iWert)                
                    iRet = iWert;
                break;
            case VALTYPMAX:
                iRet = m_pDaten[iJahr-m_iFirstYear].mon[iMonat-1].max;
                if(iJahr == t.tm_year + 1900 && iMonat == t.tm_mon+1 && iRet < iWert)                
                    iRet = iWert;
                break;
            default:
                iRet = 0;
                break;
        }
    }
    else
        iRet = 0;
    
    return iRet;
}
double CStatistic::GetDblWert(int iJahr, int iMonat, int iWert)
{
    return (double)GetIntWert(iJahr, iMonat, iWert) / (double)m_iFactor;
}
string CStatistic::GetStringWert(int iJahr, int iMonat, int iWert)
{
    char buf[MSGSIZE];
    string strRet;
    
    snprintf(buf, MSGSIZE, "%0.2f", GetDblWert(iJahr, iMonat, iWert));
    strRet = buf;
    
    return strRet;
}

int CStatistic::GetIntWert(int iJahr, int iWert)
{
    int i, iRet;
    struct tm t;
    
    m_pUhr->getUhrzeitStruct (&t);
    if(iJahr >= m_iFirstYear && iJahr <= m_pUhr->getJahr())
    {
        switch(m_iValTyp) {
            case VALTYPTOT:
                for(i=0, iRet=0; i < 12; i++)
                    iRet += m_pDaten[iJahr-m_iFirstYear].mon[i].total;
                if(t.tm_year+1900 == iJahr)
                    iRet += iWert;
                break;
            case VALTYPMIN:
                for(i=0, iRet=INT_MAX; i < 12; i++)
                {
                    if(iRet > m_pDaten[iJahr-m_iFirstYear].mon[i].min)
                        iRet = m_pDaten[iJahr-m_iFirstYear].mon[i].min;
                }
                if(t.tm_year+1900 == iJahr && iRet > iWert)
                    iRet = iWert;
                break;
            case VALTYPMAX:
                for(i=0, iRet=INT_MIN; i < 12; i++)
                {
                    if(iRet < m_pDaten[iJahr-m_iFirstYear].mon[i].max)
                        iRet = m_pDaten[iJahr-m_iFirstYear].mon[i].max;
                }
                if(t.tm_year+1900 == iJahr && iRet < iWert)
                    iRet = iWert;
                break;
            default:
                iRet = 0;
                break;
        }
    }
    else
        iRet = 0;
    
    return iRet;
}
double CStatistic::GetDblWert(int iJahr, int iWert)
{
    return (double)GetIntWert(iJahr, iWert) / (double)m_iFactor;
}
string CStatistic::GetStringWert(int iJahr, int iWert)
{
    char buf[MSGSIZE];
    string strRet;
    
    snprintf(buf, MSGSIZE, "%0.2f", GetDblWert(iJahr, iWert));
    strRet = buf;
    
    return strRet;
}

void CStatistic::SetFactor(int iFactor)
{
    m_iFactor = iFactor;
}

int CStatistic::GetFactor()
{
    return m_iFactor;
}

int CStatistic::GetResultInt(int iInterval, int iDiff, int iWert)
{
    int iRet, i, j;
    time_t iTime;
    struct tm uhr;
    int jahr, monat, tag;
    
    switch(iInterval) {
        case 1: // Zähleränderung vor iDiff Tagen
            iTime = m_pUhr->getUhrzeit();
            iTime -= iDiff * 86400;
            m_pUhr->getUhrzeitStruct(&uhr, iTime);
            iRet = GetIntWert(uhr.tm_year + 1900, uhr.tm_mon+1, uhr.tm_mday, iWert); 
            break;
        case 2: // Änderung einer Woche
            i = m_pUhr->getWochenTag();
            iTime = m_pUhr->getUhrzeit() - (iDiff*7 + i) * 86400;
            if(iDiff)
                i = 6;
            m_pUhr->getUhrzeitStruct(&uhr, iTime); 
            jahr = uhr.tm_year + 1900;
            monat = uhr.tm_mon+1;
            tag = uhr.tm_mday;
            switch(m_iValTyp) {
                case VALTYPTOT:
                    iRet = 0;
                    break;
                case VALTYPMIN:
                    iRet = INT_MAX;
                    break;
                case VALTYPMAX:
                    iRet = INT_MIN;
                    break;
                default:
                    iRet = 0;
                    break;
            }
            for(; i >= 0; i--)
            {
                j = GetIntWert(jahr, monat, tag, iWert); 
                switch(m_iValTyp) {
                    case VALTYPTOT:
                        iRet += j;
                        break;
                    case VALTYPMIN:
                        if(j < iRet)
                            iRet = j;
                        break;
                    case VALTYPMAX:
                        if(j > iRet)
                            iRet = j;
                        break;
                    default:
                        break;
                }                
                if(++tag > m_pUhr->getAnzahlTage(jahr, monat))
                {
                    tag = 1;
                    if(++monat > 12)
                    {
                        monat = 1;
                        if(++jahr < m_iAktYear)
                            break;
                    }
                }
            }
            break;
        case 3: // Zähleränderung vor iDiff Monaten
            jahr = m_pUhr->getJahr();
            monat = m_pUhr->getMonat() - iDiff;
            while(monat <= 0)
            {
                monat += 12;
                jahr--;
            }
            iRet = GetIntWert(jahr, monat, iWert);
            break;
        case 4: // Zähleränderung vor iDiff Jahren
            jahr = m_pUhr->getJahr() - iDiff;
            iRet = GetIntWert(jahr, iWert);
            break;
        default:
            iRet = 0;
            break;
    }
    return iRet;
}