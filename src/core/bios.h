#pragma once

#include <array>
#include <string>

#include "types.h"

class System;

class BIOS {
public:
    BIOS(System* system);
    void TraceFunction(u32 address, u32 index);
    void TraceAFunction(u32 index);

    void PutChar(s8 value);

private:
    template<u32 arg_count>
    void PrintFnWithArgs(const char* format_string);

    std::string stdout_buffer;

    System* sys = nullptr;
};

// http://problemkaputt.de/psx-spx.htm#kernelbios
static const std::array<const char*, 192> bios_functions_A {
    "FileOpen(filename,accessmode)",
    "FileSeek(fd,offset,seektype)",
    "FileRead(fd,dst,length)",
    "FileWrite(fd,src,length)",
    "FileClose(fd)",
    "FileIoctl(fd,cmd,arg)",
    "exit(exitcode)",
    "FileGetDeviceFlag(fd)",
    "FileGetc(fd)",
    "FilePutc(char,fd)",
    "todigit(char)",
    "atof(src)",
    "strtoul(src,src_end,base)",
    "strtol(src,src_end,base)",
    "abs(val)",
    "labs(val)",
    "atoi(src)",
    "atol(src)",
    "atob(src,num_dst)",
    "SaveState(buf)",
    "RestoreState(buf,param)",
    "strcat(dst,src)",
    "strncat(dst,src,maxlen)",
    "strcmp(str1,str2)",
    "strncmp(str1,str2,maxlen)",
    "strcpy(dst,src)",
    "strncpy(dst,src,maxlen)",
    "strlen(src)",
    "index(src,char)",
    "rindex(src,char)",
    "strchr(src,char)",     // exactly the same as "index"
    "strrchr(src,char)",    // exactly the same as "rindex"
    "strpbrk(src,list)",
    "strspn(src,list)",
    "strcspn(src,list)",
    "strtok(src,list)",      // use strtok(0,list) in further calls
    "strstr(str,substr)",    // buggy
    "toupper(char)",
    "tolower(char)",
    "bcopy(src,dst,len)",
    "bzero(dst,len)",
    "bcmp(ptr1,ptr2,len)",    // Bugged
    "memcpy(dst,src,len)",
    "memset(dst,fillbyte,len)",
    "memmove(dst,src,len)",     // Bugged
    "memcmp(src1,src2,len)",    // Bugged
    "memchr(src,scanbyte,len)",
    "rand()",
    "srand(seed)",
    "qsort(base,nel,width,callback)",
    "strtod(src,src_end)",    // Does NOT work - uses (ABSENT) cop1 !!!
    "malloc(size)",
    "free(buf)",
    "lsearch(key,base,nel,width,callback)",
    "bsearch(key,base,nel,width,callback)",
    "calloc(sizx,sizy)",           // SLOW!
    "realloc(old_buf,new_siz)",    // SLOW!
    "InitHeap(addr,size)",
    "SystemErrorExit(exitcode)",
    "or B(3Ch) std_in_getchar()",
    "or B(3Dh) std_out_putchar(char)",
    "or B(3Eh) std_in_gets(dst)",
    "or B(3Fh) std_out_puts(src)",
    "printf(txt,param1,param2,etc.)",
    "SystemErrorUnresolvedException()",
    "LoadExeHeader(filename,headerbuf)",
    "LoadExeFile(filename,headerbuf)",
    "DoExecute(headerbuf,param1,param2)",
    "FlushCache()",
    "init_a0_b0_c0_vectors",
    "GPU_dw(Xdst,Ydst,Xsiz,Ysiz,src)",
    "gpu_send_dma(Xdst,Ydst,Xsiz,Ysiz,src)",
    "SendGP1Command(gp1cmd)",
    "GPU_cw(gp0cmd)",      // send GP0 command word
    "GPU_cwp(src,num)",    // send GP0 command word and parameter words
    "send_gpu_linked_list(src)",
    "gpu_abort_dma()",
    "GetGPUStatus()",
    "gpu_sync()",
    "SystemError",
    "SystemError",
    "LoadAndExecute(filename,stackbase,stackoffset)",
    "SystemError ----OR---- GetSysSp()",
    "SystemError",
    "CdInit()",
    "_bu_init()",
    "CdRemove()",    // does NOT work due to SysDeqIntRP bug
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "dev_tty_init()",                                        // PS2: SystemError
    "dev_tty_open(fcb,and unused:path\\name,accessmode)",    // PS2: SystemError
    "dev_tty_in_out(fcb,cmd)",                               // PS2: SystemError
    "dev_tty_ioctl(fcb,cmd,arg)",                            // PS2: SystemError
    "dev_cd_open(fcb,path\\name,accessmode)",
    "dev_cd_read(fcb,dst,len)",
    "dev_cd_close(fcb)",
    "dev_cd_firstfile(fcb,path\\name,direntry)",
    "dev_cd_nextfile(fcb,direntry)",
    "dev_cd_chdir(fcb,path)",
    "dev_card_open(fcb,path\name,accessmode)",
    "dev_card_read(fcb,dst,len)",
    "dev_card_write(fcb,src,len)",
    "dev_card_close(fcb)",
    "dev_card_firstfile(fcb,path\\name,direntry)",
    "dev_card_nextfile(fcb,direntry)",
    "dev_card_erase(fcb,path\\name)",
    "dev_card_undelete(fcb,path\\name)",
    "dev_card_format(fcb)",
    "dev_card_rename(fcb1,path\\name1,fcb2,path\\name2)",
    "?   ;card ;[r4+18h]=00000000h",    // card_clear_error(fcb) or so
    "_bu_init()",
    "CdInit()",
    "CdRemove()",    // does NOT work due to SysDeqIntRP bug
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "CdAsyncSeekL(src)",
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "CdAsyncGetStatus(dst)",
    "return 0",    // DTL-H: Unknown?
    "CdAsyncReadSector(count,dst,mode)",
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "CdAsyncSetMode(mode)",
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?, or reportedly, CdStop (?)
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "return 0",    // DTL-H: Unknown?
    "CdromIoIrqFunc1()",
    "CdromDmaIrqFunc1()",
    "CdromIoIrqFunc2()",
    "CdromDmaIrqFunc2()",
    "CdromGetInt5errCode(dst1,dst2)",
    "CdInitSubFunc()",
    "AddCDROMDevice()",
    "AddMemCardDevice()",     // DTL-H: SystemError
    "AddDuartTtyDevice()",    // DTL-H: AddAdconsTtyDevice ;PS2: SystemError
    "AddDummyTtyDevice()",
    "SystemError",    // DTL-H: AddMessageWindowDevice
    "SystemError",    // DTL-H: AddCdromSimDevice
    "SetConf(num_EvCB,num_TCB,stacktop)",
    "GetConf(num_EvCB_dst,num_TCB_dst,stacktop_dst)",
    "SetCdromIrqAutoAbort(type,flag)",
    "SetMemSize(megabytes)",
    "WarmBoot()",
    "SystemErrorBootOrDiskFailure(type,errorcode)",
    "EnqueueCdIntr()",       // with prio=0 (fixed)
    "DequeueCdIntr()",       // does NOT work due to SysDeqIntRP bug
    "CdGetLbn(filename)",    // get 1st sector number (or garbage when not found)
    "CdReadSector(count,sector,buffer)",
    "CdGetStatus()",
    "bu_callback_okay()",
    "bu_callback_err_write()",
    "bu_callback_err_busy()",
    "bu_callback_err_eject()",
    "_card_info(port)",
    "_card_async_load_directory(port)",
    "set_card_auto_format(flag)",
    "bu_callback_err_prev_write()",
    "card_write_test(port)",    // CEX-1000: jump_to_00000000h
    "return 0",                 // CEX-1000: jump_to_00000000h
    "return 0",                 // CEX-1000: jump_to_00000000h
    "ioabort_raw(param)",       // CEX-1000: jump_to_00000000h
    "return 0",                 // CEX-1000: jump_to_00000000h
    "GetSystemInfo(index)",     // CEX-1000: jump_to_00000000h
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
};

