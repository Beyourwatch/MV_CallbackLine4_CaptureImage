from __future__ import print_function
import os
import platform
import sys
# import all the stuff from mvIMPACT Acquire into the current scope
from mvIMPACT import acquire
# import all the mvIMPACT Acquire related helper function such as 'conditionalSetProperty' into the current scope
# If you want to use this module in your code feel free to do so but make sure the 'Common' folder resides in a sub-folder of your project then
from mvIMPACT.Common import exampleHelper
import numpy
import ctypes
import cv2

# define line4Callback class as inheritance from acquire componentCallback
class Line4Callback(acquire.ComponentCallback):
    pCameraDevice_ = ""

    def __init__(self, pUserData):
        acquire.ComponentCallback.__init__(self)
        self.pCameraDevice_ = pUserData

    # if the line4 triggers
    def execute(self, c, pUserData):
        mvDevice = self.pCameraDevice_
        mv_device_control = acquire.DeviceControl(mvDevice)
        mv_function_interface = acquire.FunctionInterface(mvDevice)

        while mv_function_interface.imageRequestSingle() == acquire.DMR_NO_ERROR: {}

        pRequest = [None for v in range(5)]

        exampleHelper.manuallyStartAcquisitionIfNeeded(pDev, mv_function_interface)
        for i in range(5):
            requestNr = mv_function_interface.imageRequestWaitFor(-1)
            if mv_function_interface.isRequestNrValid(requestNr):
                pRequest[i] = mv_function_interface.getRequest(requestNr)
                if pRequest[i].isOK:
                    print("Info from " + pDev.serial.read() +
                          "timestamp: " + pRequest[i].infoTimeStamp_us.readS())

                    # For systems with NO mvDisplay library support
                    cbuf = (ctypes.c_char * pRequest[i].imageSize.read()).from_address(int(pRequest[i].imageData.read()))
                    channelType = numpy.uint16 if pRequest[i].imageChannelBitDepth.read() > 8 else numpy.uint8
                    arr = numpy.frombuffer(cbuf, dtype=channelType)

                    arr.shape = (
                    pRequest[i].imageHeight.read(), pRequest[i].imageWidth.read(), pRequest[i].imageChannelCount.read())
                    cv2.imshow("1", arr)
                    cv2.waitKey(10)
                    cv2.imwrite("C:\\Users\\Boqie Zhang\\PycharmProjects\\Callback_Line4\\" + pRequest[i].infoTimeStamp_us.readS()+".bmp", arr)

                mv_function_interface.imageRequestSingle()
            else:
                print("imageRequestWaitFor failed (" + str(
                    requestNr) + ", " + acquire.ImpactAcquireException.getErrorCodeAsString(requestNr) + ")")

        cv2.waitKey()
        cv2.destroyAllWindows()
        exampleHelper.manuallyStopAcquisitionIfNeeded(pDev, mv_function_interface)

        for i in range(5):
            pRequest[i].unlock()

        mv_function_interface.imageRequestReset(0, 0)


# open device manager before construct the systemmodule
devMgr = acquire.DeviceManager()

print("*********************************************************************************")
print("please choose a camera to open,  update device manager now")
devMgr.updateDeviceList()
deviceCount = devMgr.deviceCount()

for x in range(0, deviceCount):
    chosenDevice = devMgr.getDevice(x)

    if chosenDevice.isInUse:
        print("[" + str(
            x) + "]: " + chosenDevice.product.readS() + ", " + chosenDevice.serial.readS() + " !!!!Device is IN USE, CANT OPEN!!!")
    else:
        print("[" + str(
            x) + "]: " + chosenDevice.product.readS() + ", " + chosenDevice.serial.readS() + " Device not used, can open")

pDev = devMgr.getDevice(int(input()))
pDev.interfaceLayout.writeS("GenICam")
if pDev == None:
    exampleHelper.requestENTERFromUser()
    sys.exit(-1)
pDev.open()

# setup event control
eventControl = acquire.EventControl(pDev)
eventControl.eventSelector.writeS("Line4RisingEdge")
eventControl.eventNotification.write(acquire.bTrue)

# register event
localLine4Callback = Line4Callback(pDev)
if not localLine4Callback.registerComponent(eventControl.eventLine4RisingEdge):
    print("event register failed")
    sys.exit(-1)

print("register OK, use Line4 to trigger camera, or press Q to end the application")

while True:
    key = input()
    if key == "Q":
        sys.exit(-1)

sys.exit(-1)
