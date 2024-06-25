
#include "WinUtil.h"
#include "Logger.h"

#if defined(Q_OS_WIN)

#include <QProcessEnvironment>

#include <Windows.h>
#include <WtsApi32.h>			// For WTSQueryUserToken()
#include <winternl.h>			// For the internal NT process structures
#include <sddl.h>				// For convert ConvertSidToStringSid

#include <sstream>
#include <vector>
#include <QTimer>


// Gets toggled to cause one call to the intercepted CreateProcess to have
// the user token elevated
bool g_elevateNextCreateProcess = false;

static QString ErrorString(DWORD err)
{
	char ErrBuff[1024];
	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, ErrBuff, ARRAYSIZE(ErrBuff), NULL) == 0)
	{
		return "Unknown error";
	}
	return ErrBuff;
}


static bool SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		DWORD err = GetLastError();
		Logger(LOG_ERROR) << "Error " << err << " looking up privilege value " << lpszPrivilege << ": " << ErrorString(err);
		return false;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.
	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		DWORD err = GetLastError();
		Logger(LOG_ERROR) << "Error " << err << " adjusting token privilege " << lpszPrivilege << ": " << ErrorString(err);
		return false;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		Logger(LOG_ERROR) << "The token does not have the specified privilege: " << lpszPrivilege;
		return false;
	}

	return true;
}


bool AddProcessPrivilege(LPCTSTR privName)
{
	// Adjust token privileges
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		DWORD err = GetLastError();
		Logger(LOG_ERROR) << "Error " << err << " opening current process token to set privileges: " << ErrorString(err);
		return false;
	}

	bool ret = true;
	if (!SetPrivilege(hToken, privName, TRUE))
	{
		ret = false;
	}

	CloseHandle(hToken);
	return ret;
}


