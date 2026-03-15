@echo off
echo.
echo This batch file transmits the downloadable program 02_MAIN_MENU_DLP
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
talk 18 "FUNCDEF M_ENU,@P_AGE;" 0
talk 18 "PU;PA 256,640;TEXT /DLP Examples" 0
talk 18 "                  for the" 0
talk 18 "         HP 8566B Spectrum Analyzer" 0
talk 18 "" 0
talk 18 "" 0
talk 18 "" 0
talk 18 "      Softkey #     DLP function" 0
talk 18 "" 0
talk 18 "         10    Introduction" 0
talk 18 "        100    Return to this MENU" 0
talk 18 "" 0
talk 18 "        101    Harmonic Distortion" 0
talk 18 "        102    Monitor Signals" 0
talk 18 "        103    Waterfall Display" 0
talk 18 "" 0
talk 18 "" 0
talk 18 "" 0
talk 18 "   Execute desired DLP by pressing:" 0
talk 18 "" 0
talk 18 "    <INSTR PRESET> <SHIFT> Softkey# <Hz>/;" 0
talk 18 "D2;PA 672,224;TEXT /Revision  A.00.00/;D3;" 0
talk 18 "PU;PA 50,500;PD;" 0
talk 18 "PR 592,0,0,-320,-592,0,0,320;" 0
talk 18 "PR 592,0,0,-320,-592,0,0,320;" 0
talk 18 "PR 592,0,0,-320,-592,0,0,320;" 0
talk 18 "PR 592,0,0,-320,-592,0,0,320;" 0
talk 18 "PR 592,0,0,-320,-592,0,0,320;" 0
talk 18 "HD;@;FUNCDEF P_AGE,@IP;ANNOT OFF;GRAT OFF;BLANK TRA;D3;@;FUNCDEF P_AUSE,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "         Press the <Hz> key for more      /;" 0
talk 18 "EP;@;FUNCDEF H_OLD,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "       Press the <Hz> key to continue     /;" 0
talk 18 "EP;@;FUNCDEF C_PRMPT,@;PU;PA 64,96;TEXT /#Connect# Calibrator signal to RF input/;" 0
talk 18 "H_OLD;@;FUNCDEF F_RAME,@EM;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "@;FUNCDEF M_PRMPT,@;D2;PU;PA 256,32;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,0;TEXT /To Return To The Menu/;" 0
talk 18 "@;KEYDEF 100,M_ENU;" 0
echo Upload complete.  You can run this program by pressing Shift 100 Hz 
echo on the analyzer's front panel.  (Note that the desired subprograms must also
echo be downloaded to the analyzer's nonvolatile memory in order for them
echo to be launched from the main menu.)
echo.
echo User RAM bytes left:
query 18 "MEM?"

