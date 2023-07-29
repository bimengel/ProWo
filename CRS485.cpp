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

/*
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "CRS485.h"
#include "gpiolib.h"
*/

#include "ProWo.h"

extern struct bcm2835_peripheral spi1;
extern struct bcm2835_peripheral uart;

//*****************************************************************
//
// Methoden der Oberklasse CRS485ob 
//
//*****************************************************************

bool CRS485ob::FinishSend()
{
    int reg;
    reg = *(uart.addr + FR);
    if(reg & 0x08)
        return false;
    else
    {
        if(m_bWrite)
        {
            m_iWait = (int)(10.0f/((float)m_iBaud * (float)m_iZykluszeit * 0.000001f));
            m_bWrite = false;
        }

        m_iWait--;
        if(m_iWait < 0)
            return true;
        else
            return false;
    }
}	

bool CRS485ob::CanRead()
{
    int reg;
    reg = *(uart.addr + LCRH);
    reg = *(uart.addr + FR);
    if(reg & 0x10)
        return false;
    else
        return true;
}

unsigned char CRS485ob::Read()
{
    int reg, i;
    reg = *(uart.addr + DR);
    i = *(uart.addr + RSRECR);
    i &= 0xF;
    if(i)
        i = 0;
    return (unsigned char) reg;
}

int CRS485ob::Write(unsigned char ch)
{
    *(uart.addr + DR) = (int)ch;
    return (int)ch;
}

bool CRS485ob::CanSend()
{
    if(*(uart.addr + FR) & 0x20)
        return false;
    else
        return true;
}

void CRS485ob::SetRTS(bool bState)
{
    int reg;

    reg = *(uart.addr + CR);
    reg &= 0xFCFE;
    *(uart.addr + CR) = reg;
    if(bState)
    {
        switch_gpio (17, 1);
        reg |= 0x101; // Transmit enable
    }
    else
    {
        switch_gpio (17, 0);
        reg |= 0x201; //Receive enable
    }
    *(uart.addr + CR) = reg;

    CRS485::SetRTS(bState);
}

int CRS485ob::Init(int baud, int bits, int stops, char parity, int zykluszeit)
{
    float integer, fractional;
    int inh, ret = 0;

    // UART Disabled
    *(uart.addr + CR) = 0; 

    inh = ControlBaudrate (baud);
    if(!inh)
        ret = 4;

    // baudrate setzen
    integer = 3000000.0f / (16 * baud);
    inh = (int ) integer;
    *(uart.addr + IBRD) = inh;
    fractional = (integer - (float)inh)*64 + 0.5;
    inh = (int)fractional;
    *(uart.addr + FBRD) = inh;

    // Bits und parität
    switch(bits) {
    case 8:
        inh = 0x60;
        break;
    case 7:
        inh = 0x40;
        break;
    default:
        ret = 1;
        break;
    };

    // Anzahl Stop bits
    if(stops == 2)
        inh |= 0x08;
    else if(stops == 1)
        inh = inh;
    else 
        ret = 2;

    // Parity
    parity = toupper(parity);
    switch(parity) {
    case 'E':
        inh |= 0x06;
        break;
    case 'O':
        inh |= 0x02;
        break;
    case 'N':
        break;
    default:
        ret = 3;
        break;
    }
    if(ret == 0)
    {
        inh |= 0x10; // FIFO enable
        *(uart.addr + LCRH) = inh;
        // bit 0 uart enable
        *(uart.addr + CR) = 0x301;

        Reset();

        CRS485::Init(baud, bits, stops, parity, zykluszeit);
        SetRTS(false);
    }
    return ret;
}


//*****************************************************************
//
// Methoden der Oberklasse CRS485spi 
//
//*****************************************************************

bool CRS485spi::FinishSend ()
{
    int ret;

    ret= read_config ();
    if(ret & 0x4000)
    {
        if(m_bWrite)
        {
            m_iWait = (int)(10.0f/((float)m_iBaud * (float)m_iZykluszeit * 0.000001f));
            m_bWrite = false;
        }

        m_iWait--;
        if(m_iWait <= 0)
            return true;
        else
            return false;
    }
    else
        return false;

}

