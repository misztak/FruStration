#pragma once

static const char MIPS_TARGET_CONFIG[] = R"(<?xml version="1.0"?>
<!DOCTYPE feature SYSTEM "gdb-target.dtd">
<target version="1.0">
<architecture>mips:3000</architecture>
<osabi>none</osabi>
<!-- GDB MIPS32 Register Packet Format -->
<feature name="org.gnu.gdb.mips.cpu">
  <reg name="r0" bitsize="32" regnum="0"/>
  <reg name="r1" bitsize="32"/>
  <reg name="r2" bitsize="32"/>
  <reg name="r3" bitsize="32"/>
  <reg name="r4" bitsize="32"/>
  <reg name="r5" bitsize="32"/>
  <reg name="r6" bitsize="32"/>
  <reg name="r7" bitsize="32"/>
  <reg name="r8" bitsize="32"/>
  <reg name="r9" bitsize="32"/>
  <reg name="r10" bitsize="32"/>
  <reg name="r11" bitsize="32"/>
  <reg name="r12" bitsize="32"/>
  <reg name="r13" bitsize="32"/>
  <reg name="r14" bitsize="32"/>
  <reg name="r15" bitsize="32"/>
  <reg name="r16" bitsize="32"/>
  <reg name="r17" bitsize="32"/>
  <reg name="r18" bitsize="32"/>
  <reg name="r19" bitsize="32"/>
  <reg name="r20" bitsize="32"/>
  <reg name="r21" bitsize="32"/>
  <reg name="r22" bitsize="32"/>
  <reg name="r23" bitsize="32"/>
  <reg name="r24" bitsize="32"/>
  <reg name="r25" bitsize="32"/>
  <reg name="r26" bitsize="32"/>
  <reg name="r27" bitsize="32"/>
  <reg name="r28" bitsize="32"/>
  <reg name="r29" bitsize="32"/>
  <reg name="r30" bitsize="32"/>
  <reg name="r31" bitsize="32"/>
  <reg name="lo" bitsize="32" regnum="33"/>
  <reg name="hi" bitsize="32" regnum="34"/>
  <reg name="pc" bitsize="32" regnum="37"/>
</feature>
<feature name="org.gnu.gdb.mips.cp0">
  <reg name="status" bitsize="32" regnum="32"/>
  <reg name="badvaddr" bitsize="32" regnum="35"/>
  <reg name="cause" bitsize="32" regnum="36"/>
</feature>
<!-- FPU stub (required) -->
<feature name="org.gnu.gdb.mips.fpu">
  <reg name="f0" bitsize="32" type="ieee_single" regnum="38"/>
  <reg name="f1" bitsize="32" type="ieee_single"/>
  <reg name="f2" bitsize="32" type="ieee_single"/>
  <reg name="f3" bitsize="32" type="ieee_single"/>
  <reg name="f4" bitsize="32" type="ieee_single"/>
  <reg name="f5" bitsize="32" type="ieee_single"/>
  <reg name="f6" bitsize="32" type="ieee_single"/>
  <reg name="f7" bitsize="32" type="ieee_single"/>
  <reg name="f8" bitsize="32" type="ieee_single"/>
  <reg name="f9" bitsize="32" type="ieee_single"/>
  <reg name="f10" bitsize="32" type="ieee_single"/>
  <reg name="f11" bitsize="32" type="ieee_single"/>
  <reg name="f12" bitsize="32" type="ieee_single"/>
  <reg name="f13" bitsize="32" type="ieee_single"/>
  <reg name="f14" bitsize="32" type="ieee_single"/>
  <reg name="f15" bitsize="32" type="ieee_single"/>
  <reg name="f16" bitsize="32" type="ieee_single"/>
  <reg name="f17" bitsize="32" type="ieee_single"/>
  <reg name="f18" bitsize="32" type="ieee_single"/>
  <reg name="f19" bitsize="32" type="ieee_single"/>
  <reg name="f20" bitsize="32" type="ieee_single"/>
  <reg name="f21" bitsize="32" type="ieee_single"/>
  <reg name="f22" bitsize="32" type="ieee_single"/>
  <reg name="f23" bitsize="32" type="ieee_single"/>
  <reg name="f24" bitsize="32" type="ieee_single"/>
  <reg name="f25" bitsize="32" type="ieee_single"/>
  <reg name="f26" bitsize="32" type="ieee_single"/>
  <reg name="f27" bitsize="32" type="ieee_single"/>
  <reg name="f28" bitsize="32" type="ieee_single"/>
  <reg name="f29" bitsize="32" type="ieee_single"/>
  <reg name="f30" bitsize="32" type="ieee_single"/>
  <reg name="f31" bitsize="32" type="ieee_single"/>
  <reg name="fcsr" bitsize="32" group="float"/>
  <reg name="fir" bitsize="32" group="float"/>
</feature>
</target>
)";

static const char PSX_MEMORY_MAP[] = R"(<?xml version="1.0"?>
<memory-map>
  <!-- Everything here is described as RAM, because we don't really
       have any better option. -->
  <!-- Main memory bloc: let's go with 8MB straight off the bat. -->
  <memory type="ram" start="0x0000000000000000" length="0x800000"/>
  <memory type="ram" start="0xffffffff80000000" length="0x800000"/>
  <memory type="ram" start="0xffffffffa0000000" length="0x800000"/>
  <!-- EXP1 can go up to 8MB too. -->
  <memory type="ram" start="0x000000001f000000" length="0x800000"/>
  <memory type="ram" start="0xffffffff9f000000" length="0x800000"/>
  <memory type="ram" start="0xffffffffbf000000" length="0x800000"/>
  <!-- Scratchpad -->
  <memory type="ram" start="0x000000001f800000" length="0x400"/>
  <memory type="ram" start="0xffffffff9f800000" length="0x400"/>
  <!-- Hardware registers -->
  <memory type="ram" start="0x000000001f801000" length="0x2000"/>
  <memory type="ram" start="0xffffffff9f801000" length="0x2000"/>
  <memory type="ram" start="0xffffffffbf801000" length="0x2000"/>
  <!-- DTL BIOS SRAM -->
  <memory type="ram" start="0x000000001fa00000" length="0x200000"/>
  <memory type="ram" start="0xffffffff9fa00000" length="0x200000"/>
  <memory type="ram" start="0xffffffffbfa00000" length="0x200000"/>
  <!-- BIOS -->
  <memory type="ram" start="0x000000001fc00000" length="0x80000"/>
  <memory type="ram" start="0xffffffff9fc00000" length="0x80000"/>
  <memory type="ram" start="0xffffffffbfc00000" length="0x80000"/>
  <!-- This really is only for 0xfffe0130 -->
  <memory type="ram" start="0xfffffffffffe0000" length="0x200"/>
</memory-map>
)";
