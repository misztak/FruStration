#include "bios.h"

#include "bus.h"
#include "common/asserts.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "system.h"

LOG_CHANNEL(BIOS);

BIOS::BIOS(System* system) : sys(system) {}

void BIOS::PutChar(s8 value) {
    if (value == '\n') {
        LogInfo("TTY: {}", stdout_buffer);
        stdout_buffer.clear();
    } else {
        stdout_buffer.push_back(value);
    }
}

void BIOS::TraceFunction(u32 address, u32 index) {
    if (address == 0xA0 && index < A_FUNCTION_COUNT) {
        TraceAFunction(index);
    } else if (address == 0xB0 && index < B_FUNCTION_COUNT) {
        TraceBFunction(index);
    } else if (address == 0xC0 && index < C_FUNCTION_COUNT) {
        TraceCFunction(index);
    } else {
        LogWarn("Invalid BIOS function 0x{:02X}:{}", address, index);
    }
}

// https://psx-spx.consoledev.net/kernelbios/#bios-function-summary
// caller passes the first 4 arguments in registers a0-a3
// arguments 5 and onwards are stored on the stack starting at sp+16
// TODO: write a custom printf version that reads data from PSX memory
#define FN_WITH_ARGS_0(format) LogTrace(format)
#define FN_WITH_ARGS_1(format) LogTrace(format, sys->cpu->gp.a0)
#define FN_WITH_ARGS_2(format) LogTrace(format, sys->cpu->gp.a0, sys->cpu->gp.a1)
#define FN_WITH_ARGS_3(format) LogTrace(format, sys->cpu->gp.a0, sys->cpu->gp.a1, sys->cpu->gp.a2)
#define FN_WITH_ARGS_4(format) LogTrace(format, sys->cpu->gp.a0, sys->cpu->gp.a1, sys->cpu->gp.a2, sys->cpu->gp.a3)
#define FN_WITH_ARGS_5(format) \
    LogTrace(format, sys->cpu->gp.a0, sys->cpu->gp.a1, sys->cpu->gp.a2, sys->cpu->gp.a3, arg_5)
#define FN_WITH_ARGS_6(format) \
    LogTrace(format, sys->cpu->gp.a0, sys->cpu->gp.a1, sys->cpu->gp.a2, sys->cpu->gp.a3, arg_5, arg_6)