//
// Kann gesendet werden, ist Platz im FIFO-Register ?
//
bool CRS485spi::CanSend()
{   
    int ret;

    ret= read_config ();
    if(ret & 0x4000)
        return true;
    else
        return false;
}

bool CRS485spi::CanRead()
{
    int ret;

    spiset_ChipSelect(SPI_CS_CS1);
    ret = write_det(0x00, 0x00);
    m_cRead = (unsigned char)(ret % 256);
    if(ret & 0x8000)
        return true;
    else
        return false;
}

unsigned char CRS485spi::Read()
{
    return m_cRead;
}


void CRS485spi::SetRTS(bool bState)
{

    spiset_ChipSelect(SPI_CS_CS1);
    if(bState)
        // RTS auf 0 --> Transmit Enable, Receive Disable
        write_det(0x84, 0x00);
    else
        // RTS auf 1 --> Transmit Disable, Receive Enable
        write_det (0x86, 0x00);

    CRS485::SetRTS(bState);
}


int CRS485spi::Init(int baudrate, int bits, int stops, char parity, int zykluszeit)
{
    // 38400 baud, 8 bits, no parity, 1 stop bit 
    int inh, ret = 0;

    inh = ControlBaudrate (baudrate);
    if(!inh)
        ret = 4;

    // Stop bits
    if(stops == 2)
        inh |= 0x40;
    else if(stops == 1)
        inh = inh;
    else
        ret = 2;

    // Bits
    if(bits == 7)
        inh |= 0x10;
    else if(bits == 8)
        inh = inh;
    else
        ret = 1;

    // Parität senden wir mit PE gesendet und muss vom Programm errechnet
    // und beim Senden in Pt abgelegt werden
    // ist nicht implementiert, könnte aber erweitert werden !!!! JEN 02.04.14
    if(parity != 'N')
        ret = 3;

    if(ret == 0)
    {
        spiset_ChipSelect(SPI_CS_CS1);
        // bit 13=0, FIFO enabled
        // bit 9=0 Parität wird nicht gesendet !!
        write_det(0xC0, inh);
        Reset();
        CRS485::Init(baudrate, bits, stops, parity, zykluszeit);
        SetRTS(false);
    }
    return ret;
}	

int CRS485spi::Write(unsigned char data)
{
    int ret;
    
    ret = write_det(0x80, data);
    return ret;
}

int CRS485spi::write_det(int cmd, int data)
{
    int i, j, ret=0;

    spi_setTA(1);
    *(spi1.addr + SPI_FIFO) = cmd;
    *(spi1.addr + SPI_FIFO) = data;
    spi_wait_finish();
    spi_setTA(0);
    for(i=0,j=0; i < 100 && j < 2; i++)
    {
        if(*(spi1.addr + SPI_CS) & SPI_CS_RXD)
        {  
            ret *= 256;
            ret += *(spi1.addr + SPI_FIFO);
            j++;
        }
        else
            delayMicroseconds (10);
    }
    return ret;
}

int CRS485spi::read_config()
{
    int i, j, ret, cs;

    spiset_ChipSelect(SPI_CS_CS1);
    return write_det(0x40, 0x00);
}

//*****************************************************************
//
// Methoden der Oberklasse CRS485 
//
//*****************************************************************
int CRS485::ControlBaudrate(int baudrate)
{
    int inh = 0;

    switch(baudrate) {
    case 115200:
        inh = 0x01;
        break;
    case 57600:
        inh = 0x02;
        break;
    case 38400:
        inh = 0x09;
        break;
    case 19200:
        inh = 0x0A;
        break;
    case 9600:
        inh = 0x0B;
        break;
    case 4800:
        inh = 0x0C;
        break;
    case 2400:
        inh = 0x0D;
        break;
    case 1200:
        inh = 0x0E;
        break;
    default:
        break;
    }

    return inh;
}

