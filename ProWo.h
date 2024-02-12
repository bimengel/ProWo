#ifndef ProWo_h__
#define ProWo_h__

#define ZYKLUSZEIT	  1000 // Zykluszeit in µSekunden (1 msek)
#define GSMZYKLUSZEIT  500 // wird multipliziert mit ZYKLUSZEIT also 500msek

#define HYSTERESE	    5       // 0.5°  
#define STDTEMP         150     // Standart Temperatur ist 15°
#define MAXTEMP         300
#define MAXAUSGCHANNEL      128
#define PORTANZAHLCHANNEL   8
#define EWANZAHLCHANNEL     32 // Anzahl Kanäle pro EasyWave_board

using namespace std;
typedef unsigned char BYTE;

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <string>
#include <map>
#include <queue>    
#include <vector>
#include "curl/curl.h"
#include <mpd/client.h>
#include "gpiolib.h"
#include "CRS485.h"
#include "CModBus.h"
#include "sensirion_voc_algorithm.h"
#include "CSensor.h"
#include "CTH1.h"
#include "CHeizung.h"
#include "COper.h"
#include "CReadFile.h"
#include "CBerechneBase.h"
#include "CAlarmClock.h"
#include "CMusic.h"
#include "CJson.h"
#include "CSonos.h"
#include "CStatistic.h"
#include "CZaehler.h"
#include "CWSValue.h"
#include "CWStation.h"
#include "CUhr.h"
#include "CHue.h"
#include "CHistory.h"
#include "CBrowserEntity.h"
#include "CBrowserMenu.h"
#include "CAlarm.h"
#include "CSerial.h"
#include "CCalculator.h"
#include "CGsm.h"
#include "CIOGroup.h"
#include "CLCDDisplay.h"
#include "CKeyBoard.h"
#include "CBrowserSocket.h"


extern char *pProgramPath;
extern bool bRunProgram;
extern CUhr *m_pUhr;
extern int ext_iI2CSwitchType;
extern int ext_iMaxZykluszeit;
extern pthread_mutex_t ext_mutexNodejs;	
extern CKeyBoard *pKeyBoard;

#endif  // ProWo_h__