void BIOS::TraceAFunction(u32 index) {
    const u32 arg_5 = sys->bus->Peek32(sys->cpu->gp.sp + 16);
    
    switch (index) {
        case 0x00: FN_WITH_ARGS_2("open(filename=0x{:08x},accessmode=0x{:8x})"); break;
        case 0x01: FN_WITH_ARGS_3("lseek(fd={},offset={},seektype={})"); break;
        case 0x02: FN_WITH_ARGS_3("read(fd{},dst=0x{:08x},length={})"); break;
        case 0x03: FN_WITH_ARGS_3("write(fd={},src=0x{:08x},length={})"); break;
        case 0x04: FN_WITH_ARGS_1("close(fd={})"); break;
        case 0x05: FN_WITH_ARGS_3("ioctl(fd={},cmd={},arg={})"); break;
        case 0x06: FN_WITH_ARGS_1("exit(exitcode={})"); break;
        case 0x07: FN_WITH_ARGS_1("isatty(fd={})"); break;
        case 0x08: FN_WITH_ARGS_1("getc(fd={})"); break;
        case 0x09: FN_WITH_ARGS_2("putc(char={:c},fd={})"); break;
        case 0x0A: FN_WITH_ARGS_1("todigit(char={:c})"); break;
        case 0x0B: FN_WITH_ARGS_1("atof(src=0x{:08x})"); break;
        case 0x0C: FN_WITH_ARGS_3("strtoul(src=0x{:08x},src_end=0x{:08x},base={})"); break;
        case 0x0D: FN_WITH_ARGS_3("strtol(src=0x{:08x},src_end=0x{:08x},base={})"); break;
        case 0x0E: FN_WITH_ARGS_1("abs(val={})"); break;
        case 0x0F: FN_WITH_ARGS_1("labs(val={})"); break;
        case 0x10: FN_WITH_ARGS_1("atoi(src=0x{:08x})"); break;
        case 0x11: FN_WITH_ARGS_1("atol(src=0x{:08x})"); break;
        case 0x12: FN_WITH_ARGS_2("atob(src=0x{:08x},num_dst=0x{:08x})"); break;
        case 0x13: FN_WITH_ARGS_1("setjmp(buf=0x{:08x})"); break;
        case 0x14: FN_WITH_ARGS_2("longjmp(buf=0x{:08x},param={})"); break;
        case 0x15: FN_WITH_ARGS_2("strcat(dst=0x{:08x},src=0x{:08x})"); break;
        case 0x16: FN_WITH_ARGS_3("strncat(dst=0x{:08x},src=0x{:08x},maxlen={})"); break;
        case 0x17: FN_WITH_ARGS_2("strcmp(str1=0x{:08x},str=0x{:08x})"); break;
        case 0x18: FN_WITH_ARGS_3("strncmp(str1=0x{:08x},str2=0x{:08x},maxlen={})"); break;
        case 0x19: FN_WITH_ARGS_2("strcpy(dst=0x{:08x},src=0x{:08x})"); break;
        case 0x1A: FN_WITH_ARGS_3("strncpy(dst=0x{:08x},src=0x{:08x},maxlen={})"); break;
        case 0x1B: FN_WITH_ARGS_1("strlen(src=0x{:08x})"); break;
        case 0x1C: FN_WITH_ARGS_2("index(src=0x{:08x},char={:c})"); break;
        case 0x1D: FN_WITH_ARGS_2("rindex(src=0x{:08x},char={:c})"); break;
        case 0x1E: FN_WITH_ARGS_2("strchr(src=0x{:08x},char={:c})"); break;
        case 0x1F: FN_WITH_ARGS_2("strrchr(src=0x{:08x},char={:c})"); break;
        case 0x20: FN_WITH_ARGS_2("strpbrk(src=0x{:08x},list=0x{:08x})"); break;
        case 0x21: FN_WITH_ARGS_2("strspn(src=0x{:08x},list=0x{:08x})"); break;
        case 0x22: FN_WITH_ARGS_2("strcspn(src=0x{:08x},list=0x{:08x})"); break;
        case 0x23: FN_WITH_ARGS_2("strtok(src=0x{:08x},list=0x{:08x})"); break;
        case 0x24: FN_WITH_ARGS_2("strstr(str=0x{:08x},substr=0x{:08x})"); break;
        case 0x25: FN_WITH_ARGS_1("toupper(char={:c})"); break;
        case 0x26: FN_WITH_ARGS_1("tolower(char={:c})"); break;
        case 0x27: FN_WITH_ARGS_3("bcopy(src=0x{:08x},dst=0x{:08x},len={})"); break;
        case 0x28: FN_WITH_ARGS_2("bzero(dst=0x{:08x},len={})"); break;
        case 0x29: FN_WITH_ARGS_3("bcmp(ptr10x{:08x},ptr20x{:08x},len0x{:08x})"); break;
        case 0x2A: FN_WITH_ARGS_3("memcpy(dst=0x{:08x},src=0x{:08x},len={})"); break;
        case 0x2B: FN_WITH_ARGS_3("memset(dst=0x{:08x},fillbyte=0x{:x},len={})"); break;
        case 0x2C: FN_WITH_ARGS_3("memmove(dst=0x{:08x},src=0x{:08x},len={})"); break;
        case 0x2D: FN_WITH_ARGS_3("memcmp(src1=0x{:08x},src2=0x{:08x},len={})"); break;
        case 0x2E: FN_WITH_ARGS_3("memchr(src=0x{:08x},scanbyte=0x{:x},len={})"); break;
        case 0x2F: FN_WITH_ARGS_0("rand()"); break;
        case 0x30: FN_WITH_ARGS_1("srand(seed={})"); break;
        case 0x31: FN_WITH_ARGS_4("qsort(base={},nel={},width={},callback=0x{:08x})"); break;
        case 0x32: FN_WITH_ARGS_2("strtod(src=0x{:08x},src_end=0x{:08x})"); break;
        case 0x33: FN_WITH_ARGS_1("malloc(size={})"); break;
        case 0x34: FN_WITH_ARGS_1("free(buf=0x{:08x})"); break;
        case 0x35: FN_WITH_ARGS_5("lsearch(key={},base={},nel={},width={},callback=0x{:08x})"); break;
        case 0x36: FN_WITH_ARGS_5("bsearch(key={},base={},nel={},width={},callback=0x{:08x})"); break;
        case 0x37: FN_WITH_ARGS_2("calloc(sizx={},sizy={})"); break;
        case 0x38: FN_WITH_ARGS_2("realloc(old_buf=0x{:08x},new_siz={})"); break;
        case 0x39: FN_WITH_ARGS_2("InitHeap(addr=0x{:08x},size={})"); break;
        case 0x3A: FN_WITH_ARGS_1("_exit(exitcode={})"); break;
        case 0x3B: FN_WITH_ARGS_0("getchar()"); break;
        //case 0x3C: FN_WITH_ARGS_1("putchar(char={:c})"); break;
        case 0x3C: break; // ignore putchar calls because we trace TTY output separately
        case 0x3D: FN_WITH_ARGS_1("gets(dst=0x{:08x})"); break;
        case 0x3E: FN_WITH_ARGS_1("puts(src=0x{:08x})"); break;
        case 0x3F: FN_WITH_ARGS_2("printf(txt=0x{:08x},arg1=0x{:08x},args...)"); break;
        case 0x40: FN_WITH_ARGS_0("SystemErrorUnresolvedException()"); break;
        case 0x41: FN_WITH_ARGS_2("LoadTest(filename=0x{:08x},headerbuf=0x{:08x})"); break;
        case 0x42: FN_WITH_ARGS_2("Load(filename=0x{:08x},headerbuf=0x{:08x})"); break;
        case 0x43: FN_WITH_ARGS_3("Exec(headerbuf=0x{:08x},param1={},param2={})"); break;
        case 0x44: FN_WITH_ARGS_0("FlushCache()"); break;
        case 0x45: FN_WITH_ARGS_0("init_a0_b0_c0_vectors"); break;
        case 0x46: FN_WITH_ARGS_5("GPU_dw(Xdst={},Ydst={},Xsiz={},Ysiz={},src=0x{:08x})"); break;
        case 0x47: FN_WITH_ARGS_5("gpu_send_dma(Xdst={},Ydst={},Xsiz={},Ysiz={},src=0x{:08x})"); break;
        case 0x48: FN_WITH_ARGS_1("SendGP1Command(gp1cmd=0x{:08x})"); break;
        case 0x49: FN_WITH_ARGS_1("GPU_cw(gp0cmd=0x{:08x})"); break;
        case 0x4A: FN_WITH_ARGS_2("GPU_cwp(src=0x{:08x},num={})"); break;
        case 0x4B: FN_WITH_ARGS_1("send_gpu_linked_list(src=0x{:08x})"); break;
        case 0x4C: FN_WITH_ARGS_0("gpu_abort_dma()"); break;
        case 0x4D: FN_WITH_ARGS_0("GetGPUStatus()"); break;
        case 0x4E: FN_WITH_ARGS_0("gpu_sync()"); break;
        case 0x4F: FN_WITH_ARGS_0("SystemError"); break;
        case 0x50: FN_WITH_ARGS_0("SystemError"); break;
        case 0x51: FN_WITH_ARGS_3("LoadExec(filename=0x{:08x},stackbase=0x{:08x},stackoffset=0x{:x})"); break;
        case 0x52: FN_WITH_ARGS_0("GetSysSp"); break;
        case 0x53: FN_WITH_ARGS_0("SystemError"); break;
        case 0x54: FN_WITH_ARGS_0("_96_init()"); break;
        case 0x55: FN_WITH_ARGS_0("_bu_init()"); break;
        case 0x56: FN_WITH_ARGS_0("_96_remove()"); break;
        case 0x57: FN_WITH_ARGS_0("return 0"); break;
        case 0x58: FN_WITH_ARGS_0("return 0"); break;
        case 0x59: FN_WITH_ARGS_0("return 0"); break;
        case 0x5A: FN_WITH_ARGS_0("return 0"); break;
        case 0x5B: FN_WITH_ARGS_0("dev_tty_init()"); break;
        case 0x5C: FN_WITH_ARGS_3("dev_tty_open(fcb=0x{:08x},unused:'path/name'=0x{:08x},accessmode={})"); break;
        case 0x5D: FN_WITH_ARGS_2("dev_tty_in_out(fcb=0x{:08x},cmd={})"); break;
        case 0x5E: FN_WITH_ARGS_3("dev_tty_ioctl(fcb=0x{:08x},cmd={},arg={})"); break;
        case 0x5F: FN_WITH_ARGS_3("dev_cd_open(fcb=0x{:08x},'path/name'=0x{:08x},accessmode={})"); break;
        case 0x60: FN_WITH_ARGS_3("dev_cd_read(fcb=0x{:08x},dst=0x{:08x},len={})"); break;
        case 0x61: FN_WITH_ARGS_1("dev_cd_close(fcb=0x{:08x})"); break;
        case 0x62: FN_WITH_ARGS_3("dev_cd_firstfile(fcb=0x{:08x},'path/name'=0x{:08x},direntry==0x{:08x})"); break;
        case 0x63: FN_WITH_ARGS_2("dev_cd_nextfile(fcb=0x{:08x},direntry=0x{:08x})"); break;
        case 0x64: FN_WITH_ARGS_2("dev_cd_chdir(fcb=0x{:08x},'path'=0x{:08x})"); break;
        case 0x65: FN_WITH_ARGS_3("dev_card_open(fcb=0x{:08x},'path/name'=0x{:08x},accessmode={})"); break;
        case 0x66: FN_WITH_ARGS_3("dev_card_read(fcb=0x{:08x},dst=0x{:08x},len={})"); break;
        case 0x67: FN_WITH_ARGS_3("dev_card_write(fcb=0x{:08x},src=0x{:08x},len={})"); break;
        case 0x68: FN_WITH_ARGS_1("dev_card_close(fcb=0x{:08x})"); break;
        case 0x69: FN_WITH_ARGS_3("dev_card_firstfile(fcb=0x{:08x},'path/name'=0x{:08x},direntry=0x{:08x})"); break;
        case 0x6A: FN_WITH_ARGS_2("dev_card_nextfile(fcb=0x{:08x},direntry=0x{:08x})"); break;
        case 0x6B: FN_WITH_ARGS_2("dev_card_erase(fcb=0x{:08x},'path/name'=0x{:08x})"); break;
        case 0x6C: FN_WITH_ARGS_2("dev_card_undelete(fcb=0x{:08x},'path/name'=0x{:08x})"); break;
        case 0x6D: FN_WITH_ARGS_1("dev_card_format(fcb=0x{:08x})"); break;
        case 0x6E: FN_WITH_ARGS_4("dev_card_rename(fcb1=0x{:08x},'path/name1'=0x{:08x},fcb2=0x{:08x},'path/name2'=0x{:08x})"); break;
        case 0x6F: FN_WITH_ARGS_1("card_clear_error(fcb=0x{:08x})"); break;
        case 0x70: FN_WITH_ARGS_0("_bu_init()"); break;
        case 0x71: FN_WITH_ARGS_0("_96_init()"); break;
        case 0x72: FN_WITH_ARGS_0("_96_remove()"); break;
        case 0x73: FN_WITH_ARGS_0("return 0"); break;
        case 0x74: FN_WITH_ARGS_0("return 0"); break;
        case 0x75: FN_WITH_ARGS_0("return 0"); break;
        case 0x76: FN_WITH_ARGS_0("return 0"); break;
        case 0x77: FN_WITH_ARGS_0("return 0"); break;
        case 0x78: FN_WITH_ARGS_1("CdAsyncSeekL(src=0x{:08x})"); break;
        case 0x79: FN_WITH_ARGS_0("return 0"); break;
        case 0x7A: FN_WITH_ARGS_0("return 0"); break;
        case 0x7B: FN_WITH_ARGS_0("return 0"); break;
        case 0x7C: FN_WITH_ARGS_1("CdAsyncGetStatus(dst=0x{:08x})"); break;
        case 0x7D: FN_WITH_ARGS_0("return 0"); break;
        case 0x7E: FN_WITH_ARGS_3("CdAsyncReadSector(count={},dst=0x{:08x},mode={})"); break;
        case 0x7F: FN_WITH_ARGS_0("return 0"); break;
        case 0x80: FN_WITH_ARGS_0("return 0"); break;
        case 0x81: FN_WITH_ARGS_1("CdAsyncSetMode(mode={})"); break;
        case 0x82: FN_WITH_ARGS_0("return 0"); break;
        case 0x83: FN_WITH_ARGS_0("return 0"); break;
        case 0x84: FN_WITH_ARGS_0("return 0"); break;
        case 0x85: FN_WITH_ARGS_0("return 0"); break;
        case 0x86: FN_WITH_ARGS_0("return 0"); break;
        case 0x87: FN_WITH_ARGS_0("return 0"); break;
        case 0x88: FN_WITH_ARGS_0("return 0"); break;
        case 0x89: FN_WITH_ARGS_0("return 0"); break;
        case 0x8A: FN_WITH_ARGS_0("return 0"); break;
        case 0x8B: FN_WITH_ARGS_0("return 0"); break;
        case 0x8C: FN_WITH_ARGS_0("return 0"); break;
        case 0x8D: FN_WITH_ARGS_0("return 0"); break;
        case 0x8E: FN_WITH_ARGS_0("return 0"); break;
        case 0x8F: FN_WITH_ARGS_0("return 0"); break;
        case 0x90: FN_WITH_ARGS_0("CdromIoIrqFunc1()"); break;
        case 0x91: FN_WITH_ARGS_0("CdromDmaIrqFunc1()"); break;
        case 0x92: FN_WITH_ARGS_0("CdromIoIrqFunc2()"); break;
        case 0x93: FN_WITH_ARGS_0("CdromDmaIrqFunc2()"); break;
        case 0x94: FN_WITH_ARGS_2("CdromGetInt5errCode(dst1=0x{:08x},dst2=0x{:08x})"); break;
        case 0x95: FN_WITH_ARGS_0("CdInitSubFunc()"); break;
        case 0x96: FN_WITH_ARGS_0("AddCDROMDevice()"); break;
        case 0x97: FN_WITH_ARGS_0("AddMemCardDevice()"); break;
        case 0x98: FN_WITH_ARGS_0("AddDuartTtyDevice()"); break;
        case 0x99: FN_WITH_ARGS_0("add_nullcon_driver()"); break;
        case 0x9A: FN_WITH_ARGS_0("SystemError"); break;
        case 0x9B: FN_WITH_ARGS_0("SystemError"); break;
        case 0x9C: FN_WITH_ARGS_3("SetConf(num_EvCB={},num_TCB={},stacktop=0x{:08x})"); break;
        case 0x9D: FN_WITH_ARGS_3("GetConf(num_EvCB_dst={},num_TCB_dst={},stacktop_dst=0x{:08x})"); break;
        case 0x9E: FN_WITH_ARGS_2("SetCdromIrqAutoAbort(type={},flag={})"); break;
        case 0x9F: FN_WITH_ARGS_1("SetMem(megabytes={})"); break;
        case 0xA0: FN_WITH_ARGS_0("_boot()"); break;
        case 0xA1: FN_WITH_ARGS_2("SystemError(type={},errorcode={})"); break;
        case 0xA2: FN_WITH_ARGS_0("EnqueueCdIntr()"); break;
        case 0xA3: FN_WITH_ARGS_0("DequeueCdIntr()"); break;
        case 0xA4: FN_WITH_ARGS_1("CdGetLbn(filename=0x{:08x})"); break;
        case 0xA5: FN_WITH_ARGS_3("CdReadSector(count={},sector={},buffer=0x{:08x})"); break;
        case 0xA6: FN_WITH_ARGS_0("CdGetStatus()"); break;
        case 0xA7: FN_WITH_ARGS_0("bufs_cb_0()"); break;
        case 0xA8: FN_WITH_ARGS_0("bufs_cb_1()"); break;
        case 0xA9: FN_WITH_ARGS_0("bufs_cb_2()"); break;
        case 0xAA: FN_WITH_ARGS_0("bufs_cb_3()"); break;
        case 0xAB: FN_WITH_ARGS_1("_card_info(port={})"); break;
        case 0xAC: FN_WITH_ARGS_1("_card_load(port={})"); break;
        case 0xAD: FN_WITH_ARGS_1("_card_auto(flag={})"); break;
        case 0xAE: FN_WITH_ARGS_0("bufs_cb_4()"); break;
        case 0xAF: FN_WITH_ARGS_1("card_write_test(port={})"); break;
        case 0xB0: FN_WITH_ARGS_0("return 0"); break;
        case 0xB1: FN_WITH_ARGS_0("return 0"); break;
        case 0xB2: FN_WITH_ARGS_1("ioabort_raw(param={})"); break;
        case 0xB3: FN_WITH_ARGS_0("return 0"); break;
        case 0xB4: FN_WITH_ARGS_1("GetSystemInfo(index={})"); break;
        default: LogWarn("Invalid BIOS A function at index {}", index);
    }
}

