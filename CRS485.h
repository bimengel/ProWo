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

#ifndef _CRS485_H_
#define _CRS485_H_

#define DR		 0	// Data Register
#define RSRECR	 1  // Receive Status / error clear register
#define FR		 6	// Flag Register
#define IBRD	 9	// Integer baud rate divisor
#define FBRD	10	// Fractionnal baud rate divisor
#define LCRH	11	// Line Control Register
#define CR		12	// Control Register

// State RS485
#define NOTDEFINED		0
#define IDLE			1
#define SENDING			2
#define FINISHSENDING   3
#define RECEIVING		4
#define RECEIVED		5

//Errors RS485
#define NOERROR			0
#define ERRTIMEOUT		1
#define ERROVERRUN		2

class CRS485 
{
public:
	CRS485();
	virtual int Init(int baudrate, int bits, int stops, char parity, int zykluszeit); 
	void SendLen(unsigned char *ptrSend, unsigned char *ptrEmpf, int lenSend, int lenAnswer);
	int Control();
	void Reset();
	int GetState();
	void SetState(int state);
	int GetError();

protected:
	virtual void SetRTS(bool bState);
	virtual bool CanSend() {return true; };
	virtual bool CanRead() {return true; };
	virtual int Write(unsigned char ch) {return 0; };
	virtual unsigned char Read() { return ' ';};
	virtual bool FinishSend() { return true; };
	int ControlBaudrate(int baud);
	
	unsigned char *m_cEmpfPtr;
	int m_iState;
	unsigned char *m_cSendPtr;
	unsigned char *m_cStartSendPtr;
	int m_iTimeOut;
	int m_iSendLen;
	int m_iEmpfLen;
	int m_iAnzEmpf;
	int m_iBaud;
	int m_iBits;
	int m_iStops;
	char m_cParity;
	int m_iError;
	int m_iZykluszeit;

	bool m_bWrite;			// es ist gesendet worten, in FinishSend Sendezeit berückichtigen
	int m_iWait;			// Warten bis letzte bytes gesendet
	
};

class CRS485ob : public CRS485
{
public:
	virtual int Init(int baudrate, int bits, int stops, char parity, int zykluszeit);

protected:
	virtual void SetRTS(bool bState);
	virtual bool CanSend();
	virtual int Write(unsigned char ch);
	virtual bool CanRead();
	virtual unsigned char Read();
	virtual bool FinishSend();

};

class CRS485spi : public CRS485
{
public:
	virtual int Init(int baudrate, int bits, int stops, char parity, int zykluszeit);
	
protected:
	virtual void SetRTS(bool bState);
	virtual bool CanSend();
	virtual bool CanRead();
	virtual int Write(unsigned char ch);
	virtual bool FinishSend();
	virtual unsigned char Read();
	int write_det(int cmd, int data);
	int read_config();

	unsigned char m_cRead;
};

#endif // _CRS485_H_
