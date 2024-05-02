# NtManageHotpatch testing code

This project is testing code for the NtManageHotpatch syscall built into Windows.

The code is intended to replace the OriginalDLL code with the UpdatedDLL code. Currently for reasons unknown it is not working. It seems that the event callback in ntdll is being fired before the DLL is mapped into memory, but after it briefly maps and then unmaps due to the PatchCallout event failing.

To prepare the project you must compile it all, and run ImageSetHotpatchOffset against the UpdatedDLL so that the Load Config is updated with the hotpatch table offset. From there you can run the PatchTarget project on your target system, and then run PatchLoader against as well. Note that the OriginalDLL timestamp must be set correctly in the UpdatedDLL code.

You must also ensure that NtManageHotpatch is enabled in your kernel or else it will not work.

## Credits

[Signal Labs](https://signal-labs.com/windows-hotpatching-amp-process-injection/) for original POC which was a good base reference