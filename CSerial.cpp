#include "ProWo.h"
#include <sys/ioctl.h>
#include <termios.h>

CSerial::CSerial()
{
    m_iSerialPort = 0;
}

CSerial::~CSerial()
{
    if(m_iSerialPort)
        close(m_iSerialPort);
    m_iSerialPort = 0;
}
int CSerial::DataReceived()
{   
    int bytesAvailable = 0;
    if(ioctl(m_iSerialPort, FIONREAD, &bytesAvailable) < 0)
        bytesAvailable = -1;
    return bytesAvailable;
}

int CSerial::Init(string strName, int baud, int bits, int stops, char parity, bool bRTSCTS)
{
    struct termios tty;
    int ret = 0;

    if(!m_iSerialPort)
        m_iSerialPort = open(strName.c_str(), O_RDWR);
    else
        return 1;
    if(m_iSerialPort < 0)
        return 2;
    
    if(tcgetattr(m_iSerialPort, &tty) != 0)
        return 3;

    // Baudrate
    switch(baud) {
        case 230400:
            baud = B230400;
            break;
        case 115200:
            baud = B115200;
            break;
        case 57600:
            baud = B57600;
            break;
        case 38400:
            baud = B38400;
            break;
        case 19200:
            baud = B19200;
            break;
        case 9600:
            baud = B9600;
            break;
        case 4800:
            baud = B4800;
            break;
        case 2400:
            baud = B2400;
            break;
        default:
            return 4;
            break;
    }
    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    // bits
    switch(bits) {
        case 7:
            bits = CS7;
            break;
        case 8:
            bits = CS8;
            break;
        default:
            return 5;
            break;
    }
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= bits;

    // stops
    switch(stops) {
        case 1:
            tty.c_cflag &= ~CSTOPB;
            break;
        case 2:
            tty.c_cflag |= CSTOPB;
            break;
        default:
            return 6;
            break;
    }

    // Parität
    switch(parity) {
        case 'N':
            tty.c_cflag &= ~(PARENB | PARODD); // disable parity
            break;
        case 'E':
            tty.c_cflag |= (PARENB & ~PARODD); // Even parity
            break;
        case 'O':
            tty.c_cflag |= (PARENB & PARODD);  // Odd parity
            break ;
        default:
            return 7;
            break;
    }

    // Flow control RTSCTS
    if(bRTSCTS)
        tty.c_cflag |= CRTSCTS; // enable RTSCTS
    else
        tty.c_cflag &= ~CRTSCTS; // disbale RTSCTS
    tty.c_cflag |= CREAD;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_lflag &= ~ICANON; // disable canonical 
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 5;    // Wait for up to 0,5s (5 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    if (tcsetattr (m_iSerialPort, TCSANOW, &tty) != 0)
        return 1;    
    return 0;
}

CEWUSBSerial::CEWUSBSerial()
{
    m_iCode = 0;
    m_iButton = 0;
    m_iChannel = 0;
    m_pIOGroup = NULL;
    m_iWaitBeforeEnd = 0;
}

