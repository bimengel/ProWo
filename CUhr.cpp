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

#include <math.h>
#include "ProWo.h"

//  JEN 08.04.20
//  par default hat der Raspberry pi keine interne batteriegepufferte Uhr.
//  Jede Stunde und beim Herunterfahren wird die Uhrzeit in der Datei /etc/fake-hwclock.data
//  gespeichert und beim Einschalten wird diese Uhrzeit wieder gelesen. Diese
//  erfolgt weil das Paket "fake-hwclock" installiert ist. 
//  Aus diesem Grunde ist die RTC-Uhrzeit immer grösser beim Aufstarten und wird als Uhrzeit übernommen. 
//  Ist beim Aufstarten die RTC-Uhrzeit weniger als eine Stunde kleiner als die Uhrzeit des Betriebsystems,
//  wird die Systemuhrzeit übernommen und in den RTC geschrieben. Ist die Differenz grösser
//  wird der Startvorgang gestoppt und "Batterie leer?" angezeigt. Nach dem Druck der grünen Taste
//  besteht die Möglichkeit zwischen der Systemuhrzeit oder der RTC-Uhrzeit zu entscheiden, die gewählte
//  Uhrzeit wird auch in die RTC geschrieben. Erfolgt keine Wahl innerhalb von 3 Minuten wird die
//  Systemuhrzeit übernommen.
//
//  Das Daemon systemd-timesyncd überträgt die Uhrzeit von NTP-Servern ins System.
//  Der Status kann mit dem Kommando "sudo service systemd-timesyncd status" angezeigt werden.
//  Die Konfigurationsdatei ist "/etc/systemd/timesyncd.conf"
//  Mit dem Kommando "timedatectl status" wird die "Uhrzeit" Konfiguration angezeigt.
//  Hier wird zwar der NTP Service aufgeführt ist aber nicht in der vollen Version aktif, da
//  er sehr vielen Resourcen benötigt. In der vollen Version wird nicht die Uhrzeit im
//  System geändert, sondern die Uhr des System wird beschleunigt oder verlangsamt.
//
//
// Diese Funktion aktUht wird in "BerechneAusgaenge ()" aufgerufen und die Systemuhrzeit
// wird eingelesen. Alle Berechungen und Anzeigen basieren sich auf diese Uhrzeit.
// Nachts um zwei Uhr werden die Sonnen-aufgang und -untergangsuhrzeiten berechnet
// und die Uhrzeit in den RTC-Baustein geschrieben.
//
void CUhr::SetSimTagNacht(int state)
{
    m_iSimTagNacht = state;
}	   

void CUhr::SetSimUhrzeit(int min)
{
    m_iSimUhrzeit = min;
    m_bSimUhrzeit = true;
}

void CUhr::ResetSimulation()
{
    m_iSimTagNacht = 0;
    m_bSimUhrzeit = false;
}

void CUhr::aktUhr(bool bwriteRTC)
{
    pthread_mutex_lock(&m_mutexUhr);    
    m_iUhrYearAlt = m_iUhrYear;
    m_iUhrMonAlt = m_iUhrMon;
    m_iUhrWochenTagAlt = m_iUhrWochenTag;
    m_iUhrTagAlt = m_iUhrTag;
    m_iUhrStundeAlt = m_iUhrStunde;
    m_iUhrMinAlt = m_iUhrMin;
    m_iUhrSekAlt = m_iUhrSek;

    m_uhrzeit = time(NULL);
    m_tSys = *localtime(&m_uhrzeit);

    m_iUhrYear = m_tSys.tm_year + 1900;
    m_iUhrMon = m_tSys.tm_mon + 1;
    m_iUhrTag = m_tSys.tm_mday;
    m_iUhrStunde = m_tSys.tm_hour;
    m_iUhrMin = m_tSys.tm_hour * 60 + m_tSys.tm_min;
    m_iUhrSek = m_iUhrMin * 60 + m_tSys.tm_sec;
    m_iUhrTag = m_tSys.tm_mday;
    m_iUhrWochenTag = m_tSys.tm_wday - 1;
    if(m_iUhrWochenTag < 0)  // Montag ist der 1. Tag in der Woche !!
        m_iUhrWochenTag = 6;
    // Sonnenaufgang und -untergang werden nachts um zwei Uhr berechnet
    // und die Systemuhrzeit in RTC geschrieben
    if(bwriteRTC && m_iAktTag != m_tSys.tm_mday && m_iUhrMin > 120)
    {
        setRTC(gmtime(&m_uhrzeit));  // Uhrzeit in den RTC Baustein schreiben !!
        RechneSASU ();
        m_iAktTag = m_tSys.tm_mday;
    }
    pthread_mutex_unlock(&m_mutexUhr);
}

