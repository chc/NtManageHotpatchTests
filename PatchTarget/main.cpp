#include <windows.h>
#include <stdio.h>
extern "C" {
	int simple_function();
}
void dump_addresses() {
	HMODULE ntdll = GetModuleHandle("ntdll.dll");
	printf("ntdll: %p\n", ntdll);
	void* notify_proc = GetProcAddress(ntdll, "LdrHotPatchNotify");
	printf("LdrHotPatchNotify: %p\n", notify_proc);

	uint32_t* LdrpDebugFlags = (uint32_t*)((size_t)0x340008 + (size_t)ntdll);
	printf("LdrpDebugFlags: %08x\n", *LdrpDebugFlags);
	*LdrpDebugFlags = 0xFFFFFFFF;
	printf("LdrpDebugFlags: %08x\n", *LdrpDebugFlags);
}
int main() {
	dump_addresses();

	int last_result = 0;
	for (;;) {
		int result = simple_function();
		if (result != last_result) {
			printf("simple function: %d\n", result);
			last_result = result;
		}
		
		Sleep(1000);
	}
	return 0;
}