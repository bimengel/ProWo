/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTH1.h
 * Author: josefengel
 *
 * Created on July 8, 2021, 11:35 AM
 */

#ifndef CTH1_H
#define CTH1_H

class CTH1 : public CSensorModBus
{
public:
    CTH1(int nr);
    virtual ~CTH1();
	virtual void SetFunction(int func, int iLED, int iI2CTakt);
 	virtual int LesenStarten(); 
    void VocIndexDriver(unsigned int iVocRawSignal);
    
private:
    int m_iVocRawSignal;
    struct VocAlgorithmParams  m_VocAlgorithmParams;
};

#endif /* CTH1_H */