//
//  Diese Funktion gibt true zurück wenn die Sekunde gewechselt hat
//
bool CUhr::SekundenTakt ()
{
    bool bRet = false;
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrSek != m_iUhrSekAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);    
    return bRet;
}

//
//  Diese Funktion gibt true zurück wenn die Minute gewechselt hat
//
bool CUhr::MinutenTakt ()
{
    bool bRet = false;
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrMin != m_iUhrMinAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);    
    return bRet;
}
//
//  Diese Funktion gibt true zurück wenn die Stunde gewechselt hat
//
bool CUhr::StundenTakt ()
{
    bool bRet = false;
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrStunde != m_iUhrStundeAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);
    return bRet;
}

//
//  Diese Funktion gibt true zurück wenn der Tag gewechselt hat
//
bool CUhr::TagesTakt()
{
    bool bRet = false;
    
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrTag != m_iUhrTagAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);    
    return bRet;
} 

//
//  Diese Funktion gibt true zurück wenn die Woche gewechselt hat
//
bool CUhr::WochenTakt()
{
    bool bRet = false;
    
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrWochenTag  <  m_iUhrWochenTagAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);    
    return bRet;
} 

//
//  Diese Funktion gibt true zurück wenn die Woche gewechselt hat
//
bool CUhr::MonatsTakt()
{
    bool bRet = false;
    
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrMon  !=  m_iUhrMonAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);    
    return bRet;
} 

//
//  Diese Funktion gibt true zurück wenn die Woche gewechselt hat
//
bool CUhr::JahresTakt()
{
    bool bRet = false;
    
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iUhrYear  !=  m_iUhrYearAlt)
        bRet = true;
    pthread_mutex_unlock(&m_mutexUhr);    
    return bRet;
} 

//
//  die aktuelle Uhrzeit des "Programms" lesen (time_t)
//
time_t CUhr::getUhrzeit()
{
    time_t iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_uhrzeit;
    pthread_mutex_unlock(&m_mutexUhr);    
    return iRet;
}

int CUhr::getDatumTag()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = getDatumTag(m_iUhrYear, m_iUhrMon, m_iUhrTag);
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}
int CUhr::getDatumTag(int Jahr, int Monat, int Tag)
{
    return(Jahr*10000 + Monat*100 + Tag);
}
void CUhr::getDatumTag(int datumTag, int *Jahr, int *Monat, int *Tag)
{
    int rest;
    
    *Jahr = datumTag / 10000;
    rest = datumTag % 10000;
    *Monat = rest / 100;
    *Tag = rest % 100;
}
//
//  die aktuelle Uhrzeit des "Programms" lesen (struct tm *)
//
void CUhr::getUhrzeitStruct(struct tm *t)
{

    pthread_mutex_lock(&m_mutexUhr);
    *t = m_tSys;
    pthread_mutex_unlock(&m_mutexUhr);
}

void CUhr::getUhrzeitStruct(struct tm *tm, time_t uhrzeit)
{
    pthread_mutex_lock(&m_mutexUhr);
    *tm = *localtime(&uhrzeit);
    pthread_mutex_unlock(&m_mutexUhr);
}

//
//  die aktuelle Minute des "Programms" lesen
//
// 
int CUhr::getUhrmin()
{   
    int iRet;

    pthread_mutex_lock(&m_mutexUhr);
    if(m_bSimUhrzeit)
        iRet = m_iSimUhrzeit;
    else
        iRet = m_iUhrMin;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}

