/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CAlarmClock.h
 * Author: josefengel
 *
 * Created on January 16, 2022, 11:31 AM
 */

#ifndef CALARMCLOCK_H
#define CALARMCLOCK_H

class CAlarmClockEntity 
{
public:
    CAlarmClockEntity();

    bool m_bActivated;
    string m_strName;
    short m_sRepeat;
    bool m_bSnooze;  // Schlummern
    int m_iTimeMin;  
    int m_iMethod;     
};

class CAlarmClock {
public:
    CAlarmClock();
    virtual ~CAlarmClock();
    void LesDatei(); 
    void SchreibDatei();
    void Control(int iTimeMin, int iTag);
    string GetStringAlarm(int iIdx);
    string GetStringMethod(int iIdx);
    int GetAnzAlarm();
    int GetAnzMethod();
    int GetAktivMethod();
    int GetStep();
    int GetState();
    int GetChange();
    void DeleteParam(int iNr);
    void SetParam(int iNr, CAlarmClockEntity *palarmclockEntity);
    void SetActivated(int iIdx, bool bActivated);
    void SetState(int iState);

private:
    vector<CAlarmClockEntity *> m_vectAlarmClockEntity;
    vector<string>m_vectMethod;
    int m_iLastTimeMin;
    int m_iNrClockAktiv;
    int m_iStep; // aktiv seit x Minuten
    int m_iSnooze;
    int m_iChange;
};

class CBerechneAC : public CBerechneBase {

public:
    void init(CAlarmClock *pAlarmClock);
    virtual void SetState(int state);
private:
    CAlarmClock * m_pAlarmClock;   
};

//
// Wecker - Alarmclock
//
class COperAC : public COperBase
{
public:
    COperAC();
    virtual int resultInt();
    void setOper(CAlarmClock *ptr, int iFct);

protected:
    CAlarmClock * m_pAlarmClock;
    int m_iFct;
};

#endif /* CALARMCLOCK_H */

