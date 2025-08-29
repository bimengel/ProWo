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

#ifndef _CUHR_H_
#define _CUHR_H_

#include <pthread.h>

class CUhr
{
public:
    CUhr();
    ~CUhr();
    void aktUhr(bool bWriteRTC); 
    void setRTC(struct tm *t);
    void getRTC(struct tm *t);
    void RechneSASU();
    int getTagNacht();
    bool SekundenTakt();
    bool MinutenTakt();
    bool StundenTakt();
    bool TagesTakt();
    bool WochenTakt();
    bool MonatsTakt();
    bool JahresTakt();
    int getLetzteTag();
    int getLetzteMonat();
    int getTag();
    int getMonat();
    int getJahr();
    int getLetzteJahr();
    time_t getUhrzeit();
    void getUhrzeitStruct(struct tm *t);
    void getUhrzeitStruct(struct tm *t, time_t uhrzeit);
    void getDateTag(struct tm * ptm, int iOffset);
    int getUhrmin();
    int getDatumTag();
    int getDatumTag(int Jahr, int Monat, int Tag);
    void getDatumTag(int datumTag, int *Jahr, int *Monat, int *Tag);
    int getWochenTag();
    void setTime();
    void setTime(int stunde, int minute); // in System und RTC von der Struktur in lokaler Zeit
    void setTime(int tag, int monat, int jahr);
    void SetSimTagNacht(int state);
    void SetSimUhrzeit(int min);
    void ResetSimulation();
    int getAnzahlTage(int iJahr, int iMonat);
    int getSU();
    int getSA();
    CBoardAddr * m_pUhrAddr;
    string GetUhrzeitStr();
    
protected:
    int BCDinHex(int val);
    int HexinBCD(int val);
    void setSystemtime(time_t tmuhrzeit);
    time_t m_uhrzeit;
    struct tm m_tSys;
    int m_iAktTag;          // wird gebraucht für den Tagwechsel
    int m_iUhrSek;          // für Erstellung Sekundentakt
    int m_iUhrSekAlt;
    int m_iUhrMin;          // aktuelle Uhrzeit in Minuten
    int m_iUhrMinAlt;
    int m_iUhrStunde;       // aktuelle Uhrzeit in Stunden
    int m_iUhrStundeAlt;
    int m_iUhrTag;	    // aktueller Tag
    int m_iUhrTagAlt;
    int m_iUhrWochenTag;     // 0 - 6 Tage nach dem Sonntag
    int m_iUhrWochenTagAlt;
    int m_iUhrMon;          // aktueller Monat
    int m_iUhrMonAlt;
    int m_iUhrYear;         // aktuelles Jahr
    int m_iUhrYearAlt;
    int m_iSA;              // Sonnenaufgang in Minuten 
    int m_iSU;              // Sonnenuntergang in Minuten
    // Simulation
    bool m_bSimUhrzeit;
    int m_iSimTagNacht;
    int m_iSimUhrzeit;

    pthread_mutex_t m_mutexUhr;
	
private:

};

class COperUhr : public COperBase
{
public:
    COperUhr();
    virtual int resultInt();
    virtual string resultString();
    void setOper(int iFct);
protected:
    int m_iFct;
};

#endif // _CUHR_H_
