#define  _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <imagehlp.h>
#include <stdio.h>

struct _TOOL_INFO {
	const char* img_path;
	const char* img_map_path;
	const char* map_variable_name;
	uint64_t base_mask;
	
} g_ToolInfo;
BOOL our_GetImageConfigInformation(
	PLOADED_IMAGE                LoadedImage,
	PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigInformation,
	size_t *hotpatch_offset
)
{
	if (!LoadedImage || !ImageConfigInformation)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (LoadedImage->FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	ULONG LoadConfigDirectorySize;
	PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfigDirectoryAddress
		= (PIMAGE_LOAD_CONFIG_DIRECTORY)ImageDirectoryEntryToDataEx(
			LoadedImage->MappedAddress,
			FALSE,
			IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
			&LoadConfigDirectorySize,
			NULL
		);
	if (!LoadConfigDirectoryAddress)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}
	if (LoadConfigDirectorySize != sizeof(IMAGE_LOAD_CONFIG_DIRECTORY))
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}
	if (LoadConfigDirectoryAddress->Size > 0 && LoadConfigDirectoryAddress->Size < sizeof(IMAGE_LOAD_CONFIG_DIRECTORY))
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	*hotpatch_offset = (size_t)&LoadConfigDirectoryAddress->HotPatchTableOffset - (size_t)LoadedImage->MappedAddress;
	memcpy(ImageConfigInformation, LoadConfigDirectoryAddress, sizeof(IMAGE_LOAD_CONFIG_DIRECTORY));
	return TRUE;
}


size_t read_hotpatch_virtual_address(const char* path) {
	FILE* fd = fopen(path, "rb");

	int next_match_index = 0;

	int variable_len = strlen(g_ToolInfo.map_variable_name);

	bool in_match = false;

	char address_buffer[17] = {0};
	while (!feof(fd)) {
		
		char c = fgetc(fd);
		if (c == g_ToolInfo.map_variable_name[next_match_index]) {
			next_match_index++;
			if (next_match_index >= variable_len) {
				in_match = true;
				break;
			}
		}
		else {
			next_match_index = 0;
		}
	}

	
	bool got_alnum = 0;
	char* addr_buf = (char *) &address_buffer;
	if (in_match) {
		while (!feof(fd)) {
			char c = fgetc(fd);
			if (got_alnum) {
				if (isalnum(c)) {
					*addr_buf = c;
					addr_buf++;
				}
				else {
					break;
				}
			}
			else {
				if (isspace(c)) {
					continue;
				}
				else {
					got_alnum = 1;
				}
			}
		}
	}

	fclose(fd);

	if (in_match) {
		char* end;
		uint64_t address = strtoll((const char *) &address_buffer, &end, 16);		
		return (address & g_ToolInfo.base_mask);
	}	
	return 0;
}
int main() {
	g_ToolInfo.base_mask = ~0x0000000180000000;
	g_ToolInfo.img_path = "C:\\NtHotpatch\\dist\\UpdatedDLL.dll";
	g_ToolInfo.img_map_path = "C:\\NtHotpatch\\Project1\\ARM64\\Release\\UpdatedDLL.map";
	g_ToolInfo.map_variable_name = "g_HotpatchDefinition";

	PLOADED_IMAGE img;

	IMAGE_LOAD_CONFIG_DIRECTORY dic;
	ZeroMemory(&dic, sizeof(dic));
	dic.Size = sizeof(dic);

#if 1
	static LOADED_IMAGE ref_img;
	if (!MapAndLoad(g_ToolInfo.img_path, NULL, &ref_img, TRUE, TRUE)) {
		printf("failed to map and load img - %08x\n", GetLastError());
		return 0;
	}
	else {
		img = &ref_img;
	}
#else
	img = ImageLoad(g_ToolInfo.img_path, NULL);
	if (!img) {
		printf("failed to load img - %08x\n", GetLastError());
		return 0;
	}
#endif

	size_t hotpatch_info_offset = 0;
	if (!our_GetImageConfigInformation(img, &dic, &hotpatch_info_offset)) {
		printf("Failed to get img config info - %08x\n", GetLastError());
		if (!UnMapAndLoad(img)) {
			printf("Failed to unmap image\n");
			return -1;
		}
	}
	else {
		if (dic.HotPatchTableOffset != 0) {
			printf("Error: Hotpatch table offset already set: %08x\n", dic.HotPatchTableOffset);
			return -1;
		}
		printf("hotpatch_info_offset: %d\n", hotpatch_info_offset);

		size_t hotpatch_addr = (size_t)read_hotpatch_virtual_address(g_ToolInfo.img_map_path);
		uint32_t hotpatch_truncaddr = (uint32_t)hotpatch_addr;
		printf("hotpatch address: %04x\n", hotpatch_addr);

		if (!UnMapAndLoad(img)) {
			printf("Failed to unmap image\n");
			return -1;
		}

		FILE* fd = fopen(g_ToolInfo.img_path, "rb+");
		if (!fd) {
			printf("failed to open image for writing\n");
			return -1;
		}

		fseek(fd, hotpatch_info_offset, SEEK_SET);
		fwrite(&hotpatch_truncaddr, sizeof(DWORD), 1, fd);


		fclose(fd);
	}
	return 0;
}