//
//   für TN gibt 1 zurück wenn Tag ist
//
int CUhr::getTagNacht()
{
    int ret = 0;
   
    pthread_mutex_lock(&m_mutexUhr);    
    if(m_iSimTagNacht == 0)
    {
        if(m_bSimUhrzeit)
        {
            if(m_iSimUhrzeit >= m_iSA && m_iSimUhrzeit <= m_iSU)
                ret = 1;
        }
        else
        {
            if(m_iUhrMin >= m_iSA && m_iUhrMin <= m_iSU)
                ret = 1;
        }
    }
    else
        ret = m_iSimTagNacht - 1;
    pthread_mutex_unlock(&m_mutexUhr);    
    return ret;
}

//
//  Rechne Sonnenaufgang und Sonnenuntergang des aktuellen Tages
//  Erfolgt einmal pro Tag
//
void CUhr::RechneSASU()
{
    double dZeitdiff, dDeklination, dB, dZeitgleichung;
    double dN = 50.27; // Daten aus der Wetterstation in Amelscheid Nord Sankt Vith
    double dE = 6.26; //              Ost Sankt Vith
    double dWOZ, dMOZ;
    double dMEZ = 1; // +1 normal, +2 in der Sommerzeit
    double dSA, dSU;

    // tm_zone = CET für MEZ = Mitteleuropäische Zeit
    //		   = CEST für MESZ = Mitteleuropäische Sommerzeit
    //if(m_tSys.tm_zone, "CES", 3) == 0)
    //
    // oder wenn tm_isdst = 1 dann ist Sommerzeit
    if(m_tSys.tm_isdst == 1)
        dMEZ = 2;

    // Bruchteile von Tagen
    dB = M_PI * dN / 180.0;
    dDeklination = 0.4095 * sin(0.016906 *((double)m_tSys.tm_yday - 80.086));
    dZeitdiff = 12.0 * acos((sin(-0.0145) - sin(dB)*sin(dDeklination)) /
                            (cos(dB) * cos(dDeklination))) / M_PI;
    dWOZ = 12 - dZeitdiff;
    dZeitgleichung = -0.171 * sin(0.0337 * m_tSys.tm_yday + 0.465) -
                                0.1299 * sin(0.01787 * m_tSys.tm_yday - 0.168);
    // dZeitgleichung = WOZ - MOZ
    dMOZ = dWOZ - dZeitgleichung;
    dSA = dMOZ + (-dE / 15) + dMEZ; 
    dSU = 12 + dZeitdiff - dZeitgleichung - dE/15 + dMEZ;
    m_iSA = (int)(dSA * 60.0);
    m_iSU = (int)(dSU * 60.0);
}

