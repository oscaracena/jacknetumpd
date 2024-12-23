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
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/65a73d24/SystemSleep.o \
	${OBJECTDIR}/_ext/65a73d24/network.o \
	${OBJECTDIR}/_ext/8c65c7bb/NetUMP.o \
	${OBJECTDIR}/_ext/8c65c7bb/NetUMP_SessionProtocol.o \
	${OBJECTDIR}/_ext/8c65c7bb/UMP_Transcoder.o \
	${OBJECTDIR}/_ext/5c0/Endpoint.o \
	${OBJECTDIR}/_ext/5c0/UMP_mDNS.o \
	${OBJECTDIR}/_ext/5c0/jacknetumpd.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-ljack

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/jacknetumpd

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/jacknetumpd: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/jacknetumpd ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/65a73d24/SystemSleep.o: ../../../../../SDK/beb/common_src/SystemSleep.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/65a73d24
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/65a73d24/SystemSleep.o ../../../../../SDK/beb/common_src/SystemSleep.cpp

${OBJECTDIR}/_ext/65a73d24/network.o: ../../../../../SDK/beb/common_src/network.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/65a73d24
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/65a73d24/network.o ../../../../../SDK/beb/common_src/network.cpp

${OBJECTDIR}/_ext/8c65c7bb/NetUMP.o: ../../NetUMP/NetUMP.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/8c65c7bb
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/8c65c7bb/NetUMP.o ../../NetUMP/NetUMP.cpp

${OBJECTDIR}/_ext/8c65c7bb/NetUMP_SessionProtocol.o: ../../NetUMP/NetUMP_SessionProtocol.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/8c65c7bb
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/8c65c7bb/NetUMP_SessionProtocol.o ../../NetUMP/NetUMP_SessionProtocol.cpp

${OBJECTDIR}/_ext/8c65c7bb/UMP_Transcoder.o: ../../NetUMP/UMP_Transcoder.c
	${MKDIR} -p ${OBJECTDIR}/_ext/8c65c7bb
	${RM} "$@.d"
	$(COMPILE.c) -g -D__TARGET_LINUX__ -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/8c65c7bb/UMP_Transcoder.o ../../NetUMP/UMP_Transcoder.c

${OBJECTDIR}/_ext/5c0/Endpoint.o: ../Endpoint.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/Endpoint.o ../Endpoint.cpp

${OBJECTDIR}/_ext/5c0/UMP_mDNS.o: ../UMP_mDNS.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/UMP_mDNS.o ../UMP_mDNS.cpp

${OBJECTDIR}/_ext/5c0/jacknetumpd.o: ../jacknetumpd.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/5c0
	${RM} "$@.d"
	$(COMPILE.cc) -g -D__TARGET_LINUX__ -I../../../../../SDK/beb/common_src -I../../NetUMP -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5c0/jacknetumpd.o ../jacknetumpd.cpp

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
