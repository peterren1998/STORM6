#!/usr/bin/python
# ----------------------------------------------------------------------------------------
# The basic I/O class for a Hamilton syringe pump
# ----------------------------------------------------------------------------------------
# George Emanuel
# 6/28/19
# ----------------------------------------------------------------------------------------

import serial
import time

acknowledge = '\x06'
start = '\x0A'
stop = '\x0D'


# ----------------------------------------------------------------------------------------
# HamiltonPSD6 Syringe Pump Class Definition
# ----------------------------------------------------------------------------------------
class APump():
    def __init__(self, parameters=False):

        print('Initializing pump')

        # Define attributes
        self.com_port = parameters.get("pump_com_port", "COM3")
        self.pump_ID = parameters.get("pump_ID", 30)
        self.verbose = parameters.get("verbose", True)
        self.simulate = parameters.get("simulate_pump", True)
        self.serial_verbose = parameters.get("serial_verbose", False)
        self.flip_flow_direction = parameters.get("flip_flow_direction", False)

        # Create serial port
        self.serial = serial.Serial(
            port=self.com_port, baudrate=9600, timeout=0.1)

#        response_pass = False
#        while not response_pass:
#            check_status = self.sendString('/1?R\r')
#            response_code = self.getResponseCodeFromResponse(check_status)
#            response_pass = response_code == 0
#            print('Status: ', repr(check_status.decode()))
#            print('Response Code: ', response_code)
#            time.sleep(5)

        # enable h commands
        self.sendString('/1h30001R\r')
#        self.sendString('/1h30003R\r') # CHECK BEFORE RUNNING FACTORY RESET
#        response = self.sendString('/1h30003R\r')


#        while True:
#            response = self.getResponse()
#            print('Response: ', repr(response.decode()))
#            print('Waiting for 30 seconds...')
#            time.sleep(30)


        # TODO Confirm that this initialization only pushes to waste
        self.sendString('/1OZ1R\r')

        # Define initial pump status
        self.flow_status = "Stopped"
        self.speed = 1000
        self.direction = "Forward"

        self.disconnect()
        self.setSpeed(self.speed)
        self.identification = 'HamiltonSyringe'

    def getResponseCodeFromResponse(self, response):
        return int(response.decode().split('`')[1].split('\x03')[0])

    def pumpType(self):
        return 'syringe'

    def getIdentification(self):
        return self.sendImmediate(self.pump_ID, "%")

    def getPumpPosition(self):
        return int(self.sendString('/1?R\r').decode()
                   .split('`')[1].split('\x03')[0])

    def getValvePosition(self):
        positionDict = {
            0: 'Moving',
            1: 'Input',
            2: 'Output',
            3: 'Wash',
            4: 'Return',
            5: 'Bypass',
            6: 'Extra'
        }
        return positionDict[int(self.sendString('/1?23000\r').decode()
                                .split('`')[1].split('\x03')[0])]

    def getStatus(self):
        return (self.getPumpPosition(), self.getValvePosition())

        message = self.readDisplay()

        if self.flip_flow_direction:
            direction = {" ": "Not Running", "-": "Forward", "+": "Reverse"}. \
                get(message[0], "Unknown")
        else:
            direction = {" ": "Not Running", "+": "Forward", "-": "Reverse"}. \
                get(message[0], "Unknown")

        status = "Stopped" if direction == "Not Running" else "Flowing"

        control = {"K": "Keypad", "R": "Remote"}.get(message[-1], "Unknown")

        auto_start = "Disabled"

        speed = float(message[1:len(message) - 1])

        return (status, self.getPumpPosition(), direction, control, auto_start, "No Error")

    def close(self):
        pass

    def setValvePosition(self, valvePosition):
        # valve position is either 'input' or 'output'
        if valvePosition == 'Input':
            self.sendString('/1IR\r')
        elif valvePosition == 'Output':
            self.sendString('/1OR\r')

    def setSyringePosition(self, position, valvePosition=None, speed=500,
                           emptyFirst=False):
        commandString = '/1'

        if emptyFirst:
            commandString += 'OV5000A0'

        if valvePosition is not None:
            if valvePosition == 'Input':
                commandString += 'I'
            else:
                commandString += 'O'

        commandString += 'V' + str(int(speed))
        commandString += 'A' + str(int(position))

        self.sendString(commandString + 'R\r')

    def emptySyringe(self):
        self.sendString('/1OV5000A0R\r')

    def stopSyringe(self):
        self.sendString('/1TR\r')

    def resetSyringe(self):
        self.sendString('/1OZ1R\r')

    def initializeSyringe(self):
        self.sendString('/1ZR\r')

    def setSpeed(self, speed):
        if 2 <= speed <= 5800:
            self.sendString('/1V%iR\r' % speed)

    def stopFlow(self):
        self.setSpeed(0.0)
        return True

    def disconnect(self):
        pass
        #self.sendAndAcknowledge('\xff')

    def sendString(self, string):
        self.serial.write(string.encode())
        return self.serial.readline()

    def getResponse(self):
        return self.serial.readline()

    def sendStringAndWaitUntilIdle(self, string):
        idle_check = False
        response = self.sendString(string)
        while not idle_check:
            response = self.getResponse()
            idle_flag = self.getResponseCodeFromResponse(response)
            idle_check = idle_flag == 0
        print(f'Passed command {repr(string)} with idle flag on')
        return response

    def checkStatus(self):
        response = self.sendString('/1?R\r')
        print('Response: ', repr(response.decode()))
        return response

    def moveToLogicalSlot(self):
        self.sendString('/1h23001R\r')

    def queryLogicalPos(self):
        response = self.sendString('/1?23000R\r')
        print('Response: ', repr(response.decode()))
        return response
