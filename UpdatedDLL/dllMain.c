#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <winternl.h>


typedef struct _HOTPATCH_DEFINITION {
    IMAGE_HOT_PATCH_INFO hotpatchInfo;
    uint32_t alignment; //lame, find a better way
    IMAGE_HOT_PATCH_BASE hotpatchBase;
} HOTPATCH_DEFINITION;

const HOTPATCH_DEFINITION g_HotpatchDefinition = {
    {3,sizeof(IMAGE_HOT_PATCH_INFO) + sizeof(IMAGE_HOT_PATCH_BASE) + 4 + 4,1,sizeof(IMAGE_HOT_PATCH_INFO),1,0,0},
    sizeof(IMAGE_HOT_PATCH_BASE),
    {1,0,1714498260,0,0,0,0,0}
};

int simple_function() {
    return 666;
}

void CreateFileTest() {
    HANDLE hFile = 0;
    const wchar_t* str = L"\\??\\C:\\Code\\ITWORKS.txt";
    UNICODE_STRING path;
    OBJECT_ATTRIBUTES attributes;
    path.Buffer = (wchar_t*)str;
    path.Length = (USHORT)24 * sizeof(wchar_t);
    path.MaximumLength = path.Length + sizeof(wchar_t);
    InitializeObjectAttributes(&attributes,
        &path,
        OBJ_CASE_INSENSITIVE,
        0,
        0);
    IO_STATUS_BLOCK stat;
    NTSTATUS res = NtCreateFile(&hFile, GENERIC_ALL, &attributes, &stat, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE, NULL, 0);
    NtClose(hFile);
}

uint64_t WINAPI __PatchMainCallout__(uint32_t *unk1, uint32_t unk2) {
    //CreateFileTest();
    MessageBoxA(NULL, "Hello World", "Hello World", MB_OK);
    return 0;
}
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    MessageBoxA(NULL, "Hello World", "Hello World", MB_OK);
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != NULL)
        {
            break; // do not do cleanup if process termination scenario
        }

        // Perform any necessary cleanup.
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}