//
//  beim Shutdown speichert Linux die aktuelle Uhrzeit und nach dem erneuten
//  Start wird diese Uhrzeit als Basis genommen. 
//  Beim Starten wird diese Uhrzeit übernommen dann versucht die Uhrzeit mit 
//  hwclock zu lesen und über NTP.
//
//  In dieser Version wird die RTC-Uhrzeit im Betriebsystem übernommen wenn
//  die RTCUhrzeit > Betriebsystemuhrzeit ist
//  Nachteil : die Uhrzeit wird falsch gesetzt wenn die RTC-Uhr zu schnell läuft
//  N.B. : die RTCUhrzeit ist immer die "Greenwich"-Uhrzeit 
//
CUhr::CUhr()
{
    struct tm tRTC;
    time_t uhrzeitRTC;
    string str1, str2, str3;
    int iRet = 0, button;
    int iUeb, iTime = 0, iAbbruch = 0;
    bool bTaste;

    m_iAktTag = 0;

    m_uhrzeit = time(NULL);
    struct tm *t = gmtime(&m_uhrzeit);
    m_tSys = *t;

    pthread_mutex_init(&m_mutexUhr, NULL);
    m_pUhrAddr = new CBoardAddr;
    m_pUhrAddr->SetAddr(INHADDRI2C + 0x03, 0, 0, 0, 0);
    getRTC(&tRTC);
    // Uhrzeit in die UTC (Universal Time)  Uhrzeit umrechnen
    uhrzeitRTC = timegm(&tRTC);
    str1 = "Read UTC time from RTC : " + to_string(tRTC.tm_hour) + ":" + to_string(tRTC.tm_min) + ":" 
                    + to_string(tRTC.tm_sec) + " " + to_string(tRTC.tm_mday) + "." 
                    + to_string(tRTC.tm_mon+1) + "." + to_string(tRTC.tm_year+1900) + "\n";
    syslog(LOG_INFO, str1.c_str());
    str1 = "System UTC time : " + to_string(m_tSys.tm_hour) + ":" + to_string(m_tSys.tm_min) + ":"
                    + to_string(m_tSys.tm_sec)  + " " + to_string(m_tSys.tm_mday) + "."
                    + to_string(m_tSys.tm_mon+1) + "." + to_string(m_tSys.tm_year+1900) + "\n";
    syslog(LOG_INFO, str1.c_str());

    //
    // wenn die RTC-Uhrzeit > als aktuelle dann wird diese übernommen
    // sonst wird die aktuelle Uhrzeit in den RTC-Baustein übertragen
    //
    if(m_uhrzeit > uhrzeitRTC)
    {
        if(m_uhrzeit - 3600 > uhrzeitRTC)
        {
            iRet = 1; // Batterie leer oder RTC-Baustein defekt!!!
            str3 = "Batterie leer?";
        }
        else
            setRTC(&m_tSys); // Systemuhrzeit in den RTC-Baustein schreiben
    }
    else
    {
        if(uhrzeitRTC - m_uhrzeit > 86400)
        {
            iRet = 2; // System länger als ein Tag ausgeschaltet ! 
            str3 = "Lange ausgeschaltet?";
        }
        else 
            setSystemtime(uhrzeitRTC);
   }
    
    iUeb = 0;
    bTaste = false;
    while(iRet)
    {
        sleep(1); // 1 sec 
        button = pKeyBoard->getbutton(); 
        if(!button)
            iTime = 0;
        if(!iUeb)
        {
            m_tSys =*localtime(&m_uhrzeit);
            if(button)
                iUeb = 1;
        }
        else
        {
            getRTC(&tRTC);
            // Uhrzeit in die UTC (Universal Time)  Uhrzeit umrechnen
            uhrzeitRTC = timegm(&tRTC);
            aktUhr(false);
            if(iUeb == 1)
                str3 = "S Systemuhrzeit";
            else
                str3 = "B Uhr-Batterie";             
        }
        tRTC = *localtime(&uhrzeitRTC);
        str1 = "S " + strZweiStellen(m_tSys.tm_mday) + "." + strZweiStellen(m_tSys.tm_mon+1) + "."
                    + strZweiStellen(m_tSys.tm_year%100) +  " " + strZweiStellen(m_tSys.tm_hour)
                    + "." + strZweiStellen(m_tSys.tm_min);
        str2 = "B " + strZweiStellen(tRTC.tm_mday) + "." + strZweiStellen(tRTC.tm_mon+1) + "."
                    + strZweiStellen(tRTC.tm_year%100) +  " " + strZweiStellen(tRTC.tm_hour)
                    + "." + strZweiStellen(tRTC.tm_min);
        LCD_AnzeigeZeile1(str1.c_str());
        LCD_AnzeigeZeile2(str2.c_str());
        LCD_AnzeigeZeile3(str3.c_str());
        if(iUeb)
        {
            switch(button) {
                case BUTTONUP:
                case BUTTONDOWN:
                    if(!bTaste)
                    {
                        bTaste = true;
                        if(iUeb == 1)
                            iUeb = 2;
                        else
                            iUeb = 1;
                    }
                    iAbbruch = 0;
                    break;
                case BUTTONOK:
                    if(++iTime > 2)
                    {
                        iRet = 0;
                        LCD_Clear();
                        if(iUeb == 2)
                        {   // RTC Uhrzeit
                            getRTC(&tRTC);
                            // Uhrzeit in die UTC (Universal Time)  Uhrzeit umrechnen
                            uhrzeitRTC = timegm(&tRTC);
                            setSystemtime(uhrzeitRTC);
                        }
                        else
                        {   // Systemuhrzeit bleibt, speichern in RTC
                            t = gmtime(&m_uhrzeit);
                            m_tSys = *t;                        
                            setRTC(&m_tSys);
                        }
                    } 
                    iAbbruch = 0;                   
                    break;
                default:
                    bTaste = false;
                    break;
            }
        }
        if(++iAbbruch > 180) // JEN 30.09.23 von 10 min auf 3 min reduziert
        {
            iRet = 0;
            LCD_Clear();
        }    
    }

    m_uhrzeit = time(NULL);
    t = localtime(&m_uhrzeit);
    m_tSys = *t;
    m_bSimUhrzeit = false;
    m_iUhrTag = m_tSys.tm_mday;
    m_iUhrTagAlt = m_iUhrTag;
    m_iUhrMon = m_tSys.tm_mon+1;
    m_iUhrMonAlt = m_iUhrMon;
    m_iUhrYear = m_tSys.tm_year + 1900;
    m_iUhrYearAlt = m_iUhrYear;
    RechneSASU();
    ResetSimulation();
    aktUhr(true);
}