void BIOS::TraceBFunction(u32 index) {
    switch (index) {
        case 0x00: FN_WITH_ARGS_1("alloc_kernel_memory(size=0x{:0x})"); break;
        case 0x01: FN_WITH_ARGS_1("free_kernel_memory(buf=0x{:08x})"); break;
        case 0x02: FN_WITH_ARGS_3("init_timer(t={},reload={},flags={})"); break;
        case 0x03: FN_WITH_ARGS_1("get_timer(t={})"); break;
        case 0x04: FN_WITH_ARGS_1("enable_timer_irq(t={})"); break;
        case 0x05: FN_WITH_ARGS_1("disable_timer_irq(t={})"); break;
        case 0x06: FN_WITH_ARGS_1("restart_timer(t={})"); break;
        case 0x07: FN_WITH_ARGS_2("DeliverEvent(class={}, spec={})"); break;
        case 0x08: FN_WITH_ARGS_4("OpenEvent(class={},spec={},mode={},func=0x{:08x})"); break;
        case 0x09: FN_WITH_ARGS_1("CloseEvent(event={})"); break;
        case 0x0A: FN_WITH_ARGS_1("WaitEvent(event={})"); break;
        case 0x0B: FN_WITH_ARGS_1("TestEvent(event={})"); break;
        case 0x0C: FN_WITH_ARGS_1("EnableEvent(event={})"); break;
        case 0x0D: FN_WITH_ARGS_1("DisableEvent(event={})"); break;
        case 0x0E: FN_WITH_ARGS_3("OpenTh(reg_PC=0x{:08x},reg_SP_FP=0x{:08x},reg_GP=0x{:08x})"); break;
        case 0x0F: FN_WITH_ARGS_1("CloseTh(handle={})"); break;
        case 0x10: FN_WITH_ARGS_1("ChangeTh(handle={})"); break;
        case 0x11: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x12: FN_WITH_ARGS_4("InitPAD2(buf1=0x{:08x},siz1={},buf2=0x{:08x},siz2={})"); break;
        case 0x13: FN_WITH_ARGS_0("StartPAD2()"); break;
        case 0x14: FN_WITH_ARGS_0("StopPAD2()"); break;
        case 0x15: FN_WITH_ARGS_2("PAD_init2(type={},button_dest=0x{:08x},unused,unused)"); break;
        case 0x16: FN_WITH_ARGS_0("PAD_dr()"); break;
        case 0x17: FN_WITH_ARGS_0("ReturnFromException()"); break;
        case 0x18: FN_WITH_ARGS_0("ResetEntryInt()"); break;
        case 0x19: FN_WITH_ARGS_0("HookEntryInt(addr)"); break;
        case 0x1A: FN_WITH_ARGS_0("SystemError"); break;
        case 0x1B: FN_WITH_ARGS_0("SystemError"); break;
        case 0x1C: FN_WITH_ARGS_0("SystemError"); break;
        case 0x1D: FN_WITH_ARGS_0("SystemError"); break;
        case 0x1E: FN_WITH_ARGS_0("SystemError"); break;
        case 0x1F: FN_WITH_ARGS_0("SystemError"); break;
        case 0x20: FN_WITH_ARGS_2("UnDeliverEvent(class=0x{:08x},spec=0x{:08x})"); break;
        case 0x21: FN_WITH_ARGS_0("SystemError"); break;
        case 0x22: FN_WITH_ARGS_0("SystemError"); break;
        case 0x23: FN_WITH_ARGS_0("SystemError"); break;
        case 0x24: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x25: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x26: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x27: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x28: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x29: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x2A: FN_WITH_ARGS_0("SystemError"); break;
        case 0x2B: FN_WITH_ARGS_0("SystemError"); break;
        case 0x2C: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x2D: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x2E: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x2F: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x30: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x31: FN_WITH_ARGS_0("jump_to_00000000h"); break;
        case 0x32: FN_WITH_ARGS_2("open(filename=0x{:08x},accessmode={})"); break;
        case 0x33: FN_WITH_ARGS_3("lseek(fd={},offset={},seektype={})"); break;
        case 0x34: FN_WITH_ARGS_3("read(fd={},dst=0x{:08x},length={})"); break;
        case 0x35: FN_WITH_ARGS_3("write(fd={},src=0x{:08x},length={})"); break;
        case 0x36: FN_WITH_ARGS_1("close(fd={})"); break;
        case 0x37: FN_WITH_ARGS_3("ioctl(fd={},cmd={},arg={})"); break;
        case 0x38: FN_WITH_ARGS_1("exit(exitcode={})"); break;
        case 0x39: FN_WITH_ARGS_1("isatty(fd={})"); break;
        case 0x3A: FN_WITH_ARGS_1("getc(fd={})"); break;
        case 0x3B: FN_WITH_ARGS_2("putc(char={:c},fd={})"); break;
        case 0x3C: FN_WITH_ARGS_0("getchar()"); break;
        //case 0x3D: FN_WITH_ARGS_1("putchar(char={:c})"); break;
        case 0x3D: break;
        case 0x3E: FN_WITH_ARGS_1("gets(dst=0x{:08x})"); break;
        case 0x3F: FN_WITH_ARGS_1("puts(src=0x{:08x})"); break;
        case 0x40: FN_WITH_ARGS_1("cd(name=0x{:08x})"); break;
        case 0x41: FN_WITH_ARGS_1("format(devicename=0x{:08x})"); break;
        case 0x42: FN_WITH_ARGS_2("firstfile2(filename=0x{:08x},direntry=0x{:08x})"); break;
        case 0x43: FN_WITH_ARGS_1("nextfile(direntry=0x{:08x})"); break;
        case 0x44: FN_WITH_ARGS_2("rename(old_filename=0x{:08x},new_filename=0x{:08x})"); break;
        case 0x45: FN_WITH_ARGS_1("erase(filename=0x{:08x})"); break;
        case 0x46: FN_WITH_ARGS_1("undelete(filename=0x{:08x})"); break;
        case 0x47: FN_WITH_ARGS_1("AddDrv(device_info={})"); break;
        case 0x48: FN_WITH_ARGS_1("DelDrv(device_name_lowercase=0x{:08x})"); break;
        case 0x49: FN_WITH_ARGS_0("PrintInstalledDevices()"); break;
        case 0x4A: FN_WITH_ARGS_1("InitCARD2(pad_enable={})"); break;
        case 0x4B: FN_WITH_ARGS_0("StartCARD2()"); break;
        case 0x4C: FN_WITH_ARGS_0("StopCARD2()"); break;
        case 0x4D: FN_WITH_ARGS_1("_card_info_subfunc(port={})"); break;
        case 0x4E: FN_WITH_ARGS_3("_card_write(port={},sector={},src=0x{:08x})"); break;
        case 0x4F: FN_WITH_ARGS_3("_card_read(port={},sector={},dst=0x{:08x})"); break;
        case 0x50: FN_WITH_ARGS_0("_new_card()"); break;
        case 0x51: FN_WITH_ARGS_1("Krom2RawAdd(shiftjis_code=0x{:08x})"); break;
        case 0x52: FN_WITH_ARGS_0("SystemError"); break;
        case 0x53: FN_WITH_ARGS_1("Krom2Offset(shiftjis_code=0x{:08x})"); break;
        case 0x54: FN_WITH_ARGS_0("_get_errno()"); break;
        case 0x55: FN_WITH_ARGS_1("_get_error(fd={})"); break;
        case 0x56: FN_WITH_ARGS_0("GetC0Table"); break;
        case 0x57: FN_WITH_ARGS_0("GetB0Table"); break;
        case 0x58: FN_WITH_ARGS_0("_card_chan()"); break;
        case 0x59: FN_WITH_ARGS_1("testdevice(devicename=0x{:08x})"); break;
        case 0x5A: FN_WITH_ARGS_0("SystemError"); break;
        case 0x5B: FN_WITH_ARGS_1("ChangeClearPAD(int={})"); break;
        case 0x5C: FN_WITH_ARGS_1("_card_status(slot={})"); break;
        case 0x5D: FN_WITH_ARGS_1("_card_wait(slot={})"); break;
        default: LogWarn("Invalid BIOS B function at index {}", index);
    }
}

