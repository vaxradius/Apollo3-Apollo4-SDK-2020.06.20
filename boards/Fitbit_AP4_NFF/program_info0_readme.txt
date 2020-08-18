python create_info0.py --valid 1 info0 --pl 1 --main 0xC000 --gpio 0x3F --version 0 --wmask 0x2 --wSlInt 0x3A --chipType apollo4a

C:\AmbiqMicro\AP4\Apollo3-Apollo4-SDK-2020.06.20\tools\apollo4_scripts>python create_info0.py --valid 1 info0 --pl 1 --main 0xC000 --gpio 0x3F --version 0 --wmask 0x2 --wSlInt 0x3A --chipType apollo4a
Apollo4a INFO0
info0 Signature...
['0x88', '0xad', '0xea', '0x48', '0x37', '0x57', '0x70', '0xc9', '0x58', '0x84', '0x6b', '0xa', '0x74', '0x9d', '0x1a', '0xe4']
Security Word =  0x55fff
Customer Trim =  0x0
Override =  0x3f
WiredCfg =  0x4e2041d2
Version =  0x0
Main Ptr =  0xc000
SRAM Reservation =  0x0
Permanent Write Protections =  0xffffffff : 0xffffffff : 0xffffffff : 0xffffffff
Permanent Copy Protections =  0xffffffff : 0xffffffff : 0xffffffff : 0xffffffff
SBL Overridable Write Protections =  0xffffffff : 0xffffffff : 0xffffffff : 0xffffffff
SBL Overridable Copy Protections =  0xffffffff : 0xffffffff : 0xffffffff : 0xffffffff
Writing to file  info0.bin


C:\AmbiqMicro\AP4\Apollo3-Apollo4-SDK-2020.06.20\tools\apollo4_scripts>prog_info0.bat
SEGGER J-Link Commander V6.80a (Compiled May 29 2020 16:28:01)
DLL version V6.80a, compiled May 29 2020 16:27:00


J-Link Command File read successfully.
Processing script file...

J-Link connection not established yet but required for command.
Connecting to J-Link via USB...O.K.
Firmware: J-Link V9 compiled Dec 13 2019 11:14:50
Hardware version: V9.20
S/N: -1
License(s): RDI, GDB, FlashDL, FlashBP, JFlash, RDDI
VTref=1.780V

Selecting SWD as current target interface.

Selecting 1000 kHz as target interface speed

Target connection not established yet but required for command.
Device "AMAP42KK-KBR-SBLC000" selected.


Connecting to target via SWD
Found SW-DP with ID 0x6BA02477
Unknown DP version. Assuming DPv0
Scanning AP map to find all available APs
AP[2]: Stopped AP scan as end of AP map has been reached
AP[0]: AHB-AP (IDR: 0x24770011)
AP[1]: APB-AP (IDR: 0x54770002)
Iterating through AP map to find AHB-AP to use
AP[0]: Core found
AP[0]: AHB-AP ROM base: 0xE00FF000
CPUID register: 0x410FC241. Implementer code: 0x41 (ARM)
Found Cortex-M4 r0p1, Little endian.
FPUnit: 6 code (BP) slots and 2 literal slots
CoreSight components:
ROMTbl[0] @ E00FF000
ROMTbl[0][0]: E000E000, CID: B105E00D, PID: 000BB00C SCS-M7
ROMTbl[0][1]: E0001000, CID: B105E00D, PID: 003BB002 DWT
ROMTbl[0][2]: E0002000, CID: B105E00D, PID: 002BB003 FPB
ROMTbl[0][3]: E0000000, CID: B105E00D, PID: 003BB001 ITM
ROMTbl[0][5]: E0041000, CID: B105900D, PID: 000BB925 ETM
ROMTbl[0][6]: E0042000, CID: B105900D, PID: 005BB906 CTI
Cortex-M4 identified.
Reset delay: 0 ms
Reset type NORMAL: Resets core & peripherals via SYSRESETREQ & VECTRESET bit.
ResetTarget() start
 AMAP42KK-KBR.JLinkScript ResetTarget()
 JDEC PID 0x000000BF
 Ambiq Apollo4 identified!
 Bootldr = 0x04000000
 Secure device.
 Secure device. Bootloader needs to run which will then halt when finish.
 CPU halted after reset. Num Tries = 0x00000001
ResetTarget() end

Sleep(10)

Writing 00000000 -> 10000000

Writing 00000200 -> 10000004

Writing D894E09E -> 10000008

Writing FFFFFFFF -> 1000000C

Downloading file [info0.bin]...
O.K.



Sleep(50)

1000000C = 00000000

Writing 0000001B -> 40000004

Sleep(2000)


Script processing completed.

Press any key to continue . . .



////////////////////////////////////////////////////////////////////////////////////////////
// Bootloader SWO pin is GPIO28(TP1102)
////////////////////////////////////////////////////////////////////////////////////////////
Ambiq Secure BootLoader!



SecureBoot SBL_apollo4a_v1 ver:0x1(0xa410) running with VTOR @ 0x200
Current Reset Stat 0x1

Info1 Version 0x1
Info0 Version 0x0
ChipID = 0x35f3b003:0x4b6cb814
Flash Size = 0x200000, DTCM Size = 0x60000
Scratch = 0x0
INFO1-Sec = 0x70523ff
SBL version 0x1 installed at 0x0
Previous Boot was Successful:13
INFO0-Sec = 0x55fff
Patch Tracker = 0xffffffff 0xffffffff 0xffffffff 0xffffffff
Info0 Valid
OTA State: activeIdx=2 otaDesc = 0x7b179087
Override GPIO 0x3f
Override GPIO Value 0x0
Initialization done
Force GPIO Override
Attempting Wired update
Initializing IOS
Waiting for host on IOS
Done with IOS host
Out of Wired update
Proceeding to lock all security gates

