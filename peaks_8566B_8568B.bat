@echo off
echo.
echo This batch file transmits the downloadable program 04_PEAKS_DLP
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
talk 18 "VARDEF I_NDEX,0;" 0
talk 18 "VARDEF S_FREQ,0;" 0
talk 18 "VARDEF E_FREQ,0;" 0
talk 18 "VARDEF R_EPEAT,0;" 0
talk 18 "VARDEF P_KAMP,0;" 0
talk 18 "VARDEF S_PAN,0;" 0
talk 18 "VARDEF P_KFRQ,0;" 0
talk 18 "VARDEF P_KNUM,0;" 0
talk 18 "VARDEF D_ADDR,3073;" 0
talk 18 "TRDEF P_KARY,6;" 0
talk 18 "TRDEF T_RACEA,1001;" 0
talk 18 "FUNCDEF P_MNTR,@P_XPLAIN;LF;KS=;P_SETUP;TRDSP TRC ON;" 0
talk 18 "REPEAT" 0
talk 18 "  P_KDAT;" 0
talk 18 "  P_KCHK;" 0
talk 18 "UNTIL R_EPEAT,EQ,1;" 0
talk 18 "@;FUNCDEF P_XPLAIN,@P_AGE;F_RAME;" 0
talk 18 "PU;PA 224,640;TEXT /MONITOR SIGNALS" 0
talk 18 "" 0
talk 18 " This DLP asks you to select the start and" 0
talk 18 " stop frequencies for the HP 8566B to" 0
talk 18 " monitor.  The analyzer will then" 0
talk 18 " continuously monitor the selected frequency" 0
talk 18 " range.  The internal 'PEAKS' function is" 0
talk 18 " used to sort the six highest signals by" 0
talk 18 " amplitude.  The results and a reformatted" 0
talk 18 " trace are displayed on the analyzer's CRT." 0
talk 18 "" 0
talk 18 "" 0
talk 18 "  NOTE: This DLP has been configured to" 0
talk 18 "        ignore signals that are < -65 dBm" 0
talk 18 "        or have < 6 dB of signal excursion./;" 0
talk 18 "C_PRMPT;" 0
talk 18 "@;FUNCDEF P_SETUP,@EM;D3;" 0
talk 18 "PU;PA 144,608;TEXT /ENTER DESIRED START FREQUENCY/;" 0
talk 18 "PA 144,544;TEXT /& PRESS APPROPRIATE UNITS KEY/;" 0
talk 18 "PA 208,480;TEXT /( SUGGEST: 50 <MHz> )/;" 0
talk 18 "S_FREQ EP;EM;" 0
talk 18 "MOV FA,S_FREQ;MOV E_FREQ,S_FREQ;" 0
talk 18 "REPEAT" 0
talk 18 "  IF E_FREQ,LT,S_FREQ THEN" 0
talk 18 "    PU;PA 112,672;" 0
talk 18 "    TEXT /STOP FREQ. MUST BE >= START FREQ./;" 0
talk 18 "  ENDIF;" 0
talk 18 "  PU;PA 144,608;TEXT /ENTER DESIRED STOP  FREQUENCY/;" 0
talk 18 "  PA 144,544;TEXT /& PRESS APPROPRIATE UNITS KEY/;" 0
talk 18 "  PA 208,480;TEXT /( SUGGEST: 700 <MHz> )/;" 0
talk 18 "  E_FREQ EP;EM;" 0
talk 18 "UNTIL E_FREQ,GE,S_FREQ;" 0
talk 18 "MOV FB,E_FREQ;" 0
talk 18 "ANNOT OFF;GRAT OFF;DLE OFF;AUNITS DBM;" 0
talk 18 "TRDSP TRA,OFF;TRDSP TRB,OFF;" 0
talk 18 "DET POS;SNGLS;" 0
talk 18 "MKPX 6DB;TH -65DM;VBO -3;AT 20DB;HD;TS;" 0
talk 18 "DIV S_FREQ,S_FREQ,1E6;DIV E_FREQ,E_FREQ,1E6;" 0
talk 18 "D1;MOV DA,D_ADDR;" 0
talk 18 "PU;PA 0,548;PD;PR 1000,0,0,410,-1000,0,0,-410;" 0
talk 18 "PU;PA 0,70;PD;PR 1000,0,0,350,-1000,0,0,-350;D3;" 0
talk 18 "PU;PA 160,640;TEXT /M O N I T O R  S I G N A L S/;" 0
talk 18 "D2;PU;PA 496,512;TEXT /BAND SWEEP/;" 0
talk 18 "PA 112,512;TEXT /START = /;DSPLY S_FREQ,7.2;TEXT MHz/;" 0
talk 18 "PA 720,512;TEXT /STOP = /;DSPLY E_FREQ,7.2;TEXT /  MHz/;" 0
talk 18 "PA 320,352;TEXT /FREQ(MHz)/;" 0
talk 18 "PA 672,352;TEXT /AMPL(dBm)/;" 0
talk 18 "PA 480,96;TEXT /SIGNAL DATA/;" 0
talk 18 "M_PRMPT;" 0
talk 18 "MOV D_ADDR,DA;" 0
talk 18 "@;FUNCDEF P_CALC,@MOV DA,D_ADDR;" 0
talk 18 "D3;PU;PA 240,544;TEXT /                    /;" 0
talk 18 "D2;PU;PA 320,320;" 0
talk 18 "DIV S_PAN,SP,1001;" 0
talk 18 "MOV I_NDEX,0;" 0
talk 18 "REPEAT" 0
talk 18 "  ADD I_NDEX,I_NDEX,1;" 0
talk 18 "  CTA P_KAMP,TRA[P_KARY[I_NDEX]];" 0
talk 18 "  MPY P_KFRQ,S_PAN,P_KARY[I_NDEX];" 0
talk 18 "  ADD P_KFRQ,P_KFRQ,FA;" 0
talk 18 "  DIV P_KFRQ,P_KFRQ,1E6;" 0
talk 18 "  DSPLY P_KFRQ,7.1;" 0
talk 18 "  PU;PR 288,0;" 0
talk 18 "  DSPLY P_KAMP,5.1;" 0
talk 18 "  PU;PR -468,-32;" 0
talk 18 "UNTIL I_NDEX,EQ,P_KNUM;" 0
talk 18 "DW 1028;" 0
talk 18 "@;FUNCDEF P_KDAT,@TS;MOV P_KNUM,PEAKS P_KARY,TRA,AMP;" 0
talk 18 "MIN P_KNUM,P_KNUM,6;" 0
talk 18 "MOV T_RACEA,TRA;MPY T_RACEA,T_RACEA,0.4;" 0
talk 18 "ADD T_RACEA,T_RACEA,550;MOV TRB,T_RACEA;VIEW TRB;" 0
talk 18 "@;FUNCDEF P_KCHK,@IF P_KNUM,EQ,0 THEN" 0
talk 18 "  MOV DA,D_ADDR;" 0
talk 18 "  D3;PU;PA 256,544;" 0
talk 18 "  TEXT /NO SIGNALS FOUND/;" 0
talk 18 "  D2;DW 1028;" 0
talk 18 "ELSE" 0
talk 18 "  P_CALC;" 0
talk 18 "ENDIF;@;FUNCDEF P_AGE,@IP;ANNOT OFF;GRAT OFF;BLANK TRA;D3;@;FUNCDEF P_AUSE,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "         Press the <Hz> key for more      /;" 0
talk 18 "EP;@;FUNCDEF H_OLD,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "       Press the <Hz> key to continue     /;" 0
talk 18 "EP;@;FUNCDEF C_PRMPT,@;PU;PA 64,96;TEXT /Connect signal source to RF input/;" 0
talk 18 "H_OLD;@;FUNCDEF F_RAME,@EM;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "@;FUNCDEF M_PRMPT,@;D2;PU;PA 256,32;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,0;TEXT /To Return To The Menu/;" 0
talk 18 "@;KEYDEF 102,P_MNTR;" 0
echo Upload complete.  You can run this program by pressing Shift 102 Hz 
echo on the analyzer's front panel.
echo.
echo User RAM bytes left:
query 18 "MEM?"

