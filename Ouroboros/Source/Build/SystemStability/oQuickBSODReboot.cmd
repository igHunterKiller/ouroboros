@echo off
rem =============================== OOOii ===============================
rem This is a script to run on a Windows system to minimize its personal 
rem platform features and make it less like an operating system and more 
rem like an embedded system.
rem =====================================================================

rem This file contains the timeout value found in 
rem Computer|Properties|Advanced system settings|Advanced|Startup and Recovery Settings... 
rem "Time to display recovery options when needed:"
rem I can't find a way of editing the file through script, in which case 
rem I would set the timeout to 0 so in an error recovery the computer 
rem reboots as quickly as possible. As a workaround, if the file is 
rem compressed it cannot be loaded, and the recovery screen only appears
rem instantly before continuing on, so it's like a 0 timeout
rem NOTE: Must have admin rights for this to work
compact /c /q C:\Windows\bootstat.dat >> nul

rem Set auto-reboot on BSOD and don't write a mem dump so reboot is faster
rem Disable any messages of a failure that might knock our fullscreen DirectX
rem app out of focus and thus minimize it.
regedit /s oQuickBSODReboot.reg