bool ShutdownOrReboot(bool reboot)
{
	if (!AddProcessPrivilege(SE_SHUTDOWN_NAME))
	{
		return false;
	}

	if (!InitiateSystemShutdownEx(NULL, NULL, 0, TRUE, reboot ? TRUE : FALSE, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
	{
		DWORD err = GetLastError();
		Logger(LOG_ERROR) << "Error " << err << (reboot ? " rebooting" : " shutting down") << " the system: " << ErrorString(err);
		return false;
	}

	return true;
}


// Hooks a function in the Import Address Table for a specific module in the current process
// Returns the pointer to the previous function or nullptr if hooking fails
//   moduleName - The name of the DLL to hook the function in, or nullptr for the process exe
//   dllName - The name of the DLL containing the function to hook
//   functionName - The name of the function to hook
//   newFunc - The address of the new function to call
LPVOID HookFunction(LPCWSTR moduleName, const QString& dllName, const QString& functionName, LPVOID newFunc)
{
	// The 'module handle' of an exe or DLL is actually the address in memory of where it is loaded
	LPBYTE imageBase = reinterpret_cast<LPBYTE>(GetModuleHandle(moduleName));
	if (nullptr == imageBase) return nullptr;	// Can't find module
	PIMAGE_DOS_HEADER pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);
	if (0x5a4d != pDosHeader->e_magic) return nullptr;		// MZ
	PIMAGE_NT_HEADERS pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(imageBase + pDosHeader->e_lfanew);
	if (0x00004550 != pNtHeaders->Signature) return nullptr;		// PE\0\0
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptors = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(imageBase + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImportDescriptors->Name != 0)
	{
		QString thisDllName = QString::fromUtf8(reinterpret_cast<const char *>(imageBase + pImportDescriptors->Name));
		if (0 == QString::compare(thisDllName, dllName, Qt::CaseInsensitive))
		{
			// Found the correct DLL, now find the function

			// There are two parallel arrays of IMAGE_THUNK_DATA structures, one pointing to the funtion names/ordinals (IMAGE_IMPORT_BY_NAME)
			//  and one pointing to the actual function
			PIMAGE_THUNK_DATA pNameThunkData = reinterpret_cast<PIMAGE_THUNK_DATA>(imageBase + pImportDescriptors->OriginalFirstThunk);
			PIMAGE_THUNK_DATA pFunctionThunkData = reinterpret_cast<PIMAGE_THUNK_DATA>(imageBase + pImportDescriptors->FirstThunk);

			size_t n = 0;
			while (pNameThunkData[n].u1.AddressOfData != 0)
			{
				PIMAGE_IMPORT_BY_NAME pImportByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(imageBase + pNameThunkData[n].u1.AddressOfData);
				QString thisFunctionName = QString::fromUtf8(pImportByName->Name);
				if (thisFunctionName == functionName)
				{
					// Found the correct function, patch it and return the original function pointer
					LPVOID* functionPointer = reinterpret_cast<LPVOID*>(&pFunctionThunkData[n].u1.Function);
					LPVOID ret = *functionPointer;
					DWORD oldRights;
					VirtualProtect(functionPointer, sizeof(LPVOID), PAGE_READWRITE, &oldRights);
					*functionPointer = newFunc;
					VirtualProtect(functionPointer, sizeof(LPVOID), oldRights, &oldRights);
					return ret;
				}

				n++;
			}
		}

		pImportDescriptors++;
	}

	return nullptr;
}


// Gets the interactive user token (once)
HANDLE GetInteractiveUserToken()
{
	int retries = 100;
	static HANDLE s_userToken = nullptr;

	if (nullptr == s_userToken)
	{
		bool success = false;
		while (success == false)
		{
			// Query the user token of the active session
			DWORD sessionId = WTSGetActiveConsoleSessionId();
			if (0xFFFFFFFF == sessionId)
			{
				DWORD err = GetLastError();
				Logger(LOG_WARNING) << QObject::tr("Error %1 getting active console session id [%2]")
					.arg(err)
					.arg(qt_error_string(err));
			}
			else if (!WTSQueryUserToken(sessionId, &s_userToken))
			{
				DWORD err = GetLastError();
				Logger(LOG_WARNING) << QObject::tr("Error %1 querying user token with session ID %2 [%3]")
					.arg(err)
					.arg(sessionId)
					.arg(qt_error_string(err));
			}
			else
			{
				success = true;
			}

			if (false == success)
			{
				if (--retries == 0)
				{
					Logger(LOG_ERROR) << QObject::tr("Process failed to query user token");
					return nullptr;
				}

				Sleep(1000);
			}
		}
	}

	return s_userToken;
}


HANDLE GetElevatedInteractiveToken()
{
	static HANDLE s_token = nullptr;

	if (nullptr == s_token)
	{
		// Check if this is a restricted token
		s_token = GetInteractiveUserToken();
		DWORD err;
		DWORD type = 0;
		DWORD len = 0;
		if (!GetTokenInformation(
			s_token,
			TokenElevationType,
			&type,
			sizeof(DWORD),
			&len))
		{
			err = GetLastError();
			std::stringstream ss;
			ss << "GetTokenInformation(1) failed: " << err;
			OutputDebugStringA(ss.str().c_str());
		}
		else if (TokenElevationTypeLimited == type)
		{
			// Get the other token this is linked to (the unrestricted one)
			if (!GetTokenInformation(
				s_token,
				TokenLinkedToken,
				&s_token,
				sizeof(HANDLE),
				&len))
			{
				err = GetLastError();
				std::stringstream ss;
				ss << "GetTokenInformation(2) failed: " << err;
				OutputDebugStringA(ss.str().c_str());
			}
		}
	}

	return s_token;
}


typedef BOOL(__stdcall *PCREATEPROCESSW)(LPCWSTR,
	LPWSTR,
	LPSECURITY_ATTRIBUTES,
	LPSECURITY_ATTRIBUTES,
	BOOL,
	DWORD,
	LPVOID,
	LPCWSTR,
	LPSTARTUPINFOW,
	LPPROCESS_INFORMATION);

PCREATEPROCESSW oldCreateProcessW = nullptr;

BOOL __stdcall overrideCreateProcessW(
	LPCWSTR               lpApplicationName,
	LPWSTR                lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR               lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	//Logger(LOG_ALWAYS) << "Overriden CreateProcessW called";

	HANDLE userToken;
	if (g_elevateNextCreateProcess)
	{
		g_elevateNextCreateProcess = false;
		userToken = GetElevatedInteractiveToken();
	}
	else
	{
		userToken = GetInteractiveUserToken();
	}
	if (nullptr == userToken)
		return FALSE;

	BOOL ret = CreateProcessAsUser(
		userToken,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation);

	return ret;
}

// HACKY HACKHACK, patch the import address table of the Qt core DLL to intercept the CreateProcessW
// call so that we can call CreateProcessAsUser instead when we are running as a service
bool OverrideCreateProcess()
{
#ifdef _DEBUG
	LPCWSTR qtDll = L"Qt5Cored.dll";
#else
	LPCWSTR qtDll = L"Qt5Core.dll";
#endif
	oldCreateProcessW = reinterpret_cast<PCREATEPROCESSW>(HookFunction(qtDll, "Kernel32.dll", "CreateProcessW", overrideCreateProcessW));

	return oldCreateProcessW != nullptr;
}


void ElevateNextCreateProcess()
{
	g_elevateNextCreateProcess = true;
}


// Creates a shared memory object with a null security descriptor and copies data into it
// Returns false if unsuccessful
// NOTE: This function leaves hMapFile open so the object remains in existance until the program exits
bool CreateSharedMemory(const QString& name, const QByteArray& data)
{
	// Create a null security descriptor so the file mapping object can be accessed from other sessions/users
	SECURITY_ATTRIBUTES SecAttr;
	SECURITY_DESCRIPTOR SecDesc;
	if (!InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION))
		return false;
	if (!SetSecurityDescriptorDacl(&SecDesc, TRUE, (PACL)0, FALSE))
		return false;
	SecAttr.nLength = sizeof(SecAttr);
	SecAttr.lpSecurityDescriptor = &SecDesc;
	SecAttr.bInheritHandle = TRUE;

	// Create the file mapping object
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		&SecAttr,
		PAGE_READWRITE,
		0,
		data.size(),
		reinterpret_cast<LPCWSTR>(name.utf16()));
	if (nullptr == hMapFile)
		return false;

	// Map a view of the file to memory
	LPBYTE pBuf = reinterpret_cast<LPBYTE>(MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		data.size()));
	if (nullptr == pBuf)
	{
		CloseHandle(hMapFile);
		return false;
	}

	// Copy the data into the shared memory
	memcpy(pBuf, data.data(), data.size());

	UnmapViewOfFile(pBuf);

	//CloseHandle(hMapFile);

	return true;
}


