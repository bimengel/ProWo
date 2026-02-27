/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2015 <root@ProWo2>
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

#ifndef _CSENSOR_H_
#define _CSENSOR_H_

#define ANZSTATSENSOR 1440

class CSensor
{
public:
	CSensor(int nr);
	~CSensor();
	virtual int LesenStarten() {return 0;};
	int GetTemp();
    int GetParam2();
    int GetVocSignal();
    int GetTyp();
    void SetName(string strName);
    void SetKorrTemp(int iKorrTemp);
    void SetKorrParam2(int iKorrParam2);
    string GetName();
    short m_sStatTemp[ANZSTATSENSOR];
    short m_sStatParam2[ANZSTATSENSOR];
    short m_sStatVocSignal[ANZSTATSENSOR];
    int GetError();
    string GetStrError();
	
protected:
    int m_iTyp; // 1=TQS3, 2=TH1 // 3=ULC (Ultrasonic Level Sensor)
    int m_iNummer; 
	int m_iTemp;
    int m_iKorrTemp;
	int m_iParam2;
    int m_iKorrParam2;
    int m_iVocSignal;
    int m_iError;
    string m_strName;
    
    void PlaceInStat();
};

class CSensorModBus : public CSensor
{
public:
	CSensorModBus(int nr);
	void SetModBusClient(CModBusRTUClient * pModBusClient);
	virtual int LesenStarten() { return 0;};
    virtual void SetFunction(int func) {};
    
protected:
	int m_iFunction; 
	CModBusRTUClient * m_pModBusRTUClient;
};

class CTQS3 : public CSensorModBus
{
public:
    CTQS3(int nr);
    virtual ~CTQS3();
	virtual void SetFunction(int func);
	virtual int LesenStarten();    

private:

};

class CUltrasonicLevelSensor : public CSensorModBus // Ultrasonic Level Sensor
{   
public:
    CUltrasonicLevelSensor(int nr);
    virtual ~CUltrasonicLevelSensor();
    virtual void SetFunction(int func);
    virtual int LesenStarten(); 
};

#endif // _CSENSOR_H_
