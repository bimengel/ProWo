/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2014 <root@ProWo2>
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

#ifndef _CPHPSOCKET_H_
#define _CPHPSOCKET_H_
#define BUFSIZE 1024
#define PERM "0666"
#define SOCK_PATH "/var/run/prowo.sock"
#define LISTEN_BACKLOG 50

#define PORTNR 3001

class CBrowserSocket
{
public:
    int Init(CIOGroup *pIOGroup);
    void Run();
    CBrowserSocket();
    ~CBrowserSocket();
 
protected:
    int m_serverSocket;
    int m_acceptSocket;
    struct sockaddr_in m_serverAddr;
    struct sockaddr_in m_clientAddr;
    socklen_t m_clientAddrSize;
    char m_recvdMsg[BUFSIZE];
    mode_t m_mode;

    void Send(const char *ptr, int len);
    void Send(const char *ptr);
    int ReadNumber();
    int ReadBuf(char *buf, char ch);
    double ReadDouble();
    string GetMonthText(int iVal);  
    void VerwaltSteuerung(int iNiv1, int iNiv2, int iNiv3, int iNiv4);
    void VerwaltHeizung(int iNiv1, int iNiv2, int iNiv3, int iNiv4);
    void VerwaltZaehler(int iNiv1, int iNiv2, int iNiv3, int iNiv4);
    void VerwaltAlarm(int iNiv1, int iNiv2, int iNiv3, int iNiv4);
    void VerwaltHome(int iNiv1, int iNiv2, int iNiv3, int iNiv4);
    void VerwaltWS(int iNiv1, int iNiv2, int iNiv3, int iNiv4);
    void VerwaltSensor(int iNiv1, int iNiv2, int iNiv3, int iNiv4); 
    void VerwaltAlarmClock(int iNiv1, int iNiv2, int iNiv3, int iNiv4);   

private:
    int m_iPos;
    CIOGroup * m_pIOGroup;
};

#endif // _CPHPSOCKET_H_
