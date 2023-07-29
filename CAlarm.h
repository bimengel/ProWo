/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CAlarm.h
 * Author: josefengel
 *
 *  07.02.22
 */

#ifndef CALARM_H
#define CALARM_H

class CAlarmSensor {
public:
    CAlarmSensor();
    void SetSensorBez(string strSensor, string strBez, int iMust);
    void SetBerechne(CBerechneBase * pBerechne);    
    string GetSensor();
    string GetBez();
    void Berechne();
    bool GetState();
    bool GetLastState();
    bool GetMust();
    bool ChangeState();
    
protected:
    string m_strSensor;
    string m_strBez;
    CBerechneBase * m_pBerechne;
    bool m_bState;
    bool m_bMust;
    bool m_bLastState;
};

class CAlarme {

public:
    CAlarme();
    void SetBez(int iAnz, string strBez);
    string GetBez();
    void SetSensorNr(int iIdx, int iNr);
    int GetAnzSensoren();
    int GetSensorNr(int iIdx);
    void SetStateSensor(bool bState, bool bMust, int iAusloeser);
    void SetDelay(int iDelayAktiv, int iDelayAlarm);
    int GetDelayAktiv();
    int GetDelayAlarm();
    bool GetStateSensor();
    bool GetStateSensorMust();
    int GetAusloeser();
    
protected:
    string m_strBez;
    int* m_piSensor;
    int m_iAnzSensoren;
    CBerechneBase * m_pBerechne;
    bool m_bStateSensor;
    bool m_bStateSensorMust;
    int m_iDelayAktiv;
    int m_iDelayAlarm;
    int m_iAusloeser;           // Sensor der den Alarm ausgelöst hat
};

class CAlarm {
public:
    CAlarm(int iAnzSensor, int iAnzAlarme);
    virtual ~CAlarm();
    void SetSensorBez(int iSensor, string strSensor, string strBez, int iMust);
    void SetAlarmeBez(int iIdx, int iAnz, string strBez);
    bool SetAlarmeSensor(int iAlarm, int iIdx, int iNr);
    int GetAnzAlarme();
    void SetBerechneSensor(int iIdx, CBerechneBase *pBerechne);
    void Berechne(int iTime);
    bool BerechneTimer(int iTime);
    void SetDelay(int iAlarm, int iDelayAktiv, int iDelayAlarm);
    int GetParamInt(int fct);
    string GetParamString(int fct);
    int GetAlarmNr();
    CAlarme * GetAlarmeAddr(int iIdx);
    CAlarmSensor * GetSensorAddr();
    bool GetEnabled(int iNr);
    void SetEnabled(int iNr, bool bValue);
    void SetAlarm(int iAlarmNr);
    void SetPwd(string strPwd);
    string GetPwd();
    void SetTechPwd(string strPwd);
    string GetTechPwd();
    void Control();
    void ResetEnabled();
    
private:
    void BerechneAlarme();
    void SetState(int iState, string str);
    int m_iAnzSensoren;             // Anzahl Sensoren
    int m_iAnzAlarme;               // Anzahl Alarme
    CAlarmSensor *m_pAlarmSensor;
    CAlarme * m_pAlarme;
    bool * m_pbEnabled;   // Sensoren zu deaktivieren
    int m_iState;           // =0 ausgeschaltet
                            // =1 (sabotage) eingeschaltet, Sabotage wird überwacht
                            // =2 (voraktiv) ist aktiviert aber noch nicht scharf geschaltet 
                            // =3 (aktiv) scharf geschaltet
                            // =4 (voralarm) Alarm steht an aber warten auf DelayAlarm
                            // =5 (alarm) Alarm ist alamiert
    int m_iLastState;       // für einen Wechsel des Status zu ermitteln
    int m_iAlarm;           // Nummer des Alarms der aktiviert und geschaltet ist
    int m_iDelayStart;      // 
    int m_iAlarmStartStop;  // Signal von Aussen, beim Starten Nummer des Alarms
                            // oder wenn verschieden von 0 zum stoppen des Alarms
    string m_strPwd;
    string m_strTechPwd;
    queue<string> m_LogFifo;
    pthread_mutex_t m_mutexAlarmFifo;	    
};

class COperAlarm : public COperBase
{
public:
    COperAlarm();
    virtual int resultInt();
    virtual string resultString();
    void SetOper(CAlarm *pAlarm, int fct);
    
protected:
    CAlarm *m_pAlarm;
    int m_iFct;
};

class CBerechneAlarm : public CBerechneBase
{
public:
    void init(CAlarm *pAlarm, CHistory *pHistory);
    virtual void SetState(int state);
private:
    CAlarm * m_pAlarm;
    CHistory * m_pHistory;
};

#endif /* CALARM_H */

