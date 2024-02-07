class CSerial
{
public:
    CSerial();
    ~CSerial();
    int Init(string strName, int baud, int bits, int stops, char parity, bool bRTSCTS);

protected:
    int m_iSerialPort;

};

class CEWUSBSerial : public CSerial
{
public:
    CEWUSBSerial();
    ~CEWUSBSerial();
};
