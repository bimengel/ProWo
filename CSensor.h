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
    int GetHumidity();
    int GetVocSignal();
    int GetTyp();
    void SetName(string strName);
    void SetKorrTemp(int iKorrTemp);
    void SetKorrHumidity(int iKorrHumidity);
    string GetName();
    short m_sStatTemp[ANZSTATSENSOR];
    short m_sStatHumidity[ANZSTATSENSOR];
    short m_sStatVocSignal[ANZSTATSENSOR];
	
protected:
    int m_iTyp; // 1=TQS3, 2=TH1
    int m_iNummer; 
	int m_iTemp;
    int m_iKorrTemp;
	int m_iHumidity;
    int m_iKorrHumidity;
    int m_iVocSignal;
    string m_strName;
    
    void PlaceInStat();
};

class CSensorModBus : public CSensor
{
public:
	CSensorModBus(int nr);
	void SetModBusClient(CModBusClient * pModBusClient);
	virtual int LesenStarten() { return 0;};
    virtual void SetFunction(int func) {};
    
protected:
	int m_iFunction; 
	CModBusClient * m_pModBusClient;
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

#endif // _CSENSOR_H_
