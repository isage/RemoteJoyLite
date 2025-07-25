/*------------------------------------------------------------------------------*/
/* hook_usb																		*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include <pspusb.h>
#include <pspusbbus.h>
#include <string.h>
#include "debug.h"
#include "usb.h"
#define IS_SELF
#include "hook_usb.h"
#undef IS_SELF
#include "hook.h"

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
int (*sceUsbStart_Func)( const char *, int, void * ) = NULL;
int (*sceUsbStop_Func)( const char *, int, void * ) = NULL;

/*------------------------------------------------------------------------------*/
/* MyUsbStart																	*/
/*------------------------------------------------------------------------------*/
static int MyUsbStart( const char *name, int args, void *argp )
{
	// black list all drivers except for the storage driver
	if(strcmp(name, "USBStor_Driver") == 0){
		#ifndef RELEASE
		debug_printf("%s: usb driver %s whitelisted\n", __func__, name);
		#endif
		UsbSuspend();
		return sceUsbStart_Func(name, args, argp);
	}
	#ifndef RELEASE
	debug_printf("%s: usb driver %s not whitelisted\n", __func__, name);
	#endif
	return -1;
}

/*------------------------------------------------------------------------------*/
/* MyUsbStop																	*/
/*------------------------------------------------------------------------------*/
static int MyUsbStop( const char *name, int args, void *argp )
{
	#ifndef RELEASE
	debug_printf("%s: usb driver %s\n", __func__, name);
	#endif

	int ret = sceUsbStop_Func(name, args, argp);
	if(strcmp(name, "USBStor_Driver") == 0){
		UsbResume();
	}
	return ret;
}

/*------------------------------------------------------------------------------*/
/* hookUsbFunc																	*/
/*------------------------------------------------------------------------------*/
#if 0
void hookUsbFunc( void )
{
	SceModule *module = sceKernelFindModuleByName( "sceUSB_Driver" );
	if ( module == NULL ){ return; }

	if ( sceUsbStart_Func == NULL ){
		sceUsbStart_Func = HookNidAddress( module, "sceUsb", 0xAE5DE6AF );
		void *hook_addr = HookSyscallAddress( sceUsbStart_Func );
		HookFuncSetting( hook_addr, MyUsbStart );
	}

	if ( sceUsbStop_Func == NULL ){
		sceUsbStop_Func = HookNidAddress( module, "sceUsb", 0xC2464FA0 );
		void *hook_addr = HookSyscallAddress( sceUsbStop_Func );
		HookFuncSetting( hook_addr, MyUsbStop );
	}
}
#endif

int usbModuleLoaded(){
	u32 loaded = sctrlHENFindFunction("sceUSB_Driver", "sceUsb", 0xAE5DE6AF);
	return loaded != NULL;
}

typedef int (*module_load_function_t)(const char *, int flags, SceKernelLMOption *load_options);
typedef int (*module_start_function_t)(SceUID modid, int args, void **argp, int *status, void *start_options);
void loadUsbModule(){
	module_load_function_t KernelLoadModule = (module_load_function_t)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForKernel", 0x977DE386);
	module_start_function_t KernelStartModule = (module_start_function_t)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForKernel", 0x50F0C1EC);

	EARLY_LOG("%s: KernelLoadModule 0x%x\n", __func__, KernelLoadModule);
	EARLY_LOG("%s: KernelStartModule 0x%x\n", __func__, KernelStartModule);

	SceUID modid = KernelLoadModule("flash0:/kd/usb.prx", 0, NULL);
	if (modid < 0){
		EARLY_LOG("%s: failed loading usb.prx, 0x%x, this is fatal\n", __func__, modid);
		return;
	}

	int status;
	int start_status = KernelStartModule(modid, 0, NULL, &status, NULL);
	if (start_status < 0){
		EARLY_LOG("%s: failed starting usb.prx, 0x%x, this is fatal\n", __func__, start_status);
		return;
	}

	EARLY_LOG("%s: usb.prx loaded\n", __func__);
}

void hookUsbFunc(){
	u32 UsbStart = GET_JUMP_TARGET(_lw((u32)sceUsbStart));
	u32 UsbStop = GET_JUMP_TARGET(_lw((u32)sceUsbStop));

	#define str(s) #s
	#define LOG_IMPORT(n) { \
		EARLY_LOG("%s: %s 0x%x\n", __func__, str(n), n); \
	}

	LOG_IMPORT(UsbStart);
	LOG_IMPORT(UsbStop);

	HIJACK_FUNCTION(UsbStart, MyUsbStart, sceUsbStart_Func);
	HIJACK_FUNCTION(UsbStop, MyUsbStop, sceUsbStop_Func);

	/*
	HIJACK_SYSCALL_STUB(sceUsbStart, MyUsbStart, sceUsbStart_Func);
	HIJACK_SYSCALL_STUB(sceUsbStop, MyUsbStop, sceUsbStop_Func);
	*/
}