void BIOS::TraceCFunction(u32 index) {
    switch (index) {
        case 0x00: FN_WITH_ARGS_1("EnqueueTimerAndVblankIrqs(priority={})"); break;
        case 0x01: FN_WITH_ARGS_1("EnqueueSyscallHandler(priority={})"); break;
        case 0x02: FN_WITH_ARGS_2("SysEnqIntRP(priority={},struc=0x{:08x})"); break;
        case 0x03: FN_WITH_ARGS_2("SysDeqIntRP(priority={},struc=0x{:08x})"); break;
        case 0x04: FN_WITH_ARGS_0("get_free_EvCB_slot()"); break;
        case 0x05: FN_WITH_ARGS_0("get_free_TCB_slot()"); break;
        case 0x06: FN_WITH_ARGS_0("ExceptionHandler()"); break;
        case 0x07: FN_WITH_ARGS_0("InstallExceptionHandlers()"); break;
        case 0x08: FN_WITH_ARGS_2("SysInitMemory(addr=0x{:08x},size={})"); break;
        case 0x09: FN_WITH_ARGS_0("SysInitKernelVariables()"); break;
        case 0x0A: FN_WITH_ARGS_2("ChangeClearRCnt(t={},flag={})"); break;
        case 0x0B: FN_WITH_ARGS_0("SystemError"); break;
        case 0x0C: FN_WITH_ARGS_1("InitDefInt(priority={})"); break;
        case 0x0D: FN_WITH_ARGS_2("SetIrqAutoAck(irq={},flag={})"); break;
        case 0x0E: FN_WITH_ARGS_0("return 0"); break;
        case 0x0F: FN_WITH_ARGS_0("return 0"); break;
        case 0x10: FN_WITH_ARGS_0("return 0"); break;
        case 0x11: FN_WITH_ARGS_0("return 0"); break;
        case 0x12: FN_WITH_ARGS_1("InstallDevices(ttyflag=0x{:x})"); break;
        case 0x13: FN_WITH_ARGS_0("FlushStdInOutPut()"); break;
        case 0x14: FN_WITH_ARGS_0("return 0"); break;
        case 0x15: FN_WITH_ARGS_2("_cdevinput(circ=0x{:08x},char={:c})"); break;
        case 0x16: FN_WITH_ARGS_0("_cdevscan()"); break;
        case 0x17: FN_WITH_ARGS_1("_circgetc(circ=0x{:08x})"); break;
        case 0x18: FN_WITH_ARGS_2("_circputc(char={:c},circ=0x{:08x})"); break;
        case 0x19: FN_WITH_ARGS_2("_ioabort(txt1=0x{:08x},txt2=0x{:08x})"); break;
        case 0x1A: FN_WITH_ARGS_1("set_card_find_mode(mode={})"); break;
        case 0x1B: FN_WITH_ARGS_1("KernelRedirect(ttyflag={})"); break;
        case 0x1C: FN_WITH_ARGS_0("AdjustA0Table()"); break;
        case 0x1D: FN_WITH_ARGS_0("get_card_find_mode()"); break;
        default: LogWarn("Invalid BIOS C function at index {}", index);
    }
}

#undef FN_WITH_ARGS_0
#undef FN_WITH_ARGS_1
#undef FN_WITH_ARGS_2
#undef FN_WITH_ARGS_3
#undef FN_WITH_ARGS_4
#undef FN_WITH_ARGS_5
#undef FN_WITH_ARGS_6
