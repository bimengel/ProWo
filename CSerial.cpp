#include "ProWo.h"

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

int CSerial::Init(string strName, int baud, int bits, int stops, char parity, bool bRTSCTS)
{
    struct termios tty;

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
            tty.c_cflag |= (PARENB & ~PARODD);
            break;
        case 'O':
            tty.c_cflag |= PARENB & PARODD;
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
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

    tty.c_lflag &= ~ICANON; // disable canonical 
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    if (tcsetattr (m_iSerialPort, TCSANOW, &tty) != 0)
        return 8;
    else
        return 0;
}

CEWUSBSerial::CEWUSBSerial()
{

}

CEWUSBSerial::~CEWUSBSerial()
{

}