static const std::array<const char*, 94> bios_functions_B = {
    "alloc_kernel_memory(size)",
    "free_kernel_memory(buf)",
    "init_timer(t,reload,flags)",
    "get_timer(t)",
    "enable_timer_irq(t)",
    "disable_timer_irq(t)",
    "restart_timer(t)",
    "DeliverEvent(class, spec)",
    "OpenEvent(class,spec,mode,func)",
    "CloseEvent(event)",
    "WaitEvent(event)",
    "TestEvent(event)",
    "EnableEvent(event)",
    "DisableEvent(event)",
    "OpenThread(reg_PC,reg_SP_FP,reg_GP)",
    "CloseThread(handle)",
    "ChangeThread(handle)",
    "jump_to_00000000h",
    "InitPad(buf1,siz1,buf2,siz2)",
    "StartPad()",
    "StopPad()",
    "OutdatedPadInitAndStart(type,button_dest,unused,unused)",
    "OutdatedPadGetButtons()",
    "ReturnFromException()",
    "SetDefaultExitFromException()",
    "SetCustomExitFromException(addr)",
    "SystemError",
    "SystemError",
    "SystemError",
    "SystemError",
    "SystemError",
    "SystemError",
    "UnDeliverEvent(class,spec)",
    "SystemError",
    "SystemError",
    "SystemError",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "SystemError",
    "SystemError",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "jump_to_00000000h",
    "FileOpen(filename,accessmode)",
    "FileSeek(fd,offset,seektype)",
    "FileRead(fd,dst,length)",
    "FileWrite(fd,src,length)",
    "FileClose(fd)",
    "FileIoctl(fd,cmd,arg)",
    "exit(exitcode)",
    "FileGetDeviceFlag(fd)",
    "FileGetc(fd)",
    "FilePutc(char,fd)",
    "std_in_getchar()",
    "std_out_putchar(char)",
    "std_in_gets(dst)",
    "std_out_puts(src)",
    "chdir(name)",
    "FormatDevice(devicename)",
    "firstfile(filename,direntry)",
    "nextfile(direntry)",
    "FileRename(old_filename,new_filename)",
    "FileDelete(filename)",
    "FileUndelete(filename)",
    "AddDevice(device_info)",    // subfunction for AddXxxDevice functions
    "RemoveDevice(device_name_lowercase)",
    "PrintInstalledDevices()",
    "InitCard(pad_enable)",    // uses/destroys k0/k1 !!!
    "StartCard()",
    "StopCard()",
    "_card_info_subfunc(port)",    // subfunction for "_card_info"
    "write_card_sector(port,sector,src)",
    "read_card_sector(port,sector,dst)",
    "allow_new_card()",
    "Krom2RawAdd(shiftjis_code)",
    "SystemError",
    "Krom2Offset(shiftjis_code)",
    "GetLastError()",
    "GetLastFileError(fd)",
    "GetC0Table",
    "GetB0Table",
    "get_bu_callback_port()",
    "testdevice(devicename)",
    "SystemError",
    "ChangeClearPad(int)",
    "get_card_status(slot)",
    "wait_card_status(slot)",
    // B(5Eh..FFh) N/A ;jump_to_00000000h    ;CEX-1000: B(5Eh..F6h) only
    // B(100h....) N/A ;garbage              ;CEX-1000: B(F7h.....) and up
};

