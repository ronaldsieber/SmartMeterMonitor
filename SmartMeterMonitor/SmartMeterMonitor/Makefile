#***************************************************************************#
#                                                                           #
#  Copyright (c) 2026 Ronald Sieber                                         #
#                                                                           #
#  File:         Makefile                                                   #
#  Description:  Makefile for SmartMeterMonitor                             #
#                (Version for sysWORXX Pi-AM62x with Smart Metering HAT)    #
#                                                                           #
#  -----------------------------------------------------------------------  #
#                                                                           #
#  Revision History:                                                        #
#                                                                           #
#  2026/01/17 -rs:   V1.00 Initial version                                  #
#                                                                           #
#****************************************************************************


# --------- Project Settings ---------

ifeq ('$(TARGET_CFG)','')
	TARGET_CFG	= RELEASE
#	TARGET_CFG	= DEBUG
endif

#  Select between debug and release settings
ifeq ($(TARGET_CFG),RELEASE)
    DBG_MODE = NDEBUG
else
    DBG_MODE = _DEBUG
endif

GASMETER = _GASMETER_



# --------- Compile Settings ---------
CC					= g++
STRIP				= strip
CFLAGS				= -D$(GASMETER) -D$(DBG_MODE)
INC_APP_ROOT		= ./
SRC_SMARTMETER		= ./SmartMeter
SRC_GASMETER		= ./GasMeter
MODBUS_H			= /home/smm/bin/include/modbus
MODBUS_LIB			= -L/home/smm/bin/lib -lmodbus
SRC_CONFIGPARSER	= ../configparser/src
INC_CONFIGPARSER	= ../configparser/include
SRC_MQTT_PACKET		= ../Mqtt/paho_mqtt_embedded_c/MQTTPacket/src
SRC_MQTT_TRANSPORT	= ../Mqtt/Mqtt_Transport

INCLUDE				= -I$(INC_APP_ROOT) -I$(SRC_SMARTMETER) -I$(SRC_GASMETER) -I$(MODBUS_H) -I$(INC_CONFIGPARSER) -I$(SRC_MQTT_PACKET) -I$(SRC_MQTT_TRANSPORT) -I/usr/include/
LIBS				= -lsysworxx_io

EXEC				= SmartMeterMonitor

OBJS				= Main.o \
					  CfgReader.o \
					  LibSys.o \
					  LibMqtt.o \
					  SDM230.o \
					  SDM630.o \
					  GasMeter.o \
					  GMSensIf_sysPi.o \
					  configparser.o \
					  MqttTransport_Posix.o \
					  MQTTConnectClient.o \
					  MQTTSubscribeClient.o \
					  MQTTUnsubscribeClient.o \
					  MQTTConnectServer.o \
					  MQTTSubscribeServer.o \
					  MQTTUnsubscribeServer.o \
					  MQTTPacket.o \
					  MQTTFormat.o \
					  MQTTSerializePublish.o \
					  MQTTDeserializePublish.o \
					  Trace.o



# --------- Default-Target ---------
all:				print_settings $(EXEC)



# --------- Print Settings ---------
print_settings:
					@echo
					@echo "Make Settings"
					@echo "   CFLAGS     = '$(CFLAGS)'"
					@echo "   INCLUDE    = '$(INCLUDE)'"
					@echo "   LIBS       = '$(LIBS)'"
					@echo "   MODBUS_LIB = '$(MODBUS_LIB)'"
					@echo "   EXEC       = '$(EXEC)'"
					@echo



# --------- Compile single Source ---------

#           ----- MainApp -----
Main.o:				Makefile Main.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

CfgReader.o:		Makefile CfgReader.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

LibSys.o:			Makefile LibSys.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

LibMqtt.o:			Makefile LibMqtt.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

Trace.o:			Makefile Trace.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

SDM230.o:			Makefile $(SRC_SMARTMETER)/SDM230.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_SMARTMETER)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

SDM630.o:			Makefile $(SRC_SMARTMETER)/SDM630.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_SMARTMETER)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

GasMeter.o:			Makefile $(SRC_GASMETER)/GasMeter.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_GASMETER)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

GMSensIf_sysPi.o:	Makefile $(SRC_GASMETER)/GMSensIf_sysPi.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_GASMETER)/$(notdir $*.cpp) $(INCLUDE) -o $*.o


#           ----- configparser -----
configparser.o:		Makefile $(SRC_CONFIGPARSER)/configparser.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_CONFIGPARSER)/$(notdir $*.cpp) $(INCLUDE) -o $*.o


#           ----- MQTT -----
MQTTConnectClient.o:	Makefile $(SRC_MQTT_PACKET)/MQTTConnectClient.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTSubscribeClient.o:	Makefile $(SRC_MQTT_PACKET)/MQTTSubscribeClient.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTUnsubscribeClient.o:	Makefile $(SRC_MQTT_PACKET)/MQTTUnsubscribeClient.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTConnectServer.o:	Makefile $(SRC_MQTT_PACKET)/MQTTConnectServer.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTSubscribeServer.o:	Makefile $(SRC_MQTT_PACKET)/MQTTSubscribeServer.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTUnsubscribeServer.o:	Makefile $(SRC_MQTT_PACKET)/MQTTUnsubscribeServer.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTPacket.o:		Makefile $(SRC_MQTT_PACKET)/MQTTPacket.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTFormat.o:		Makefile $(SRC_MQTT_PACKET)/MQTTFormat.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTSerializePublish.o:	Makefile $(SRC_MQTT_PACKET)/MQTTSerializePublish.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTDeserializePublish.o:	Makefile $(SRC_MQTT_PACKET)/MQTTDeserializePublish.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MqttTransport_Posix.o:	Makefile $(SRC_MQTT_TRANSPORT)/MqttTransport_Posix.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_TRANSPORT)/$(notdir $*.c) $(INCLUDE) -o $*.o



# --------- Link Executeable ---------
$(EXEC):			Makefile $(OBJS)
					@echo "Linking '$(EXEC)'..."
					@$(CC) -o $@ $(OBJS) $(MODBUS_LIB) $(LIBS)
ifeq ($(TARGET_CFG),RELEASE)
					@echo "Stripping '$(EXEC)'..."
					@$(STRIP) $@
endif
					@echo "Done."
					@echo



# --------- Clean Project ---------
clean:
					rm -f *.bak
					rm -f *.tmp
					rm -f $(EXEC)
					rm -f *.elf *.gdb *.o