CRS485::CRS485()
{
    m_iBaud = 0;
    m_iBits = 0;
    m_iStops = 0;
    m_cParity = ' ';
    m_iState = NOTDEFINED;
    m_cEmpfPtr = NULL;
}

int CRS485::Control()
{
    bool bOK = true;
    char ptr[100];

    switch(m_iState) {
    case NOTDEFINED:
        break;
    case IDLE:
        m_iState = 1;
        break;
    case SENDING:
        SetRTS(true);
        while(bOK)
        {
            if(CanSend())
            {
                if(m_iSendLen > 0)
                {   m_bWrite = true;
                    Write(*m_cSendPtr);
                    m_cSendPtr++;
                    m_iSendLen--;
                }
                else
                {   if(m_iEmpfLen)
                        m_iState = FINISHSENDING;
                    else				
                        m_iState = IDLE;
                    bOK = false;
                }
            }
            else
                bOK = false;
        }
        break;
    case FINISHSENDING: // warten bis Senden beendet
        if(FinishSend())
        {   int i;

            m_iAnzEmpf = 0;
            m_iTimeOut = 0;
            SetRTS(false); // Empfang frei schalten
            m_iState = RECEIVING;
        }
        break;
    case RECEIVING: 
        while(bOK)
        {
            if(CanRead())
            {
                m_iTimeOut = 0;
                *(m_cEmpfPtr + m_iAnzEmpf) = Read();
                m_iAnzEmpf++;

                if(m_iAnzEmpf == m_iEmpfLen)
                {	
                    m_iState = RECEIVED;
                    bOK = false;
                }
            }
            else
            {
                m_iTimeOut++;
                // JEN 15.06.18 if(m_iTimeOut >= 1000000/m_iZykluszeit)
                if(m_iTimeOut >= 100000/m_iZykluszeit) // 100 msek
                {   
                    sprintf(ptr, "Timeout Error Adresse=%d, Empf_Soll=%d, Empfangen=%d\n", *m_cStartSendPtr, m_iEmpfLen, m_iAnzEmpf);
                    syslog(LOG_ERR, ptr);
                    m_iError = ERRTIMEOUT;
                    m_iState = RECEIVED;
                }
                bOK = false;
            }
        }
        break;
    default:
        break;
    }

    return m_iState;
}

void CRS485::SetRTS(bool state)
{
    int i;
    if(state == false) // es wird auf Empfang geschaltet
    {	// Empfangs-FIFO-Register leeren   
        for(i=0; i < 20 && CanRead(); i++)
            Read();
        if(i==20)   // Überlauf des FIFO Registers
            m_iError = ERROVERRUN;
    }
}

void CRS485::SendLen(unsigned char *ptrSend, unsigned char *ptrEmpf, int sendLen, int empfLen)
{
    m_cSendPtr = ptrSend;
    m_cStartSendPtr = ptrSend;
    m_cEmpfPtr = ptrEmpf;
    m_iSendLen = sendLen + 2;
    m_iEmpfLen = empfLen + 3 + 2; 
    // 3 Anfangstelegramm (Adresse, Funktion (+0x80 =ERROR), Anzahl bytes, 2 CRC am Ende
    m_iState = SENDING;
    m_iError = NOERROR;
}

int CRS485::Init(int baud, int bits, int stops, char parity, int zykluszeit)
{
    m_iBaud = baud;
    m_iBits = bits;
    m_iStops = stops;
    m_cParity = parity;
    m_iState = IDLE;
    m_iError = NOERROR;
    m_iZykluszeit = zykluszeit;
    return 0;
}

void CRS485::Reset()
{
    m_iState = IDLE;
    m_iError = NOERROR;
}

int CRS485::GetState()
{
    return m_iState;
}
void CRS485::SetState(int state)
{
    m_iState = state;
}


int CRS485::GetError()
{
    return m_iError;
}