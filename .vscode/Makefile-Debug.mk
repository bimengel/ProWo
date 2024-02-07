#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/CAlarm.o \
	${OBJECTDIR}/CAlarmClock.o \
	${OBJECTDIR}/CBerechneBase.o \
	${OBJECTDIR}/CBrowserEntity.o \
	${OBJECTDIR}/CBrowserMenu.o \
	${OBJECTDIR}/CBrowserSocket.o \
	${OBJECTDIR}/CCalculator.o \
	${OBJECTDIR}/CGsm.o \
	${OBJECTDIR}/CHeizung.o \
	${OBJECTDIR}/CHistory.o \
	${OBJECTDIR}/CHue.o \
	${OBJECTDIR}/CIOGroup.o \
	${OBJECTDIR}/CJson.o \
	${OBJECTDIR}/CKeyBoard.o \
	${OBJECTDIR}/CLCDDisplay.o \
	${OBJECTDIR}/CModBus.o \
	${OBJECTDIR}/CMusic.o \
	${OBJECTDIR}/COper.o \
	${OBJECTDIR}/CRS485.o \
	${OBJECTDIR}/CReadFile.o \
	${OBJECTDIR}/CSensor.o \
	${OBJECTDIR}/CSerial.o \
	${OBJECTDIR}/CSonos.o \
	${OBJECTDIR}/CStatistic.o \
	${OBJECTDIR}/CTH1.o \
	${OBJECTDIR}/CUhr.o \
	${OBJECTDIR}/CWSValue.o \
	${OBJECTDIR}/CWStation.o \
	${OBJECTDIR}/CZaehler.o \
	${OBJECTDIR}/gpiolib.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/sensirion_voc_algoritm.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pthread -D DEBUG
CXXFLAGS=-pthread -D DEBUG

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs libcurl` `pkg-config --libs libmpdclient`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f .vscode/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/prowo

${CND_DISTDIR}/${CND_CONF}/prowo: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/prowo ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/CAlarm.o: CAlarm.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CAlarm.o CAlarm.cpp

${OBJECTDIR}/CAlarmClock.o: CAlarmClock.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CAlarmClock.o CAlarmClock.cpp

${OBJECTDIR}/CBerechneBase.o: CBerechneBase.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CBerechneBase.o CBerechneBase.cpp

${OBJECTDIR}/CBrowserEntity.o: CBrowserEntity.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CBrowserEntity.o CBrowserEntity.cpp

${OBJECTDIR}/CBrowserMenu.o: CBrowserMenu.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CBrowserMenu.o CBrowserMenu.cpp

${OBJECTDIR}/CBrowserSocket.o: CBrowserSocket.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CBrowserSocket.o CBrowserSocket.cpp

${OBJECTDIR}/CCalculator.o: CCalculator.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CCalculator.o CCalculator.cpp

${OBJECTDIR}/CJson.o: CJson.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CJson.o CJson.cpp

${OBJECTDIR}/CGsm.o: CGsm.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CGsm.o CGsm.cpp

${OBJECTDIR}/CHeizung.o: CHeizung.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CHeizung.o CHeizung.cpp

${OBJECTDIR}/CHistory.o: CHistory.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CHistory.o CHistory.cpp

${OBJECTDIR}/CHue.o: CHue.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CHue.o CHue.cpp

${OBJECTDIR}/CIOGroup.o: CIOGroup.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CIOGroup.o CIOGroup.cpp

${OBJECTDIR}/CKeyBoard.o: CKeyBoard.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CKeyBoard.o CKeyBoard.cpp

${OBJECTDIR}/CLCDDisplay.o: CLCDDisplay.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CLCDDisplay.o CLCDDisplay.cpp

${OBJECTDIR}/CModBus.o: CModBus.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CModBus.o CModBus.cpp

${OBJECTDIR}/CMusic.o: CMusic.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CMusic.o CMusic.cpp

${OBJECTDIR}/COper.o: COper.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/COper.o COper.cpp

${OBJECTDIR}/CRS485.o: CRS485.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CRS485.o CRS485.cpp

${OBJECTDIR}/CReadFile.o: CReadFile.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CReadFile.o CReadFile.cpp

${OBJECTDIR}/CSensor.o: CSensor.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CSensor.o CSensor.cpp

${OBJECTDIR}/CSerial.o: CSerial.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CSerial.o CSerial.cpp

${OBJECTDIR}/CSonos.o: CSonos.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CSonos.o CSonos.cpp

${OBJECTDIR}/CStatistic.o: CStatistic.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CStatistic.o CStatistic.cpp

${OBJECTDIR}/CTH1.o: CTH1.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CTH1.o CTH1.cpp

${OBJECTDIR}/CUhr.o: CUhr.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CUhr.o CUhr.cpp

${OBJECTDIR}/CWSValue.o: CWSValue.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CWSValue.o CWSValue.cpp

${OBJECTDIR}/CWStation.o: CWStation.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CWStation.o CWStation.cpp

${OBJECTDIR}/CZaehler.o: CZaehler.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CZaehler.o CZaehler.cpp

${OBJECTDIR}/gpiolib.o: gpiolib.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gpiolib.o gpiolib.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/sensirion_voc_algoritm.o: sensirion_voc_algoritm.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g `pkg-config --cflags libcurl` `pkg-config --cflags libmpdclient`   -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/sensirion_voc_algoritm.o sensirion_voc_algoritm.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
