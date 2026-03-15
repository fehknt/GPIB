@echo off
echo.
echo This batch file transmits the downloadable program H_DIST
echo for total harmonic distortion (THD) analysis, dated
echo (8.17.87) to the nonvolatile user RAM on an HP 8566B or 8568B 
echo spectrum analyzer.  
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
talk 18 "VARDEF O_PTRNG,0;" 0
talk 18 "VARDEF F_FRQ,0;" 0
talk 18 "VARDEF F_RSTD,0;" 0
talk 18 "VARDEF S_ECDD,0;" 0
talk 18 "VARDEF T_HRDD,0;" 0
talk 18 "VARDEF F_RSTV,0;" 0
talk 18 "VARDEF S_ECDV,0;" 0
talk 18 "VARDEF T_HRDV,0;" 0
talk 18 "VARDEF S_ECSQ,0;" 0
talk 18 "VARDEF T_HRSQ,0;" 0
talk 18 "VARDEF D_ISTRN,0;" 0
talk 18 "VARDEF H_SPAD,0;" 0
talk 18 "TRDEF H_FTRC,200;" 0
talk 18 "FUNCDEF H_INTRO,@P_AGE;F_RAME;" 0
talk 18 "PU;PA 160,640;TEXT /TOTAL HARMONIC DISTORTION" 0
talk 18 " This DLP asks you to enter a fundamental" 0
talk 18 " frequency to be measured." 0
talk 18 "  " 0
talk 18 " The DLP calculates the total harmonic" 0
talk 18 " distortion using the measured amplitudes" 0
talk 18 " of the fundamental and the first two" 0
talk 18 " harmonics.  The results as well as a" 0
talk 18 " compressed display of the fundamental" 0
talk 18 " are displayed on the analyzer's CRT." 0
talk 18 "  " 0
talk 18 " NOTE:  Other signal sources may be used" 0
talk 18 "        but should NOT exceed 0 dBm./;" 0
talk 18 "C_PRMPT;" 0
talk 18 "@;FUNCDEF H_SETUP,@ANNOT ON;" 0
talk 18 "EM;PU;PA 144,608;TEXT /ENTER FUNDAMENTAL FREQUENCY/;" 0
talk 18 "PA 144,544;TEXT /& PRESS APPROPRIATE UNITS KEY/;" 0
talk 18 "F_FRQ EP;EM;MOV CF,F_FRQ;" 0
talk 18 "MPY SP,F_FRQ,.05;" 0
talk 18 "CLRW TRA;GRAT ON;" 0
talk 18 "VBO 0;DET POS;" 0
talk 18 "TS;MKPK HI;MKTRACK ON;" 0
talk 18 "SP 100KZ;HD;MKTRACK OFF;" 0
talk 18 "SNGLS;TS;MKPK HI;" 0
talk 18 "PP;TS;ADD O_PTRNG,MKA,20;" 0
talk 18 "MXM AT,10,O_PTRNG;TS;" 0
talk 18 "TS;MKPK HI;MKRL;TS;MKPK HI;MKCF;TS;" 0
talk 18 "MOV F_FRQ,MF;MKSS;" 0
talk 18 "@;FUNCDEF H_MEAS,@MOV F_RSTD,MKA;AUNITS V;MOV F_RSTV,MKA;AUNITS DBM;" 0
talk 18 "IF F_FRQ,LE,11000E6 THEN" 0
talk 18 "CF UP;TS;" 0
talk 18 "MKPK HI;" 0
talk 18 "PP;TS;" 0
talk 18 "MOV S_ECDD,MKA;" 0
talk 18 "AUNITS V;" 0
talk 18 "MOV S_ECDV,MKA;" 0
talk 18 "AUNITS DBM;" 0
talk 18 "IF F_FRQ,LE,7333.33333333E6 THEN" 0
talk 18 "CF UP;TS;" 0
talk 18 "MKPK HI;" 0
talk 18 "PP;TS;" 0
talk 18 "MOV T_HRDD,MKA;" 0
talk 18 "AUNITS V;" 0
talk 18 "MOV T_HRDV,MKA;" 0
talk 18 "AUNITS DBM;" 0
talk 18 "ENDIF;" 0
talk 18 "MPY S_ECSQ,S_ECDV,S_ECDV;" 0
talk 18 "MPY T_HRSQ,T_HRDV,T_HRDV;" 0
talk 18 "ADD S_ECSQ,S_ECSQ,T_HRSQ;" 0
talk 18 "SQR S_ECSQ,S_ECSQ;" 0
talk 18 "MPY S_ECSQ,S_ECSQ,100;" 0
talk 18 "DIV D_ISTRN,S_ECSQ,F_RSTV;" 0
talk 18 "SUB S_ECDD,S_ECDD,F_RSTD;" 0
talk 18 "SUB T_HRDD,T_HRDD,F_RSTD;" 0
talk 18 "ENDIF;@;FUNCDEF H_DISP,@CLRW TRA;TRDSP TRA OFF;ANNOT OFF;GRAT OFF;MKOFF ALL;" 0
talk 18 "DIV F_FRQ,F_FRQ,1E6;" 0
talk 18 "D3;PU;PA 208,448;TEXT /<===== FUNDAMENTAL:/;" 0
talk 18 "PA 352,384;TEXT /Freq  /;DSPLY F_FRQ,9.3;TEXT /  MHz/;" 0
talk 18 "PA 352,352;TEXT /Ampl   /;DSPLY F_RSTD,6.1;TEXT /  dBm/;" 0
talk 18 "IF F_FRQ,LE,11000 THEN" 0
talk 18 "PA 320,256;TEXT /HARMONICS:/;" 0
talk 18 "PA 352,192;TEXT /2nd    /;" 0
talk 18 "DSPLY S_ECDD,6.1;TEXT /  dBc/;" 0
talk 18 "IF F_FRQ,LE,7333.33333333 THEN" 0
talk 18 "PA 352,160;TEXT /3rd    /;" 0
talk 18 "DSPLY T_HRDD,6.1;TEXT /  dBc/;" 0
talk 18 "ENDIF;" 0
talk 18 "PA 48,576;TEXT /Total Harmonic Distortion = /;" 0
talk 18 "DSPLY D_ISTRN,5.2;TEXT / %/;" 0
talk 18 "ELSE" 0
talk 18 "PA 320,256;TEXT /NO HARMONICS IN BAND/;" 0
talk 18 "ENDIF;" 0
talk 18 "M_PRMPT;" 0
talk 18 "@;FUNCDEF H_SHOW,@MPY FA,F_FRQ,.998E6;MPY FB,F_FRQ,1.002E6;" 0
talk 18 "ADD RL,F_RSTD,10;" 0
talk 18 "D1;PU;PA 0,228;PD;PR 0,500,200,0,0,-500,-200,0" 0
talk 18 "PU 67,0;PD 0,500;PU 67,0;PD 0,-500;" 0
talk 18 "PU 66,100;PD -200,0;PU 0,100;PD 200,0;" 0
talk 18 "PU 0,100;PD -200,0;PU 0,100;PD 200,0;" 0
talk 18 "D2;PU;PA 0,704;DSPLY RL,6.1;PU;PA 16,672;TEXT $  dBm" 0
talk 18 "20dB/$;" 0
talk 18 "REPEAT" 0
talk 18 "TS;" 0
talk 18 "COMPRESS H_FTRC,TRA,POS;" 0
talk 18 "DIV H_FTRC,H_FTRC,2;" 0
talk 18 "ADD H_FTRC,H_FTRC,228;" 0
talk 18 "MOV H_FTRC[200],1056;" 0
talk 18 "MOV TRB,H_FTRC;" 0
talk 18 "VIEW TRB;" 0
talk 18 "UNTIL H_SPAD,EQ,1;" 0
talk 18 "@;FUNCDEF P_AGE,@IP;ANNOT OFF;GRAT OFF;BLANK TRA;D3;@;FUNCDEF P_AUSE,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "Press the <Hz> key for more      /;" 0
talk 18 "EP;@;FUNCDEF H_OLD,@PU;PA 0,64;TEXT / -------------------------------------------" 0
talk 18 "Press the <Hz> key to continue     /;" 0
talk 18 "EP;@;FUNCDEF C_PRMPT,@;PU;PA 64,96;TEXT /Connect Calibrator signal to RF input/;" 0
talk 18 "H_OLD;@;FUNCDEF F_RAME,@EM;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "PU;PA 5,735;PD;PR 705,0,0,-735,-705,0,0,735;" 0
talk 18 "@;FUNCDEF M_PRMPT,@;D2;PU;PA 256,32;TEXT /<INSTR PRESET> <SHIFT>  100  <Hz>/;" 0
talk 18 "PA 352,0;TEXT /To Return To The Menu/;" 0
talk 18 "@;FUNCDEF H_DIST,@H_INTRO;H_SETUP;H_MEAS;H_DISP;H_SHOW;@;KEYDEF 101,H_DIST;" 0
echo Upload complete.  You can run this program by pressing Shift 101 Hz 
echo on the analyzer's front panel.
echo.
echo User RAM bytes left:
query 18 "MEM?"
echo.
