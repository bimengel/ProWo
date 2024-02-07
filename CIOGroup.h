#ifndef IOGroup_h__
#define IOGroup_h__


#define EWEINGDELAY         45
#define BEZLEN              16 // maximale Anzahl Zeichen der Bezeichnung

// I2C Adressen
// MCP 2307
#define ADDR23017           0x20		
#define ADDR23017REGA       0x00
#define ADDR23017REGB       0x10

// EasyWave RTRM08 
#define ADDRRTRM08          0x62

// Real time clock und RAM
#define ADDRCLOCKRAM        0x68

// PCA9544A
#define ADDRI2C             0x70
#define ADDRI2C9544         ADDRI2C + 0x07 // Adresse des ersten Switch
#define ADDRI2C9545         ADDRI2C + 0x03
#define INHADDRI2C          0xF4

// SC16IS750 
#define ADDRUARTIS750       0x48
#define EINGDELAYTIME       10 // Entprellzeit der Eingänge in msek

class CIOGroup
{
public:
    CRS485spi * m_pRS485_1;     // linke Schnittstelle über spi
    CRS485ob * m_pRS485_2;      // rechte Schnittstelle RaspberryPi onboard
    CModBus *m_pModBus;
    CBrowserMenu *m_pBrowserMenu;
    CWStation *m_pWStation;
    
    // GSM-Modul
    CGsm *m_pGsm;
    int m_UartAnz;
    int m_iLastEWButton;
    int m_iLastEWButtonErkennen;

    CHue *m_pHue;
    int m_iMaxAnzHueEntity;
    CAlarm *m_pAlarm;
    CMusic *m_pMusic;
    CHistory *m_pHistory;
    CSonos *m_pSonos; 
    CAlarmClock *m_pAlarmClock;

protected:
    int m_iMaxAnzModBusClient; 
    bool m_bBerechne;           // Zustand der Ausgänge muss neu errechnet werden
    bool m_bBerechneHeizung;
    int m_iControl;             // wenn 20x hintereinander ein Interrupt ansteht
    int m_iTest;
    int m_iAusgTest;            // gebraucht wenn m_iTest = 10
    // Eingänge
    int m_EingAnz;
    char *m_pcEing;             // Zustand der Eingänge
    char *m_pcEingLast;         // Zustand der Eingänge
    int *m_pEingInterrupt;		// delay der EingAusgboards
    CBoardAddr *m_pEingBoardAddr;

    // EasyWave board
    int m_EWAnzBoard;	// Anzahl Boards a 32 Kanäle
    // EasyWave USB
    CEWUSBSerial * m_pEWUSBSerial;
    int m_EWUSBEingAnz; // Anzahl USB Eingänge 
    int m_EWUSBAusgAnz; // Anzahl USB Ausgänge
    char *m_pEWEing;	// Zustand der EWEingänge
    int m_iEWUSBSetLearnChannel;
    //			Bit 0 : Zustandeinzeltaster A
    //			Bit 1 : Zustandeinzeltaster B
    //			Bit 2 : Zustandeinzeltaster C
    //			Bit 3 : Zustandeinzeltaster D
    //			Bit 4 : Zustandtaster A-B
    //			Bit 5 : Zustandtaster C-D
    int *m_pEWEingDelay;    // nach EWEINGDELAY (45 Sek) werden die bits 0 bis 3 zurückgesetzt
    char *m_pEWEingLast;
    CBoardAddr * m_pEWBoardAddr;

    // Merker
    int m_MerkAnz;          // Anzahl Merker
    char *m_pcMerk;
    char *m_pcMerkLast;
    
    // Merker Integer
    int m_IntegerAnz;
    CInteger * m_pInteger;

    // Sekunden Timer
    int m_SecTimerAnz;
    CSecTimer *m_pSecTimer;

    // Software-Eingänge
    int m_SEingAnz;
    char *m_pcSEing;
    char *m_pcSEingLast;

    // Ausgänge
    int m_AusgAnz;          // Anzahl der Ausgänge
    char *m_pcAusg;         // Zustand der Ausgänge
    char *m_pcAusgLast;     // letzter Zustand der Ausgänge
    char *m_pcAusgHistory;  // Ausgänge von vor einigen Tagen die angestellt werden sollen
    char *m_pcInvAusgHistoryEnabled; // Inverter Wert der aktivierten History-Ausgänge
    
    CBoardAddr *m_pAusgBoardAddr;

    // EW EasyWare Ausgänge
    char *m_pEWAusg;        // Zustand der EWAusgänge
    // 0=A, 1=B, 2=C, 3=D, 4=AB, 5=CD


    CBerechneBase **m_pBerechne;
    int m_iBerechneAnz;

