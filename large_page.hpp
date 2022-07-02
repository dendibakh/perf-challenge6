#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include <iostream>
#include <ntstatus.h>
#include <windows.h>
#include <ntsecapi.h>
#include <Sddl.h>


#define BUF_SIZE 65536

TCHAR szName[]=TEXT("LARGEPAGE");
typedef int (*GETLARGEPAGEMINIMUM)(void);

void DisplayError(const char* pszAPI, DWORD dwError)
{
    LPVOID lpvMessageBuffer;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dwError,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpvMessageBuffer, 0, NULL);

    //... now display this string
    _tprintf(TEXT("ERROR: API        = %s\n"), pszAPI);
    _tprintf(TEXT("       error code = %d\n"), dwError);
    _tprintf(TEXT("       message    = %s\n"), lpvMessageBuffer);

    // Free the buffer allocated by the system
    LocalFree(lpvMessageBuffer);

    ExitProcess(GetLastError());
}

void Privilege(const char* pszPrivilege, BOOL bEnable)
{
    HANDLE           hToken;
    TOKEN_PRIVILEGES tp;
    BOOL             status;
    DWORD            error;

    // open process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        DisplayError(TEXT("OpenProcessToken"), GetLastError());

    // get the luid
    if (!LookupPrivilegeValue(NULL, pszPrivilege, &tp.Privileges[0].Luid))
        DisplayError(TEXT("LookupPrivilegeValue"), GetLastError());

    tp.PrivilegeCount = 1;

    // enable or disable privilege
    if (bEnable)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // enable or disable privilege
    status = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    // It is possible for AdjustTokenPrivileges to return TRUE and still not succeed.
    // So always check for the last error value.
    error = GetLastError();
    if (!status || (error != ERROR_SUCCESS))
        DisplayError(TEXT("AdjustTokenPrivileges"), GetLastError());

    // close the handle
    if (!CloseHandle(hToken))
        DisplayError(TEXT("CloseHandle"), GetLastError());
}

void enable_large_pages(void)
{

    HANDLE hMapFile;
    LPCTSTR pBuf;
    DWORD size;
    GETLARGEPAGEMINIMUM pGetLargePageMinimum;
    HINSTANCE  hDll;

    // call succeeds only on Windows Server 2003 SP1 or later
    hDll = LoadLibrary(TEXT("kernel32.dll"));
    if (hDll == NULL)
        DisplayError(TEXT("LoadLibrary"), GetLastError());

    pGetLargePageMinimum = (GETLARGEPAGEMINIMUM)GetProcAddress(hDll,
        "GetLargePageMinimum");
    if (pGetLargePageMinimum == NULL)
        DisplayError(TEXT("GetProcAddress"), GetLastError());

    size = (*pGetLargePageMinimum)();

    printf("large page size : %d\n", size);

    FreeLibrary(hDll);

    _tprintf(TEXT("Page Size: %u\n"), size);

    Privilege(TEXT("SeLockMemoryPrivilege"), TRUE);

    printf("privilege set\n");

    /*
    hMapFile = CreateFileMapping(
         INVALID_HANDLE_VALUE,    // use paging file
         NULL,                    // default security
         PAGE_READWRITE | SEC_COMMIT | SEC_LARGE_PAGES,
         0,                       // max. object size
         size,                    // buffer size
         szName);                 // name of mapping object

    if (hMapFile == NULL)
        DisplayError(TEXT("CreateFileMapping"), GetLastError());
    else
        _tprintf(TEXT("File mapping object successfully created.\n"));

    Privilege(TEXT("SeLockMemoryPrivilege"), FALSE);

    pBuf = (LPTSTR) MapViewOfFile(hMapFile,          // handle to map object
         FILE_MAP_ALL_ACCESS | FILE_MAP_LARGE_PAGES, // read/write permission
         0,
         0,
         BUF_SIZE);

    if (pBuf == NULL)
        DisplayError(TEXT("MapViewOfFile"), GetLastError());
    else
        _tprintf(TEXT("View of file successfully mapped.\n"));

    // do nothing, clean up an exit
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    */
}