void CUhr::setSystemtime(time_t tmUhr)
{   
    char output[50];
    struct tm tmloc;
     
    localtime_r(&tmUhr, &tmloc);
    snprintf(output, 50, "timedatectl set-time \"%d-%02d-%2d %02d:%02d:%02d\"",
        tmloc.tm_year+1900, tmloc.tm_mon+1, tmloc.tm_mday, tmloc.tm_hour, tmloc.tm_min, tmloc.tm_sec); 
    int ret = system("timedatectl set-ntp 0");
    ret = system(output);
    m_uhrzeit = time(NULL);
    ret = system("timedatectl set-ntp 1");
}       

CUhr::~CUhr()
{
    pthread_mutex_destroy(&m_mutexUhr);
    delete m_pUhrAddr;
}

void CUhr::setTime(int stunde, int minute)
{
    pthread_mutex_lock(&m_mutexUhr);
    m_tSys.tm_hour = stunde;
    m_tSys.tm_min = minute;
    setTime();
}


void CUhr::setTime(int tag, int monat, int jahr)
{
    pthread_mutex_lock(&m_mutexUhr);
    m_tSys.tm_mday = tag;
    m_tSys.tm_mon = monat - 1;
    m_tSys.tm_year = jahr - 1900;
    setTime();
}
	
// Ausgehend von der Uhrzeit in m_tSys in lokaler Zeit, in das System übernehmen
// und in den RTC-Baustein speichern
void CUhr::setTime()
{
    time_t uhrzeit;
    struct tm *t;

    uhrzeit = mktime(&m_tSys); // lokale Uhrzeit in m_tSys in UTC umrechnen
    t = gmtime(&uhrzeit);      // time_t in struct tm schreiben
    setRTC(t);
    setSystemtime(uhrzeit);    
    m_uhrzeit = time(NULL);
    t = localtime(&m_uhrzeit);
    m_tSys = *t;
    pthread_mutex_unlock(&m_mutexUhr);
}

//
//  Der RTC-Baustein hat immer die absolute (UTC) Uhrzeit
//
void CUhr::getRTC(struct tm *t)
{
    m_pUhrAddr->setI2C_gpio ();

    t->tm_sec = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 0) & 0x7F);
    t->tm_min = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 1));
    t->tm_hour = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 2) & 0x3F);
    t->tm_wday = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 3));
    t->tm_mday = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 4));
    t->tm_mon = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 5)) - 1;
    t->tm_year = BCDinHex(BSC_ReadReg (1, ADDRCLOCKRAM, 6)) + 100;

}