    bool m_bHandAusg;		// gesetzt wenn Handsteuerung aktiv ist
    bool m_bSimEing;		// Simuation der Eingänge ist aktiviert worden
    int m_iChangeSensorModBusAddress;   // Nummer des TQS3-TF1 Sensors mit Adressenänderung
    int m_iZykluszeit;
    int m_gsmtimeslice;

    queue<struct EWAusgEntity> m_EWAusgFifo;

    CReadFile *m_pReadFile;

    CZaehler **m_pZaehler;
    int m_iAnzZaehler;
    int m_iMaxAnzZaehler;

    CSensor **m_pSensor;
    int m_iAnzSensor;
    int m_iMaxAnzSensor;
  
    CHeizung *m_pHeizung;
    
public:
    CIOGroup();
    ~CIOGroup();
    void SetHandAusg(bool bState);
    void SetBerechne();
    void SetBerechneHeizung();
    bool GetBerechne();
    void ResetSimulation();
    void InitGroup();
    int GetSEingAnz();
    bool GetSEingStatus(int nr);
    void SetSEingStatus(int nr, bool bState);
    char * GetSEingAddress(int nr);
    char * GetSEingLastAddress(int nr);
    int GetEWBoardAnz();
    int GetEWUSBEingAnz();
    int GetEWUSBAusgAnz();
    int GetEWEingTotAnz();
    int GetEWAusgTotAnz();
    char * GetEWEingAddress(int nr);
    char * GetEWEingLastAddress(int nr);
    char * GetEWAusgAddress(int nr);
    void SetEWEingStatus(int nr, int button, bool bState);
    int GetEingAnz();
    char * GetEingAddress(int nr);
    char * GetEingLastAddress(int nr);
    bool GetEingStatus(int nr);
    void SetEingStatus(int nr, bool bState);
    int GetMerkAnz();
    char * GetMerkAddress(int nr);
    char * GetMerkLastAddress(int nr);
    int GetIntegerAnz();
    CInteger * GetIntegerAddress(int nr);
    int GetSecTimerAnz();
    CSecTimer * GetSecTimerAddress(int nr);
    int GetAusgAnz();
    char * GetAusgAddress(int nr);
    char * GetAusgLastAddress(int nr);
    bool GetAusgStatus(int nr);
    void SetAusgStatus(int nr, bool bState);
    int GetAnzSensor();
    CSensor * GetSensorAddress(int nr);
    int GetHKAnz();
    CHeizKoerper * GetHKAddress(int nr);
    void Control(bool bAll);
    int BerechneZeile(int iLine, bool bState, bool bEval);
    void BerechneParameter(int iTime);
    void EW_Reset();
    void EW_ResetChannel(int channel);
    int EW_SetLearn(int channel);
    void EW_AusgSend(int channel, int button);
    int EWBoard_Transmit(int channel, int button);
    void EW_GetBezeichnung(char *ptr, int channel);
    void Les_IOGroup();
    int Write_Ausg(CBoardAddr *addr, int value);
    CZaehler *GetZaehler(int nr);
    int GetAnzZaehler();
    CHeizung * GetHZAddress();
    queue<struct EWAusgEntity> * GetEWAusgFifo();
    char * GetpcAusgHistory();
    int GetTest();
    CAlarmClock * GetAlarmClockAddress();
    
protected:
    int readAnzeige();
    bool EWBoard_TransmitFinish(int channel);
    void ReadConfig(char *pProgramPath);
    void PreReadConfig(char *pProgramPath);
    void LesParam(char *pProgramPath);
    void LesBrowserMenu(char *pProgramPath);
    void LesHeizung(char *pProgramPath);
    void LesAlarm(char *pProgramPath);
    void LesHistory(char *pProgramPath);
    void LesAlleEing();
    void Addr_unique (CBoardAddr *addr, int Anzahl, int Inh1, int Addr2,
             int Inh2, int Addr3, int Reg);
    void initEingAusg(int *anzEin, int *anzAus, int Inh1, int Addr2, 
             int Inh2, int Addr3);
    void initEingExp(int *AnzEin, int Inh1, int Addr2, int Inh2, int iAddr3);
    void initAusgExp(int *AnzAus, int Inh1, int Addr2, int Inh2, int iAddr3);
    void setEingang(CBoardAddr *pBoardAddr);
    void setAusgang(CBoardAddr *pBoardAddr);
    void init_EW(CBoardAddr *addr, int *anz, int Inh1, int Addr2, int Inh2, 
               int Addr3, int Reg);
    int EW_SetReceive (CBoardAddr *addr);
    int EW_Received(int board);
};

#endif