// Returns true if the interactive user can be queried
bool IsUserLoggedIn()
{
	return 0xFFFFFFFF != WTSGetActiveConsoleSessionId();
}


// Gets the user name and sid string of the interactive user
bool GetInteractiveUsername(QString& userstring, QString& sidstring)
{
	HANDLE userToken = GetInteractiveUserToken();
	if (nullptr == userToken)
	{
		return false;
	}

	char buff[4096];
	
	DWORD retLen = 0;
	if (!GetTokenInformation(
		userToken,
		TokenUser,
		buff,
		sizeof(buff),
		&retLen))
	{
		Logger(LOG_WARNING) << "GetTokenInformation failed: " << GetLastError();
		return false;
	}

	PTOKEN_USER pTokenUserStruct = reinterpret_cast<PTOKEN_USER>(buff);

	LPWSTR lpwStringSid = nullptr;
	if (!ConvertSidToStringSidW(pTokenUserStruct->User.Sid, &lpwStringSid))
	{
		Logger(LOG_WARNING) << "ConvertSidToStringSid failed: " << GetLastError();
		return false;
	}

	if (nullptr != lpwStringSid)
	{
		sidstring = QString::fromWCharArray(lpwStringSid);
		LocalFree(lpwStringSid);
	}

	WCHAR username[256] = L"";
	WCHAR domain[256] = L"";
	DWORD nameLen = 256;
	DWORD domainLen = 256;
	SID_NAME_USE use;
	if (!LookupAccountSidW(
		nullptr,
		pTokenUserStruct->User.Sid,
		username,
		&nameLen,
		domain,
		&domainLen,
		&use))
	{
		Logger(LOG_WARNING) << "LookupAccountSid failed: " << GetLastError();
		return false;
	}

	userstring = QString::fromWCharArray(username);

	return true;
}


// Replaces Windows enviroment variable names in a string with their values
QString SubstituteEnvironmentstring(const QString& envString, const QProcessEnvironment& procEnv)
{
	QString ret(envString);

	do
	{
		int perctPos1 = ret.indexOf('%');
		if (-1 == perctPos1)
			return ret;

		int perctPos2 = ret.indexOf('%', perctPos1 + 1);
		if (-1 == perctPos2)
			return ret;

		QString variable = ret.mid(perctPos1 + 1, perctPos2 - perctPos1 - 1);
		ret = ret.left(perctPos1) + procEnv.value(variable) + ret.mid(perctPos2 + 1);
	} while (!ret.isEmpty());

	return ret;
}

#endif
