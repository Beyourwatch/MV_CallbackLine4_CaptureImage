#ifdef _MSC_VER // is Microsoft compiler?
#   if _MSC_VER < 1300  // is 'old' VC 6 compiler?
#       pragma warning( disable : 4786 ) // 'identifier was truncated to '255' characters in the debug information'
pragma warning(disable : 4100) // 'unreferenced formal parameter'
#   endif // #if _MSC_VER < 1300
#endif // #ifdef _MSC_VER
#pragma warning( disable : 4100 ) // 'unreferenced formal parameter'
#include <iostream>
#include <apps/Common/exampleHelper.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#ifdef _WIN32
#   include <mvDisplay/Include/mvIMPACT_acquire_display.h>
using namespace mvIMPACT::acquire::display;
#endif // #ifdef _WIN32

using namespace mvIMPACT::acquire;
using namespace std;

#if defined(linux) || defined(__linux) || defined(__linux__)
#   define NO_DISPLAY
#else
#   undef NO_DISPLAY
#endif // #if defined(linux) || defined(__linux) || defined(__linux__)


/********************************Camera Lin4 Call Back********************************************/
class CameraCallbackLine4 : public  mvIMPACT::acquire::ComponentCallback
{
public:
	CameraCallbackLine4(mvIMPACT::acquire::Device* pMVCamera) :
		ComponentCallback(), m_pMVCamera(pMVCamera) {}

	virtual void execute(mvIMPACT::acquire::Component& c, void* pDummy)
	{
		if (m_pMVCamera->state.readS() == "Present") {
			if (c.flags() & cfReadAccess)
			{
				PropertyI64 p(c);
				try
				{	
					mvIMPACT::acquire::FunctionInterface fi(m_pMVCamera);
					TDMR_ERROR result = DMR_NO_ERROR;
					while ((result = static_cast<TDMR_ERROR>(fi.imageRequestSingle())) == DMR_NO_ERROR) {};
					if (result != DEV_NO_FREE_REQUEST_AVAILABLE)
					{
						cout << "'FunctionInterface.imageRequestSingle' returned with an unexpected result: " << result
							<< "(" << mvIMPACT::acquire::ImpactAcquireException::getErrorCodeAsString(result) << ")" << endl;
					}
					manuallyStartAcquisitionIfNeeded(m_pMVCamera, fi);

					// Take 5 images, use 5 request to keep result
					mvIMPACT::acquire::Request* pRequest[5] ;


					const unsigned int timeout_ms = -1;

					// set cnt to 0, start from 0, max 5 images
					unsigned int cnt = 0;
					while (cnt<5)
					{
						// wait for results from the default capture queue
						int requestNr = fi.imageRequestWaitFor(timeout_ms);
						pRequest[cnt] = fi.isRequestNrValid(requestNr) ? fi.getRequest(requestNr) : 0;
						if (pRequest[cnt])
						{
							if (pRequest[cnt]->isOK())
							{
							
								cout << "Info from request"  << ": frame ID: " <<
									pRequest[cnt]->infoFrameID.readS() << ": Timestamp: " <<
									pRequest[cnt]->infoTimeStamp_us.readS()
									<< endl;
								++cnt;
							}
							else
							{
								cout << "Error: " << pRequest[cnt]->requestResult.readS() << endl;
							}

							fi.imageRequestSingle();
						}
						
					}

					// stop engine after work
					manuallyStopAcquisitionIfNeeded(m_pMVCamera, fi);

					// release request
					for (int index = 0; index < 5; index++)
					{
						if (pRequest[index])
						{
							pRequest[index]->unlock();
						}
					}
					// clear all queues
					fi.imageRequestReset(0, 0);

				}
				catch (const ImpactAcquireException& e)
				{
					cout << "An error occurred (error code: " << e.getErrorCode() << ")." << endl;
				}
			}
			else
			{
				cout << "No read access" << endl;
			}
		}
	}
public:
	Device* m_pMVCamera;
	int count = 0;

};


//-----------------------------------------------------------------------------
int main(int /*argc*/, char* /*argv*/[])
//-----------------------------------------------------------------------------
{
	DeviceManager devMgr;
	Device* pDev = getDeviceFromUserInput(devMgr);
	if (!pDev)
	{
		cout << "Unable to continue!" << endl
			<< "Press [ENTER] to end the application" << endl;
		cin.get();
		return 0;
	}

	try
	{
		pDev->open();
	}
	catch (const ImpactAcquireException& e)
	{
		// this e.g. might happen if the same device is already opened in another process...
		cout << "An error occurred while opening the device(error code: " << e.getErrorCode() << ")." << endl
			<< "Press [ENTER] to end the application" << endl;
		cin.get();
		return 0;
	}

	

	/****************************************************************************/
	// callback is executed when Line4RingsingEdge-event occurs
	mvIMPACT::acquire::GenICam::EventControl eventControl(pDev);
	eventControl.eventSelector.writeS("Line4RisingEdge");
	eventControl.eventNotification.writeS("On");


	CameraCallbackLine4 cameraCallbackline4(pDev);

	if (cameraCallbackline4.registerComponent(eventControl.eventLine4RisingEdgeFrameID) != true)
	{
		std::cout << "ERROR: Unable to register the camera's CallBack function!\n";
		std::cout << eventControl.eventLine4RisingEdgeFrameID.name() << " = " << eventControl.eventLine4RisingEdgeFrameID.readS() << endl;
	}
		
	
	/****************************************************************************/

	cout << "Press Q to end the application" << endl;
	while (true)
	{
		char key= cin.get();
		if (key == 'Q')
		{
			return 0;
			
		}


	}
	
	return 0;
}
