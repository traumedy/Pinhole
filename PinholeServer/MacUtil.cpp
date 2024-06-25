#include "MacUtil.h"
#include "Logger.h"


#if defined(Q_OS_MAC)
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>

static OSStatus SendAppleEventToSystemProcess(AEEventID EventToSend);


OSStatus SendAppleEventToSystemProcess(AEEventID EventToSend)
{
	AEAddressDesc targetDesc;
	static const ProcessSerialNumber kPSNOfSystemProcess = { 0, kSystemProcess };
	AppleEvent eventReply = { typeNull, NULL };
	AppleEvent appleEventToSend = { typeNull, NULL };

	OSStatus error = noErr;

	error = AECreateDesc(typeProcessSerialNumber, &kPSNOfSystemProcess,
		sizeof(kPSNOfSystemProcess), &targetDesc);

	if (error != noErr)
	{
		return(error);
	}

	error = AECreateAppleEvent(kCoreEventClass, EventToSend, &targetDesc,
		kAutoGenerateReturnID, kAnyTransactionID, &appleEventToSend);

	AEDisposeDesc(&targetDesc);
	if (error != noErr)
	{
		return(error);
	}

#if 0
	// Deprecated
	error = AESend(&appleEventToSend, &eventReply, kAENoReply,
		kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
#else
	error = AESendMessage(&appleEventToSend, &eventReply,
		kAENoReply, kAEDefaultTimeout);
#endif

	AEDisposeDesc(&appleEventToSend);
	if (error != noErr)
	{
		return(error);
	}

	AEDisposeDesc(&eventReply);

	return(error);
}


bool ShutdownOrReboot(bool reboot)
{
	OSStatus error = SendAppleEventToSystemProcess(reboot ? kAERestart : kAEShutDown);
	if (0 != error)
	{
		Logger(LOG_ERROR) << "Error " << error << (reboot ? " rebooting" : " shutting down") << " the system";
		return false;
	}
	return true;
}

#endif

