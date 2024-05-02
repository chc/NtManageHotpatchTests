#include <stdio.h>
#include <windows.h>
#include <winternl.h>

enum NtManageHotPatchOperation {
	OPERATION_LOAD_PATCH, //0
	OPERATION_UNLOAD_PATCH, //1
	OPERATION_QUERY_PATCHES, //2
	OPERATION_LOAD_PATCH_FOR_SID, //3
	OPERATION_UNLOAD_PATCH_FOR_SID, //4
	OPERATION_QUERY_PATCHES_FOR_SID, //5
	OPERATION_QUERY_PROCESS_ACTIVE_PATCHES, //6
	OPERATION_APPLY_IMAGE_HOTPATCH, //7
	OPERATION_QUERY_SINGLE_LOADED_PATCH, //8
	OPERATION_INIT_HOTPATCHING, //9
};
typedef NTSTATUS(__stdcall *fpNtManageHotPatch)(uint32_t operationId, void* info, uint32_t info_len, uint32_t* result_len);


typedef struct LOAD_HOTPATCH_INFO { //type 0, 3 - 104 bytes
	uint32_t version;
	UNICODE_STRING hotpatch_path;
	uint8_t target_sid[68];
	uint32_t target_checksum;
	uint32_t target_timestamp;
};

typedef struct APPLY_HOTPATCH_INFO { //type 7 - 32 bytes
	uint32_t version; //seems to be 1 always
	uint32_t flags; //1 = apply usermode patches?, 2 = apply kernel patches?, 4 = "do not disble extreme control flow guard"
	uint32_t unk1;
	uint32_t unk2;
	uint64_t unk3;
	uint64_t unk4;
};

fpNtManageHotPatch NtManageHotPatch;

void adjust_token() {
	TOKEN_PRIVILEGES tp;
	LUID luid;


	HANDLE hToken;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		printf("Failed to open process token\n");
		return;
	}

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		SE_LOAD_DRIVER_NAME,   // privilege to lookup 
		&luid)) {
		printf("Lookup token failed\n");
		return;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		printf("AdjustTokenPrivileges error: %u\n", GetLastError());
		return;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		printf("The token does not have the specified privilege. \n");
		return;
	}
}

void load_hotpatch() {
	LOAD_HOTPATCH_INFO info;
	printf("LOAD_HOTPATCH_INFO: %p\n", &info);

	//static wchar_t path[] = L"\\DosDevices\\C:\\dist\\UpdatedDLL.dll";
	static wchar_t path[] = L"\\??\\C:\\dist\\UpdatedDLL.dll";

	ZeroMemory(&info, sizeof(LOAD_HOTPATCH_INFO));

	uint32_t result = 0;

	RtlInitUnicodeString(&info.hotpatch_path, path);
	wprintf(L"Patch image: %ls\n", &path);
	

	info.version = 2;
	info.target_checksum = 0;
	info.target_timestamp = 0x662A7C63;

	NTSTATUS status = NtManageHotPatch(0, &info, sizeof(LOAD_HOTPATCH_INFO), &result);
	switch (status) {
	case 0xC0000061:
		printf("Missing SeLoadDriverPrivilege\n");
		break;
	case 0xC000000D:
		printf("Incorrect version\n");
		break;
	case 0xC00000BB:
		printf("Hotpatching not enabled in kernel\n");
		break;
	case 0:
		printf("Success\n");
		break;
	default:
		printf("Got status code: %08x\n", status);
		break;
	}
}
int main() {
	adjust_token();
	printf("LOAD_HOTPATCH_INFO size: %d\n", sizeof(LOAD_HOTPATCH_INFO));
	NtManageHotPatch = (fpNtManageHotPatch)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtManageHotPatch");
	printf("NtManageHotPatch: %p\n", NtManageHotPatch);

	load_hotpatch();

	return 0;
}
