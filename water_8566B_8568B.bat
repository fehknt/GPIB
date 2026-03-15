@echo off
echo.
echo This batch file transmits the downloadable program 05_WATER_DLP
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
talk 18 "VARDEF W_TEMP,0;" 0
talk 18 "VARDEF W_TEMP,0;" 0
talk 18 "VARDEF W_LOOP,0;" 0
talk 18 "VARDEF W_SFRQ,0;" 0
talk 18 "VARDEF W_EFRQ,0;" 0
talk 18 "TRDEF W_TRONE,50;" 0
talk 18 "TRDEF W_TRTWO,50;" 0
talk 18 "TRDEF W_TRTHR,50;" 0
talk 18 "TRDEF W_TRFOR,50;" 0
talk 18 "TRDEF W_TRFIV,50;" 0
talk 18 "TRDEF W_TRSIX,50;" 0
talk 18 "TRDEF W_TRSEV,50;" 0
talk 18 "TRDEF W_TREIG,50;" 0
talk 18 "TRDEF W_TRNIN,50;" 0
talk 18 "FUNCDEF W_DISP,@W_XPLAIN;" 0
talk 18 "LF;" 0
talk 18 "W_SETUP;" 0
talk 18 "REPEAT" 0
talk 18 "  W_UPDTE;" 0
talk 18 "  ADD W_LOOP,W_LOOP,1;" 0
talk 18 "UNTIL W_LOOP,GT,9;" 0
talk 18 "ONEOS /W_UPDTE;W_GRPH;/;" 0
talk 18 "@;FUNCDEF W_XPLAIN,@P_AGE;F_RAME;" 0
talk 18 "PU;PA 224,640;TEXT /WATERFALL DISPLAY" 0
talk 18 "" 0
talk 18 " This DLP demonstrates the falling raster" 0
talk 18 " (Waterfall) display used in surveillance" 0
talk 18 " and EW environments.  This type of display" 0
talk 18 " allows the USER to view the RF input signal" 0
talk 18 " in a signal-frequency-time relationship." 0
talk 18 "" 0
talk 18 " You are asked to select the initial start" 0
talk 18 " and stop frequencies,  while the DLP" 0
talk 18 " is running you can use the the FUNCTION," 0
talk 18 " COUPLED FUNCTION & SWEEP front panel keys" 0
talk 18 " to change the analyzer's settings." 0
talk 18 "" 0
talk 18 " NOTE:  Other signal sources may be used" 0
talk 18 "        in place of the calibrator./;" 0
talk 18 "C_PRMPT;" 0
talk 18 "@;FUNCDEF W_SETUP,@EM;D3;" 0
talk 18 "PU;PA 144,608;TEXT /ENTER DESIRED START FREQUENCY/;" 0
talk 18 "PA 144,544;TEXT /& PRESS APPROPRIATE UNITS KEY/;" 0
talk 18 "PA 208,480;TEXT /( SUGGEST: 50 <MHz> )/;" 0
talk 18 "W_SFRQ EP;EM;" 0
talk 18 "MOV FA,W_SFRQ;MOV W_EFRQ,W_SFRQ;" 0
talk 18 "REPEAT" 0
talk 18 "  IF W_EFRQ,LT,W_SFRQ THEN" 0
talk 18 "    PU;PA 112,672;" 0
talk 18 "    TEXT /STOP FREQ. #MUST BE >=# START FREQ./;" 0
talk 18 "  ENDIF;" 0
talk 18 "  PU;PA 144,608;TEXT /ENTER DESIRED STOP  FREQUENCY/;" 0
talk 18 "  PA 144,544;TEXT /& PRESS APPROPRIATE UNITS KEY/;" 0
talk 18 "  PA 208,480;TEXT /( SUGGEST: 550 <MHz> )/;" 0
talk 18 "  W_EFRQ EP;EM;" 0
talk 18 "UNTIL W_EFRQ,GE,W_SFRQ;" 0
talk 18 "MOV FB,W_EFRQ;" 0
talk 18 "TS;KS=;TRDSP TRA,OFF;" 0
talk 18 "MOV TRB,0;BLANK TRB;" 0
talk 18 "TRDSP TRC,ON;" 0
talk 18 "GRAT OFF;" 0
talk 18 "DA1024;DW1025;D3;" 0
talk 18 "PU;PA 224,576;TEXT /WATERFALL DISPLAY/;" 0
talk 18 "D2;PU;PA 256,128;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,96;TEXT /To Return To The Menu/;" 0
talk 18 "PS;HD;DA2864;DW146;HD;D1;" 0
talk 18 "@;FUNCDEF W_UPDTE,@MOV W_TRNIN,W_TREIG;MOV W_TREIG,W_TRSEV;" 0
talk 18 "MOV W_TRSEV,W_TRSIX;MOV W_TRSIX,W_TRFIV;" 0
talk 18 "MOV W_TRFIV,W_TRFOR;MOV W_TRFOR,W_TRTHR;" 0
talk 18 "MOV W_TRTHR,W_TRTWO;MOV W_TRTWO,W_TRONE;" 0
talk 18 "COMPRESS W_TRONE,TRA,POS;MPY W_TRONE,W_TRONE,.5;" 0
talk 18 "@;FUNCDEF W_GRPH,@TRGRPH 3073,140,100,15,W_TRONE;" 0
talk 18 "TRGRPH 3174,150,130,15,W_TRTWO;" 0
talk 18 "TRGRPH 3275,160,160,15,W_TRTHR;" 0
talk 18 "TRGRPH 3376,170,190,15,W_TRFOR;" 0
talk 18 "TRGRPH 3477,180,210,15,W_TRFIV;" 0
talk 18 "TRGRPH 3578,190,240,15,W_TRSIX;" 0
talk 18 "TRGRPH 3679,200,270,15,W_TRSEV;" 0
talk 18 "TRGRPH 3780,210,300,15,W_TREIG;" 0
talk 18 "TRGRPH 3881,220,330,15,W_TRNIN;" 0
talk 18 "@;FUNCDEF P_AGE,@IP;ANNOT OFF;GRAT OFF;BLANK TRA;D3;@;FUNCDEF P_AUSE,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "         Press the <Hz> key for more      /;" 0
talk 18 "EP;@;FUNCDEF H_OLD,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "       Press the <Hz> key to continue     /;" 0
talk 18 "EP;@;FUNCDEF C_PRMPT,@;PU;PA 64,96;TEXT /#Connect# Calibrator signal to RF input/;" 0
talk 18 "H_OLD;@;FUNCDEF F_RAME,@EM;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "@;FUNCDEF M_PRMPT,@;D2;PU;PA 256,32;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,0;TEXT /To Return To The Menu/;" 0
talk 18 "@;KEYDEF 103,W_DISP;" 0
echo Upload complete.  You can run this program by pressing Shift 103 Hz 
echo on the analyzer's front panel.
echo.
echo User RAM bytes left:
query 18 "MEM?"
