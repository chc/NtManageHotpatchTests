#define  _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <imagehlp.h>
#include <stdio.h>
BOOL our_GetImageConfigInformation(
	PLOADED_IMAGE                LoadedImage,
	PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigInformation
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
	size_t hotpatch_offset = 0;
	hotpatch_offset = (size_t)&LoadConfigDirectoryAddress->HotPatchTableOffset - (size_t)LoadedImage->MappedAddress;
	printf("Hotpatch File Offset: %d\n", hotpatch_offset);

	memcpy(ImageConfigInformation, LoadConfigDirectoryAddress, sizeof(IMAGE_LOAD_CONFIG_DIRECTORY));
	return TRUE;
}
size_t determine_patch_table_offset(PLOADED_IMAGE img, PIMAGE_LOAD_CONFIG_DIRECTORY dic) {
	for (int i = 0; i < img->NumberOfSections; i++) {
		if (dic->HotPatchTableOffset > img->Sections[i].VirtualAddress && dic->HotPatchTableOffset < (img->Sections[i].VirtualAddress + img->Sections[i].SizeOfRawData)) {

			size_t result =  img->Sections[i].PointerToRawData + (dic->HotPatchTableOffset - img->Sections[i].VirtualAddress);
			return result;
		}

	}
	return 0;
}
int main() {
	//const char* img_path = "C:\\NtHotpatch\\dist\\kernelbase_patch_mod.dll";
	const char* img_path = "C:\\NtHotpatch\\dist\\UpdatedDLL.dll";
	PLOADED_IMAGE img;

	IMAGE_LOAD_CONFIG_DIRECTORY dic;
	ZeroMemory(&dic, sizeof(dic));
	dic.Size = sizeof(dic);

#if 1
	static LOADED_IMAGE ref_img;
	if (!MapAndLoad(img_path, NULL, &ref_img, TRUE, TRUE)) {
		printf("failed to map and load img - %08x\n", GetLastError());
		return 0;
	}
	else {
		img = &ref_img;
	}
#else
	img = ImageLoad(img_path, NULL);
	if (!img) {
		printf("failed to load img - %08x\n", GetLastError());
		return 0;
	}
#endif

	if (!our_GetImageConfigInformation(img, &dic)) {
		printf("Failed to get img config info - %08x\n", GetLastError());
	} 
	else {
		printf("Hotpatch: %d\n", dic.HotPatchTableOffset);
		/*if (!ImageUnload(img)) {
			printf("Failed to unload img\n");
			return 0;
		}*/
		FILE* fd = fopen(img_path, "rb");

		int patch_info_offset = determine_patch_table_offset(img, &dic);
		fseek(fd, patch_info_offset, SEEK_SET);

		IMAGE_HOT_PATCH_INFO info;
		ZeroMemory(&info, sizeof(info));
		fread(&info, sizeof(IMAGE_HOT_PATCH_INFO), 1, fd);
		
		printf("IMAGE_HOT_PATCH_INFO - size %d: \n", sizeof(IMAGE_HOT_PATCH_INFO));
		printf("\tVersion: %d\n", info.Version);
		printf("\tSize: %d\n", info.Size);
		printf("\tSequenceNumber: %d\n", info.SequenceNumber);
		printf("\tBaseImageList: %d\n", info.BaseImageList);
		printf("\tBaseImageCount: %d\n", info.BaseImageCount);
		if (info.Version >= 2) {
			printf("\tBufferOffset: %d\n", info.BufferOffset);
			if (info.Version >= 3) {
				printf("\tExtraPatchSize: %d\n", info.ExtraPatchSize);
			}
		}
		else {
			printf("unsupported patch info version\n");
			return 0;
		}

		fseek(fd, patch_info_offset + info.BaseImageList, SEEK_SET);
		while ((ftell(fd) & 4)) { //force 32 bit alignment
			uint8_t x;
			fread(&x, 1, 1, fd);
			printf("align... %02x\n", x);
			
		}
		for (int i = 0; i < info.BaseImageCount; i++) {
			IMAGE_HOT_PATCH_BASE base;
			fread(&base, sizeof(IMAGE_HOT_PATCH_BASE), 1, fd);
			printf("IMAGE_HOT_PATCH_BASE - size: %d\n", sizeof(IMAGE_HOT_PATCH_BASE));
			printf("\tSequenceNumber: %d\n", base.SequenceNumber);
			printf("\tFlags: %d\n", base.Flags);
			printf("\tOriginalTimeDateStamp: %d\n", base.OriginalTimeDateStamp);
			printf("\tOriginalCheckSum: %d\n", base.OriginalCheckSum);
			printf("\tCodeIntegrityInfo: %d\n", base.CodeIntegrityInfo);
			printf("\tCodeIntegritySize: %d\n", base.CodeIntegritySize);
			printf("\tPatchTable: %d\n", base.PatchTable);
			printf("\tBufferOffset: %d\n", base.BufferOffset);
			break;
		}

		

		fclose(fd);
	}

	return 0;
}