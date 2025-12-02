/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) 2013 JEN <pi@raspberrypi>
 * 
 * GPIO is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * GPIO is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "ProWo.h"

void *ThreadNodejsProc(void *);
void *ThreadModbusProc(void *);
void *ThreadProc(void *);

bool bThreadNodejs = false;
bool bThreadModbus = false;
bool bThreadProc = false;
pthread_mutex_t ext_mutexNodejs;	

static void signalhandler(int sig);
void daemonShutdown();
int daemonize();

int pidFilehandle;
bool bRunProgram;

static char pidfile[] = "/var/run/prowo.pid";
char *pProgramPath = NULL;
CIOGroup *iogrp = NULL;
 
CBrowserSocket *pBrowserSocket = NULL;
CUhr * m_pUhr = NULL;
int ext_iI2CSwitchType = 0; // 1 = 9544, 2 = 9545
int ext_iMaxZykluszeit = 0;
CKeyBoard *pKeyBoard = NULL;

int main(int argc, char *argv[])
{   
    // Meldungen in syslog (/var/log) schreiben
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog("ProWo", LOG_CONS | LOG_PERROR, LOG_USER);
    syslog(LOG_INFO, "starting up");

    int button, j;
    CLCDDisplay *pLCDDisplay = NULL;
    pthread_t commThreadNodejs;
    pthread_t commThreadModbus;
    pthread_t commThread;
    bool bFirst = true;

    struct timespec start, finish; 
    long lDelay;
    string str;
    
// #ifdef DEBUG 
    static char path[] = "/home/pi/";
    j = strlen(path);
    pProgramPath = new char[j+1];
    strcpy(pProgramPath, path); 
//#else 
/*   j = strlen(argv[0]);
    pProgramPath = new char[j+1];
    strcpy(pProgramPath, argv[0]);
    for( ; j > 0; j--)
    {
        if(pProgramPath[j-1] == '/')
        {
            if(bFirst) 
                bFirst = false;
            else
            {
                pProgramPath[j] = 0;
                break;
            }
        }
    } */
    daemonize();
    
    FILE * pFile;
    char text[100];
    int iRaspberry;
    pFile = fopen("/sys/firmware/devicetree/base/model", "r");
    if(pFile == NULL)
    {
        syslog(LOG_ERR, "cannot open /sys/firmware/devicetree/base/modul");
        exit(-1);
    }
    fgets(text, 99, pFile);
    fclose(pFile);

    if(strncmp(text, "Raspberry Pi 3 Model B", 22) == 0)
        iRaspberry = 3;
    else
        iRaspberry = 2;
    init_gpio(1, iRaspberry);
    LCD_Init();
    bRunProgram = true;
    pKeyBoard = new CKeyBoard;
    
    iogrp = new CIOGroup;
    try 
    {
        iogrp->InitGroup ();
    }
    catch(int error)
    {
        pLCDDisplay = new CLCDDisplay;
        pLCDDisplay->GetIPAddress(text, (char *)"eth0");
        LCD_AnzeigeZeile1 (text);
        delete pLCDDisplay;
        delete iogrp;
        delete[] pProgramPath;
        delete m_pUhr;
        daemonShutdown();
        return 1;
    }
    
    if(iogrp->m_pHue != NULL || iogrp->m_pSomfy != NULL)
        curl_global_init(CURL_GLOBAL_DEFAULT); // or ALL ????
    pLCDDisplay = new CLCDDisplay;
    pLCDDisplay->InitMenu(iogrp, argv[0]);

    // GPIO18 INT - Request vom PCA9544
    setmode_gpio(18, INPUT);
    setpullUpDown_gpio(18, PUD_UP);
    // GPIO22 Button Pfeil nach oben
    setmode_gpio(22, INPUT);
    setpullUpDown_gpio(22, PUD_UP);
    // GPIO23 Button Pfeil nach unten
    setmode_gpio(23, INPUT);
    setpullUpDown_gpio(23, PUD_UP);
    // GPIO24 Button OK
    setmode_gpio(24, INPUT);
    setpullUpDown_gpio(24, PUD_UP);
    // GPIO25 Button Abbruch
    setmode_gpio(25, INPUT);
    setpullUpDown_gpio(25, PUD_UP);
    // GPIO27 aus Ausgang definieren
    setmode_gpio (27, OUTPUT);

    str.clear();
    if(pthread_create(&commThreadNodejs, NULL, &ThreadNodejsProc, NULL) < 0)
        str = "Error open Socket Thread (Nodejs, GSM)!!\n";

    if(pthread_create(&commThreadModbus, NULL, &ThreadModbusProc, NULL) < 0)
        str = "Error open Socket Thread (Modbus)!!\n";
    
    // Speichern in Dateien, HUE, Musik
    if(pthread_create(&commThread, NULL, &ThreadProc, NULL) < 0)
        str = "Error open Socket Thread (Speichern Dateien, HUE, Musik)!!\n";
    
    if(!str.empty())
    {
        syslog(LOG_ERR, str.c_str());
        LCD_AnzeigeZeile1((char *)str.c_str());
        return 1;
    }
        
    iogrp->Control (true);
       
    while(bRunProgram)
    {   
        clock_gettime(CLOCK_REALTIME, &start); 

        // Tastatur abfragen
        button = pKeyBoard->getch();
        if(button == BUTTONUPDOWN)
            bRunProgram = false;
        if(button == BUTTONOKCANCEL)
        {	
            LCD_AnzeigeZeile1 ((char *)"ProWo reboot");
            system("reboot");
        }
        // Menüverwaltung
        pLCDDisplay->VerwaltMenu(button);
        iogrp->Control(false);
        delayMicroseconds(ZYKLUSZEIT);
        clock_gettime(CLOCK_REALTIME, &finish);
        lDelay = (finish.tv_nsec - start.tv_nsec)/1000; // nanosekunden in microsekunden 
        if(lDelay > ext_iMaxZykluszeit)
            ext_iMaxZykluszeit = lDelay;

    }

    LCD_Clear ();
    LCD_AnzeigeZeile1 ((char *)"Program stopped");
    syslog(LOG_INFO, "Program stopped");

    // Kummunikation Thread mit Nodejs stoppen
    while(bThreadNodejs || bThreadModbus || bThreadProc)
    {
        if(bThreadNodejs)
            LCD_AnzeigeZeile3 ((char *)"Thread Nodejs!");
        if(bThreadModbus)
            LCD_AnzeigeZeile3((char *)"Thread Modbus!");
        if(bThreadProc)
            LCD_AnzeigeZeile3((char *)"Thread Proc!");
         delayMicroseconds(ZYKLUSZEIT);       
    }
    LCD_AnzeigeZeile3((char *) "");
    // Thread Nodejs stoppen
    j = pthread_cancel(commThreadNodejs);
    // Thread Modbus stoppen
    j = pthread_cancel(commThreadModbus);
    // Speichern in Dateien, HUE, Musik
    j = pthread_cancel(commThread);    
  
    // gpio zurücksetzen
    init_gpio(0, 0);
    
    delete pLCDDisplay;
    delete iogrp;
    delete pKeyBoard;
    delete[] pProgramPath;
    delete m_pUhr;

    daemonShutdown();
    
    return 0;
}

