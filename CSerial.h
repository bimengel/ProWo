class CSerial
{
public:
    CSerial();
    ~CSerial();
    int Init(string strName, int baud, int bits, int stops, char parity, bool bRTSCTS);
    int DataReceived();

protected:
    int m_iSerialPort;
};

class CEWUSBSerial : public CSerial
{
public:
    CEWUSBSerial();
    ~CEWUSBSerial();
    bool Control(int *m_iEWUSBSetLearnChannel);
    void SetIOGroup(char *pPtr);
    bool IsBelegt(int channel);
    void ResetEingChannel(int channel);
    void ResetEing();
    bool Transmit(int channel, int button);
    int GetCode();
protected:
    int WriteEWTabelle();
    int ReadEWTabelle();
    CIOGroup *m_pIOGroup;
    int m_iCode;
    int m_iButton;
    int m_iChannel;
    int m_iWaitBeforeEnd; // Zusatzdauer bis ein Taster als losgelassen gilt
    // Hier werden die channelnr (startent mit 0!!) und der Kode abgespeichert
    // Key = Kode, channel = mapped value
    std::map<int, int> m_mapCode;
};