CEWUSBSerial::~CEWUSBSerial()
{

}
void CEWUSBSerial::SetIOGroup(char *pPtr)
{
    m_pIOGroup = (CIOGroup *)pPtr;
    ReadEWTabelle();    
}
bool CEWUSBSerial::Control(int *iEWUSBSetLearnChannel)
{
    int bytesRead, iPos, iCode, iButton, bytesAvailable;
    char chBuf[ANZRECEIVECHAR];
    bool bRet = false;
    char chError[50];

    bytesAvailable = DataReceived();
    if(bytesAvailable > 0)
    {
        if(bytesAvailable >= ANZRECEIVECHAR)
            bytesAvailable = ANZRECEIVECHAR;
        bytesRead = read(m_iSerialPort, chBuf, bytesAvailable);
        for(iPos=0; chBuf[iPos] && iPos < bytesAvailable && iPos < ANZRECEIVECHAR; )
        {
            if(strncmp(&chBuf[iPos], "OK", 2) == 0)
                iPos += 3;
            else if(strncmp(&chBuf[iPos], "ERROR", 5) == 0)
                iPos += 6;
            else if(strncmp(&chBuf[iPos], "REC", 3) == 0)
            {
                iCode = (int)strtol(&chBuf[iPos+4], NULL, 16);
                iButton = chBuf[iPos+11] - 'A';

                if(m_iCode != iCode || m_iButton != iButton)
                {                       
                    m_itmapCode = m_mapCode.find(iCode);
                    if(m_itmapCode != m_mapCode.end())
                    {
                        m_iChannel= m_itmapCode->second;
                        if(m_iChannel > 0 && m_iChannel <= m_pIOGroup->GetEWUSBEingAnz())
                        {
                            if(*iEWUSBSetLearnChannel) // Anlernen aktiviert aber Sender bekannt!
                            {
                                snprintf(chError, 50, "Sender bekannt!");
                                LCD_AnzeigeZeile3(chError);
                            }
                            else
                            {
                                m_pIOGroup->SetEWEingStatus(m_iChannel+m_pIOGroup->GetEWBoardAnz(), iButton, true);
                                m_iWaitBeforeEnd = 0;
                                m_iCode = iCode;
                                m_iButton = iButton;                            
                                bRet = true;
                            }
                        }
                    }
                    else // Code ist nicht in der Liste vorhanden !!
                    {
                        if(*iEWUSBSetLearnChannel)
                        {
                            m_mapCode.insert({iCode,*iEWUSBSetLearnChannel});
                            snprintf(chError, 50, "Kan.:%d, Tast:%d", *iEWUSBSetLearnChannel + m_pIOGroup->GetEWBoardAnz(), iButton);
                            LCD_AnzeigeZeile3(chError);
                            *iEWUSBSetLearnChannel = 0; 
                            WriteEWTabelle();                           
                        }
                    }
                }
                iPos += 13;
            }
            else
                iPos++;
        }
        if(iPos != bytesAvailable)
            syslog(LOG_INFO, "EWUSB read synchronisation problem");
    }
    else
    {   
        if(m_iChannel)
        {
            if(m_iWaitBeforeEnd++ >= 500) // bei einer ZYKLUSZEIT von 1ms, mindestens 500m
            {
                m_pIOGroup->SetEWEingStatus(m_iChannel+m_pIOGroup->GetEWBoardAnz(), m_iButton, false);
                m_iChannel = 0;
                m_iButton = 0;
                m_iCode = 0; 
                bRet = true;
            }
        }
    }
    return bRet;  
}

bool CEWUSBSerial::IsBelegt(int channel)
{   
    bool bRet = false;
    channel = channel - m_pIOGroup->GetEWBoardAnz();
    for(m_itmapCode = m_mapCode.begin(); m_itmapCode != m_mapCode.end(); m_itmapCode++ )
    {
        if(m_itmapCode->second == channel)
        {
            bRet = true;
            break;
        }
    }
    return bRet;
}

int CEWUSBSerial::WriteEWTabelle()
{   
    CReadFile *pReadFile;
    char chBuf[20];

    pReadFile = new CReadFile;
    pReadFile->OpenWrite(pProgramPath, 18, 0);
    for(m_itmapCode = m_mapCode.begin(); m_itmapCode != m_mapCode.end(); m_itmapCode++)
    {
        sprintf(chBuf, "%d;%d\n", m_itmapCode->first, m_itmapCode->second);
        pReadFile->WriteLine(chBuf);
    }
    pReadFile->Close();
    delete pReadFile;
    return 0;
}

int CEWUSBSerial::ReadEWTabelle()
{   
    CReadFile *pReadFile;
    int key, channel;

    pReadFile = new CReadFile;
    if(pReadFile->OpenRead(pProgramPath, 18, 0))
    {   
        while(pReadFile->ReadLine())
        {
            key = pReadFile->ReadNumber();
            pReadFile->ReadSeparator();
            channel = pReadFile->ReadNumber();
            if(channel < 0 && channel >= m_pIOGroup->GetEWUSBAusgAnz())
                pReadFile->Error(145);
            m_mapCode.insert(std::pair<int,int>(key, channel));
        }
    }
    pReadFile->Close();
    delete pReadFile;
    return 0;
}

void CEWUSBSerial::ResetEingChannel(int channel)
{
    channel = channel - m_pIOGroup->GetEWBoardAnz();
    if(channel > 0 && channel <= m_pIOGroup->GetEWUSBEingAnz())
    {
        for(m_itmapCode = m_mapCode.begin(); m_itmapCode != m_mapCode.end(); m_itmapCode++)
        {
            if(m_itmapCode->second == channel)
            {
                m_mapCode.erase(m_itmapCode->first);
                break;
            }
            
        }
        WriteEWTabelle();
    }
}

void CEWUSBSerial::ResetEing()
{
    m_mapCode.clear();
    WriteEWTabelle();
}

bool CEWUSBSerial::Transmit(int channel, int button)
{   
    char chSend[ANZSENDCHAR];
    int bytesWrite, iLen;
    bool bRet = true;

    iLen = sprintf(chSend, "TXP,%d,%c\n", channel, 'A'+button);
    bytesWrite = write(m_iSerialPort, chSend, iLen);
    if(bytesWrite != iLen)
    {   bRet = false;
        syslog(LOG_ERR, "USB Stick write Error!");
    }
    return bRet;
}

int CEWUSBSerial::GetCode()
{
    return m_iCode;
}