void CUhr::setRTC(struct tm *t)
{
    int ret;

    m_pUhrAddr->setI2C_gpio();
    BSC_WriteReg(1, ADDRCLOCKRAM, 0x80, 0);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_min), 1);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_hour), 2);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_wday), 3);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_mday), 4);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_mon+1), 5);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_year-100), 6);
    BSC_WriteReg(1, ADDRCLOCKRAM, HexinBCD(t->tm_sec), 0);
}

int CUhr::BCDinHex (int val)
{
    return (val & 0x0F) + (val /16) * 10;
}

int CUhr::HexinBCD(int val)
{
    return (val /10 * 16) + val % 10;
}

int CUhr::getLetzteTag()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrTagAlt;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}

int CUhr::getLetzteMonat()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrMonAlt;
    pthread_mutex_unlock(&m_mutexUhr);    
    return iRet;
}
int CUhr::getMonat()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrMon;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}
int CUhr::getJahr()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrYear;
    pthread_mutex_unlock(&m_mutexUhr);    
    return iRet;
}
int CUhr::getTag()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrTag;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}

int CUhr::getLetzteJahr()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrYearAlt;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}
int CUhr::getWochenTag()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iUhrWochenTag;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}
int CUhr::getAnzahlTage(int year, int mon)
{
    int ret;

    switch(mon) {
    case 1:  // Januar
    case 3:  // März
    case 5:  // Mai
    case 7:  // Juli
    case 8:  // August
    case 10: // Oktober
    case 12: // Dezember
        ret = 31;
        break;
    case 4:  // April
    case 6:  // Juni
    case 9:  // September
    case 11: // November
        ret = 30;
        break;
    case 2:  // Februar
        if((year % 4) == 0)
            ret = 29;
        else
            ret = 28;
        break;
    default:
        ret = 0;
        break;
    }

    return  ret;
}
int CUhr::getSA()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iSA;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}
int CUhr::getSU()
{
    int iRet;
    pthread_mutex_lock(&m_mutexUhr);
    iRet = m_iSU;
    pthread_mutex_unlock(&m_mutexUhr);
    return iRet;
}

string CUhr::GetUhrzeitStr()
{
    char buf[MSGSIZE];
    
    string str;
    pthread_mutex_lock(&m_mutexUhr);
    strftime(buf, MSGSIZE, "%F %T", &m_tSys);
    pthread_mutex_unlock(&m_mutexUhr);
    str = buf;
    return str;
}

//
// Ablgeleitete Klasse von OperBase für die Uhrzeitberechnung
//
COperUhr::COperUhr()
{
    m_iFct = 0;
}

int COperUhr::resultInt()
{
    int iRet = 0;
    
    if(m_pUhr != NULL)
    {
        switch(m_iFct) {
            case 1: // in Minuten
                iRet = m_pUhr->getUhrmin();
                break;
            case 2:
                iRet = m_pUhr->getDatumTag();
                break;
            case 3: // Minutenwechsel
                iRet = m_pUhr->MinutenTakt();
                break;
            case 4: // Stundenwechsel
                iRet = m_pUhr->StundenTakt();
                break;
            case 5: // Tagwechsel
                iRet = m_pUhr->TagesTakt();
                break;
            case 6: // Wochenwechsel
                iRet = m_pUhr->WochenTakt();
                break;
            case 7: // Monatswechsel
                iRet = m_pUhr->MonatsTakt();
                break;
            case 8: // Jahreswechsel
                iRet = m_pUhr->JahresTakt();
                break;
            default:
                break;
        }
    }
    return iRet;
}

string COperUhr::resultString()
{
    string str;
    
    if(m_pUhr != NULL)
    {
        switch(m_iFct) {
            case 1:
                str = m_pUhr->GetUhrzeitStr();
                break;
            case 2:
                str = to_string(m_pUhr->getDatumTag());
                break;
            default:
                str.clear();
                break;
        }
    }
    else
        str.clear();
    return str;
}

void COperUhr::setOper(int iFct)
{
    m_iFct = iFct;
}