static const std::array<const char*, 30> bios_functions_C = {
    "EnqueueTimerAndVblankIrqs(priority)",    // used with prio=1
    "EnqueueSyscallHandler(priority)",        // used with prio=0
    "SysEnqIntRP(priority,struc)",            // bugged, use with care
    "SysDeqIntRP(priority,struc)",            // bugged, use with care
    "get_free_EvCB_slot()",
    "get_free_TCB_slot()",
    "ExceptionHandler()",
    "InstallExceptionHandlers()",    // destroys/uses k0/k1
    "SysInitMemory(addr,size)",
    "SysInitKernelVariables()",
    "ChangeClearRCnt(t,flag)",
    "SystemError",
    "InitDefInt(priority)",    // used with prio=3
    "SetIrqAutoAck(irq,flag)",
    "return 0",    // DTL-H2000: dev_sio_init
    "return 0",    // DTL-H2000: dev_sio_open
    "return 0",    // DTL-H2000: dev_sio_in_out
    "return 0",    // DTL-H2000: dev_sio_ioctl
    "InstallDevices(ttyflag)",
    "FlushStdInOutPut()",
    "return 0",    // DTL-H2000: SystemError
    "tty_cdevinput(circ,char)",
    "tty_cdevscan()",
    "tty_circgetc(circ)",    // uses r5 as garbage txt for ioabort
    "tty_circputc(char,circ)",
    "ioabort(txt1,txt2)",
    "set_card_find_mode(mode)",    // 0=normal, 1=find deleted files
    "KernelRedirect(ttyflag)",     // PS2: ttyflag=1 causes SystemError
    "AdjustA0Table()",
    "get_card_find_mode()",
    // C(1Eh..7Fh) N/A ;jump_to_00000000h
    // C(80h.....) N/A ;mirrors to B(00h.....)
};