int daemonize()
{
    pid_t pid;
    int i;
    char str[20];

    // Create a new process
/*    pid = fork();
    if(pid < 0)
    {   
        syslog(LOG_ERR, "can't create process");
        exit(EXIT_FAILURE);
    }

    if(pid > 0)
    {
        // Elternprozess beenden
        exit(EXIT_SUCCESS);
    }

    // Kindprozess wenn pid = 0
    // Create new session and process group
    if(setsid() == -1)
        return 0;

    // Change file mode mask
    umask(027);

    // Set the working directory to the root directory
    if(chdir("/") == -1)
        return 0;

    // close all open files
    for(i=0; i < NR_OPEN; i++)
        close(i);
*/
    // redirect fd's 0,1,2 to /dev/null
    open("/dev/null", O_RDWR);  // 0 = stdin
    dup(0);						// 1 = stdout
    dup(0);						// 2 = stderr

    if(signal(SIGINT, signalhandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Cannot handler SIGINT!");
        return 0;
    }
    if(signal(SIGTERM, signalhandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Cannot handler SIGTERM!");
        return 0;
    }
    if(signal(SIGHUP, signalhandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Cannot handler SIGHUP!");
        return 0;
    }

    // PID File wird in /var/run gegründet und beinaltet die PID in Textform
    // Sie bleibt offen und ist gelockt so lange der Prozess läuft
    pidFilehandle = open(pidfile, O_CREAT | O_RDWR);
    if(pidFilehandle == -1)
    {
        syslog(LOG_ERR, "Could not open PID lock file %s, exiting", pidfile);
        exit(EXIT_FAILURE);
    }
    if(lockf(pidFilehandle, F_TLOCK, 0) == -1)
    {
        syslog(LOG_ERR, "Could not lock PID lock file %s, exiting", pidfile);
        exit(EXIT_FAILURE);
    }
    sprintf(str, "%d\n", getpid());
    write(pidFilehandle, str, strlen(str));

    return 1;
}

void daemonShutdown()
{
    close(pidFilehandle);
    unlink(pidfile);
    closelog();
}

static void signalhandler(int sig)
{
    switch(sig) {
    case SIGHUP:
        syslog(LOG_WARNING, "Receive SIGHUP Signal");
        break;
    case SIGINT:
    case SIGTERM:
        syslog(LOG_INFO, "Receiving SIGTERM, Daemon exiting");
        bRunProgram = false; 
        break;
    case SIGPIPE:
        bRunProgram = true;
        break;
    default:
        syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
        break;
    }
}

void *ThreadNodejsProc(void * ptr)
{
    bThreadNodejs = false;
    
    if(iogrp->GetSEingAnz ())
    {
        pBrowserSocket = new CBrowserSocket;
        if(pBrowserSocket != NULL)
        {	 
            if(pBrowserSocket->Init(iogrp) == 0)
            {
                bThreadNodejs = true;
                while(bRunProgram)
                {
                    delayMicroseconds(ZYKLUSZEIT);
                    pBrowserSocket->Run();
                }  
            }
        }
        delete pBrowserSocket;
        pBrowserSocket = NULL;
        bThreadNodejs = false;
    }
    return (void *) NULL;
}

//
//  Kann nur in einer getrennten Thread laufen, wenn Modbus der 2. RS485
//  Schnittstelle zugeordnet wird. Es gibt sonst Kollisionen auf dem SPI-Bus
//  (da aus mehreren Threads auf den SPI-Bus zugegriffen wird !!)
//  (in der aktuellen Softwareversion erfolgt diese Zuordnung automatisch !!)
//
void *ThreadModbusProc(void * ptr)
{
	bThreadModbus = false;
    
    if(iogrp->m_pModBusRTU == NULL)
        return (void *)NULL;

    bThreadModbus = true;
    while(bRunProgram)
    {
        delayMicroseconds(ZYKLUSZEIT);
        if(iogrp->m_pModBusRTU != NULL)
            iogrp->m_pModBusRTU->Control();
    }
    bThreadModbus = false;
    return (void *) NULL;
}

void *ThreadProc(void *ptr)
{
  
    bThreadProc = true;
    while(bRunProgram)
    {
        delayMicroseconds(ZYKLUSZEIT);
        if(iogrp->m_pHue)
            iogrp->m_pHue->Control();
        if(iogrp->m_pMusic)
            iogrp->m_pMusic->Control();
        if(iogrp->m_pAlarm)
            iogrp->m_pAlarm->Control();
        if(iogrp->m_pHistory)
            iogrp->m_pHistory->ControlFiles(iogrp->GetpcAusgHistory(), iogrp->GetEWAusgFifo(), iogrp->m_pHue, iogrp->m_pSomfy);
        if(iogrp->m_pSonos)
            iogrp->m_pSonos->Control(m_pUhr->getUhrzeit());
        if(iogrp->m_pSomfy)
            iogrp->m_pSomfy->Control();
        if(iogrp->m_pAlarmClock)
            iogrp->m_pAlarmClock->Control(m_pUhr->getUhrmin(), m_pUhr->getWochenTag());            
    }
    bThreadProc = false;
    return (void *)NULL;
}
