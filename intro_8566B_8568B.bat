@echo off
echo.
echo This batch file transmits the downloadable program 01_INTRO_DLP
echo to the nonvolatile user RAM on an HP 8566B or 8568B spectrum analyzer.  
echo.
echo See HP product note 8566B/8568B-1 for more information about DLPs
echo.
echo Ensure that spectrum analyzer is available at GPIB address 18!
pause
echo.
echo User RAM bytes left:
query 18 "MEM?"
choice /C YN /M "Dispose of all existing user program(s) first?"
if errorlevel 2 goto nodispose
echo Erasing...
talk 18 "DISPOSE ALL;" 0
:nodispose
echo.
echo Uploading...
echo.
talk 18 "FUNCDEF F_IRST,@P_AGE;F_RAME;" 0
talk 18 "PU;PA 208,672;TEXT /#T H A N K   Y O U ! !#" 0
talk 18 "" 0
talk 18 " For selecting the HP 8566B High-Performance" 0
talk 18 " Spectrum Analyzer. In addition to capabili-" 0
talk 18 " ties traditionally found in a programmable" 0
talk 18 " analyzer,  your HP 8566B can execute" 0
talk 18 " DOWN-LOADABLE PROGRAMS (DLPs)." 0
talk 18 "" 0
talk 18 "" 0
talk 18 " A DLP is a software routine that is created" 0
talk 18 " with an external computer and then stored" 0
talk 18 " (Down-Loaded) into the non-volatile memory" 0
talk 18 " of the HP 8566B.  Once down-loaded the DLP" 0
talk 18 " can be executed without the further need of" 0
talk 18 " an external computer.  The text you are" 0
talk 18 " reading and the demonstrations you will" 0
talk 18 " see have been generated using DLPs./;" 0
talk 18 "P_AUSE;@;FUNCDEF S_ECND,@F_RAME;" 0
talk 18 "PU;PA 16,640;TEXT $DLPs are powerful routines that have the" 0
talk 18 " following capabilities:" 0
talk 18 "" 0
talk 18 " * PROGRAM CONTROL:  REPEAT/UNTIL,  IF/THEN" 0
talk 18 "" 0
talk 18 " * CONDITIONAL BRANCHING: =, <, >, etc..." 0
talk 18 "" 0
talk 18 " * MATH FUNCTIONS: ADD, DIV, LOG, etc..." 0
talk 18 "" 0
talk 18 " * IEEE-488 CONTROL: OUTPUT/ENTER" 0
talk 18 "" 0
talk 18 " DLPs can: Program the analyzer to make" 0
talk 18 "           complex/repetitious measurements." 0
talk 18 "           Process signal and trace data." 0
talk 18 "           Make decisions based on the data." 0
talk 18 "" 0
talk 18 " The following screens will demonstrate the" 0
talk 18 " capabilities of DLPs.$;" 0
talk 18 "P_AUSE;@;FUNCDEF P_AGE,@IP;ANNOT OFF;GRAT OFF;BLANK TRA;D3;@;FUNCDEF P_AUSE,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "         Press the <Hz> key for more      /;" 0
talk 18 "EP;@;FUNCDEF H_OLD,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "       Press the <Hz> key to continue     /;" 0
talk 18 "EP;@;FUNCDEF C_PRMPT,@;PU;PA 64,96;TEXT /#Connect# Calibrator signal to RF input/;" 0
talk 18 "H_OLD;@;FUNCDEF F_RAME,@EM;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "@;FUNCDEF M_PRMPT,@;D2;PU;PA 256,32;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,0;TEXT /To Return To The Menu/;" 0
talk 18 "@;FUNCDEF S_XPLAIN,@P_AGE;F_RAME;" 0
talk 18 "PU;PA 16,640;TEXT /To begin,  we will show you a small DLP" 0
talk 18 " to help you get acquainted with the syntax" 0
talk 18 " used in programming the HP 8566B." 0
talk 18 "" 0
talk 18 " The DLP will send a set of commands to the" 0
talk 18 " analyzer to make it tune to the calibrator" 0
talk 18 " signal, zoom to a 1 MHz Span, put a marker" 0
talk 18 " at the peak of the signal,  and show the" 0
talk 18 " frequency & amplitude of that signal." 0
talk 18 "" 0
talk 18 " At each step,  the analyzer will display a" 0
talk 18 " brief description and the DLP commands to" 0
talk 18 " be executed.  The DLP will then wait until" 0
talk 18 " the <Hz> key is pressed before executing" 0
talk 18 " the next set of DLP commands./;" 0
talk 18 "C_PRMPT;" 0
talk 18 "@;FUNCDEF S_NGL,@EM;PU;PA 80,576;TEXT /Brief description:      Preset/;" 0
talk 18 "PA 464,544;TEXT /Analyzer/;B_OX;" 0
talk 18 "PA 0,384;TEXT /Commands to be executed ==>  LF;SNGLS;/;" 0
talk 18 "PA 0,288;TEXT /Prompt for next step =====>/;C_ONT;W_AIT;" 0
talk 18 "LF;SNGLS;TS;" 0
talk 18 "D3;EM;PU;PA 464,576;TEXT /Set Start Freq./;" 0
talk 18 "PA 464,544;TEXT /to 50 MHz/;B_OX;" 0
talk 18 "PA 464,384;TEXT /FA 50MZ;/;C_ONT;W_AIT;" 0
talk 18 "FA 50MZ;TS;" 0
talk 18 "PU;PA 464,576;TEXT /Set Stop Freq./;" 0
talk 18 "PA 464,544;TEXT /to 200 MHz/;B_OX;" 0
talk 18 "PA 464,384;TEXT /FB 200MZ;/;C_ONT;W_AIT;" 0
talk 18 "FB 200MZ;TS;" 0
talk 18 "PU;PA 464,576;TEXT /Bring signal/;" 0
talk 18 "PA 464,544;TEXT /to center and/;" 0
talk 18 "PA 464,512;TEXT /reduce span/;" 0
talk 18 "PA 464,480;TEXT /to 1 MHz/;B_OX;" 0
talk 18 "PA 464,416;TEXT /MKPK HI;TS;/;" 0
talk 18 "PA 464,384;TEXT /MKTRACK ON;/;" 0
talk 18 "PA 464,352;TEXT /SP 1MZ/;C_ONT;W_AIT;" 0
talk 18 "MKPK HI;MKTRACK ON;TS;SP 1MZ;TS;" 0
talk 18 "PU;PA 464,576;TEXT /Bring marker/;" 0
talk 18 "PA 464,544;TEXT /to peak and/;" 0
talk 18 "PA 464,512;TEXT /turn on/;" 0
talk 18 "PA 464,480;TEXT /marker readout/;B_OX;" 0
talk 18 "PA 464,416;TEXT /MKTRACK OFF;/;" 0
talk 18 "PA 464,384;TEXT /TS;MKPK HI;/;" 0
talk 18 "PA 464,352;TEXT /MKN;TS;/;C_ONT;EP;" 0
talk 18 "MKTRACK OFF;TS;MKPK HI;MKN;TS;EM;" 0
talk 18 "PU;PA 464,576;TEXT /Now press/;" 0
talk 18 "PA 464,544;TEXT /<Hz> to see/;" 0
talk 18 "PA 464,512;TEXT /the same DLP/;" 0
talk 18 "PA 464,480;TEXT /run at normal/;" 0
talk 18 "PA 464,448;TEXT /speed/;" 0
talk 18 "MKN;EP;EM;@;FUNCDEF B_OX,@PU;PA 448,448;PD;" 0
talk 18 "PR 210,0,0,-100,-210,0,0,100;" 0
talk 18 "PR 210,0,0,-100,-210,0,0,100;" 0
talk 18 "PR 210,0,0,-100,-210,0,0,100;" 0
talk 18 "PR 210,0,0,-100,-210,0,0,100;" 0
talk 18 "PU;@;FUNCDEF C_ONT,@D2;PU;PA 688,448;TEXT /#<Hz> to continue#/;" 0
talk 18 "D3;HD;@;FUNCDEF W_AIT,@HD;EP;EM;@;FUNCDEF S_NORM,@LF;SNGLS;TS;FA 50MZ;FB 200MZ;" 0
talk 18 "TS;MKPK HI;MKTRACK ON;SP 1MZ;MKTRACK OFF;TS;MKPK HI;MKN;CONTS;" 0
talk 18 "EM;D3;PU;PA 448,576;TEXT /Press <Hz>/;" 0
talk 18 "PA 448,544;TEXT /to continue/;" 0
talk 18 "PA 512,480;TEXT /or/;" 0
talk 18 "PA 448,416;TEXT /<INSTR PRESET>/;" 0
talk 18 "PA 448,384;TEXT /<SHIFT>  30/;" 0
talk 18 "PA 448,352;TEXT /<Hz> to replay/;" 0
talk 18 "MKN;EP;@;FUNCDEF S_UMM,@P_AGE;F_RAME;" 0
talk 18 "PU;PA 16,576;TEXT /The remaining DLPs show how the HP 8566B" 0
talk 18 " can make complex measurements,  process" 0
talk 18 " data,  and display data without needing" 0
talk 18 " an external computer." 0
talk 18 "" 0
talk 18 "" 0
talk 18 "" 0
talk 18 " Hewlett-Packard provides a series of DLP" 0
talk 18 " measurement products which allow you to" 0
talk 18 " use DLPs in your applications.  A DLP" 0
talk 18 " utility is also available that can help" 0
talk 18 " you create your own DLPs./;" 0
talk 18 "P_AUSE;" 0
talk 18 "F_RAME;" 0
talk 18 "PU;PA 16,608; TEXT /To get more information on DLPs and how" 0
talk 18 " to write your own,  please send in the" 0
talk 18 " survey card included with your HP 8566B." 0
talk 18 "" 0
talk 18 "" 0
talk 18 " If the survey card is missing,  you can" 0
talk 18 " get more information by contacting your" 0
talk 18 " local HP sales office or writing to:" 0
talk 18 "" 0
talk 18 "          HEWLETT-PACKARD COMPANY" 0
talk 18 "          SIGNAL ANALYSIS DIVISION" 0
talk 18 "          ATTN: PRODUCT MARKETING" 0
talk 18 "          1212 Valley House Drive" 0
talk 18 "          Rohnert Park, CA 94928-4999" 0
talk 18 "          U.S.A./;" 0
talk 18 "H_OLD;@;FUNCDEF P_AGE,@IP;ANNOT OFF;GRAT OFF;BLANK TRA;D3;@;FUNCDEF P_AUSE,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "         Press the <Hz> key for more      /;" 0
talk 18 "EP;@;FUNCDEF H_OLD,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "       Press the <Hz> key to continue     /;" 0
talk 18 "EP;@;FUNCDEF C_PRMPT,@;PU;PA 64,96;TEXT /#Connect# Calibrator signal to RF input/;" 0
talk 18 "H_OLD;@;FUNCDEF F_RAME,@EM;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "@;FUNCDEF M_PRMPT,@;D2;PU;PA 256,32;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,0;TEXT /To Return To The Menu/;" 0
talk 18 "@;FUNCDEF S_IMPLE,@S_XPLAIN;S_NGL;S_NORM;S_UMM;@;KEYDEF  30,/S_IMPLE;M_ENU;/" 0
talk 18 "FUNCDEF I_NTRO,@F_IRST;S_ECND;S_IMPLE;@;KEYDEF  10,/I_NTRO;M_ENU;/" 0
echo Upload complete.  You can run this program by pressing Shift 10 Hz 
echo on the analyzer's front panel.
echo.
echo User RAM bytes left:
query 18 "MEM?"
echo.