std::string GetErrorAsString(DWORD errorMessageID)
{
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

LSA_HANDLE GetPolicyHandle(WCHAR* SystemName)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    USHORT SystemNameLength;
    LSA_UNICODE_STRING lusSystemName;
    NTSTATUS ntsResult;
    LSA_HANDLE lsahPolicyHandle;

    // Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    //Initialize an LSA_UNICODE_STRING to the server name.
    SystemNameLength = wcslen(SystemName);
    lusSystemName.Buffer = SystemName;
    lusSystemName.Length = SystemNameLength * sizeof(WCHAR);
    lusSystemName.MaximumLength = (SystemNameLength + 1) * sizeof(WCHAR);

    // Get a handle to the Policy object.
    ntsResult = LsaOpenPolicy(
        &lusSystemName,    //Name of the target system.
        &ObjectAttributes, //Object attributes.
        POLICY_ALL_ACCESS, //Desired access permissions.
        &lsahPolicyHandle  //Receives the policy handle.
    );

    if (ntsResult != STATUS_SUCCESS)
    {
        // An error occurred. Display it as a win32 error code.
        auto winError = LsaNtStatusToWinError(ntsResult);
        wprintf(L"OpenPolicy returned %lu\n", winError);
        std::cout << "Error message: " << GetErrorAsString(winError) << std::endl;
        return NULL;
    }
    return lsahPolicyHandle;
}

bool InitLsaString(
    PLSA_UNICODE_STRING pLsaString,
    LPCWSTR pwszString
)
{
    DWORD dwLen = 0;

    if (NULL == pLsaString)
        return FALSE;

    if (NULL != pwszString)
    {
        dwLen = wcslen(pwszString);
        if (dwLen > 0x7ffe)   // String is too large
            return FALSE;
    }

    // Store the string.
    pLsaString->Buffer = (WCHAR*)pwszString;
    pLsaString->Length = (USHORT)dwLen * sizeof(WCHAR);
    pLsaString->MaximumLength = (USHORT)(dwLen + 1) * sizeof(WCHAR);

    return TRUE;
}

void AddPrivileges(PSID AccountSID, LSA_HANDLE PolicyHandle)
{
    LSA_UNICODE_STRING lucPrivilege;
    NTSTATUS ntsResult;

    // Create an LSA_UNICODE_STRING for the privilege names.
    if (!InitLsaString(&lucPrivilege, L"SeLockMemoryPrivilege"))
    {
        wprintf(L"Failed InitLsaString\n");
        return;
    }

    ntsResult = LsaAddAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID.
        &lucPrivilege, // The privileges.
        1              // Number of privileges.
    );
    if (ntsResult == STATUS_SUCCESS)
    {
        wprintf(L"Privilege added.\n");
    }
    else
    {
        wprintf(L"Privilege was not added - %lu \n",
            LsaNtStatusToWinError(ntsResult));
        std::cout << "Error message: " << GetErrorAsString(LsaNtStatusToWinError(ntsResult)) << std::endl;
    }
}


void enable_privilege()
{
    /*
    Get Sid from runnign powershell commands:
        $objUser = New-Object System.Security.Principal.NTAccount("aybas")
        $strSID = $objUser.Translate([System.Security.Principal.SecurityIdentifier])
        $strSID.Value
    There is probably a way through Win32 API, but that seems easier
    */
    LPCSTR strSid = "S-1-5-21-1122353133-3652626337-3931676656-1001";
    PSID sid;
    WCHAR SystemName[] = L"LAPTOP-UQEIU0OH";

    ConvertStringSidToSidA(strSid, &sid);
    AddPrivileges(sid, GetPolicyHandle(SystemName));
}
