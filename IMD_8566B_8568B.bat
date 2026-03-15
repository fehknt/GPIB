@echo off
echo.
echo This batch file transmits the downloadable program T_HIRDIMOD
echo (25.12.84) to the nonvolatile user RAM on an HP 8566B or 8568B 
echo spectrum analyzer.  
echo.
echo See HP product note 8566B/8568B-1 for more information.
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
talk 18 "VARDEF O_PTRANGE,0;VARDEF T_ONE,0;VARDEF T_TWO,0;" 0
talk 18 "VARDEF O_PTRANGE,0;VARDEF T_ONE,0;VARDEF T_TWO,0;" 0
talk 18 "VARDEF T_ONEAMP,0;VARDEF T_TWOAMP,0;VARDEF H_EIGHT,0;"  0
talk 18 "VARDEF D_IFF,0;VARDEF N_OISE,0;"  0
talk 18 "VARDEF I_MRDBC,0;VARDEF I_MLDBC,0;"  0
talk 18 "VARDEF I_TOI,0;" 0
talk 18 "VARDEF H_OLD,0;VARDEF L_EFT,0;"  0
talk 18 "TRDEF S_TORE,1008;"  0

talk 18 "FUNCDEF T_ESTTONES, @"  0
talk 18 "IP;SNGLS;EM;FA 10 MZ;FB 500 MZ;"  0
talk 18 "TS;MKPK HI;MKTRACK ON;"  0
talk 18 "SP30KZ;VB300HZ;TS;"  0
talk 18 "MKTRACK OFF;TS;MKPK HI;"  0
talk 18 "IF MA,GT,RL THEN;"  0
talk 18 " REPEAT;"  0
talk 18 " RL UP;TS;MKPK HI;"  0
talk 18 " UNTIL MA,LE,RL;"  0
talk 18 "ENDIF;"  0
talk 18 "ADD O_PTRANGE,MA,38;"  0
talk 18 "IF AT,LT,O_PTRANGE THEN;"  0
talk 18 " REPEAT;"  0
talk 18 " AT UP;"  0
talk 18 " UNTIL AT,GE,O_PTRANGE;"  0
talk 18 "ENDIF;"  0
talk 18 "MKRL;TS;"  0
talk 18 "MOV T_ONE,MF;"  0
talk 18 "MOV T_ONEAMP,MA;" 0
talk 18 "MKPX 10DB;" 0
talk 18 "MKPK NH;" 0
talk 18 "MOV T_TWO,MF;" 0
talk 18 "MOV T_TWOAMP,MA;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF E_QUALAMP, @"  0
talk 18 "SUB H_EIGHT,T_ONEAMP,T_TWOAMP;"  0
talk 18 "IF H_EIGHT,LT,0;"  0
talk 18 " THEN SUB H_EIGHT,0,H_EIGHT;"  0
talk 18 "ENDIF;"  0
talk 18 "IF H_EIGHT,GT,2 THEN;"  0
talk 18 " CONTS;DA3072;D3;PU;PA100,600;TEXT /ADJUST TEST TONES FOR EQUAL/;"  0
talk 18 " PU;PA100,550;TEXT /AMPLITUDE AND PRESS THE HZ KEY/;"  0
talk 18 " PU;PA100,350;TEXT /(CURRENT LEVELS =      /;" 0
talk 18 " PU;PA390,350;DSPLY T_ONEAMP,5.2;PU;PA 510,350;DSPLY T_TWOAMP,5.2;PU;PA 620,350;TEXT /)/; HD;"  0
talk 18 " SS EP;"  0
talk 18 " EM;SNGLS;TS;MKPK HI;"  0
talk 18 " MOV T_ONE,MF;" 0
talk 18 " MOV T_ONEAMP,MA;"  0
talk 18 " MKPK NH;"  0
talk 18 " MOV T_TWO,MF;"  0
talk 18 " MOV T_TWOAMP,MA;"  0
talk 18 "ENDIF;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF P_RODUCTS, @"  0
talk 18 "IF T_ONE,GE,T_TWO THEN ;"  0
talk 18 " XCH T_ONE,T_TWO;"  0
talk 18 " XCH T_ONEAMP,T_TWOAMP;"  0
talk 18 "ENDIF;"  0
talk 18 "SUB D_IFF,T_TWO,T_ONE;"  0
talk 18 "DIV H_OLD,D_IFF,2;"  0
talk 18 "ADD CF,T_ONE,H_OLD;"  0
talk 18 "IF D_IFF,LT,3000 THEN;"  0
talk 18 " SP DN;"  0
talk 18 "ENDIF;"  0
talk 18 "TS;MOV S_TORE,TRA;"  0
talk 18 "SAVES 2;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF M_EASURE, @"  0
talk 18 "ADD CF,T_TWO,D_IFF;"  0
talk 18 "SP DN;SP DN;SP DN;TS;"  0
talk 18 "IF D_IFF,GE,3000 THEN;"  0
talk 18 " MKPK HI;MKRL;MOV VB,RB;"  0
talk 18 " VB;DN;TS;MKPK HI;"  0
talk 18 "ELSE MKN;SP DN;TS;MKPK HI;"  0
talk 18 " MKRL;VB;DN;TS;MKPK HI;"  0
talk 18 "ENDIF;"  0
talk 18 "SUB I_MRDBC,MA,T_TWOAMP;"  0
talk 18 "SUB CF,T_ONE,D_IFF;"  0
talk 18 "TS;MKPK HI;"  0
talk 18 "MOV L_EFT,MA;"  0
talk 18 "SUB I_MLDBC,MA,T_ONEAMP;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF R_EPORT, @"  0
talk 18 "DIV I_TOI,I_MLDBC,-2;" 0
talk 18 "ADD I_TOI,I_TOI,T_ONEAMP;" 0
talk 18 "VIEW TRA;RCLS 2;MOV TRA,S_TORE;"  0
talk 18 "DA3072;D2;PU;PA260,800;TEXT /INTERMODULATION DISTORTION/;"  0
talk 18 "PU;PA200,730;TEXT /TEST TONE LEVEL =      /;PA590,730;DSPLY T_ONEAMP,5.2;PU;PA 700,730;TEXT /dBm/;"  0
talk 18 "PU;PA200,700;TEXT /TEST TONE SEPARATION = /;PA560,700;DSPLY D_IFF,6.0;PU;PA700,700;TEXT /Hz/;"  0
talk 18 "PU;PA200,630;TEXT /2F1-F2/;PU;PA 564,630;DSPLY I_MLDBC,5.2;PU;PA700,630;TEXT /dBc/;"  0
talk 18 "PU;PA200,600;TEXT /2F2-F1/;PU;PA 564,600;DSPLY I_MRDBC,5.2;PU;PA700,600;TEXT /dBc/;"  0
talk 18 "PU;PA200,570;TEXT /IP3/;PU;PA 590,570;DSPLY I_TOI,5.2;PU;PA700,570;TEXT /dBm/;"  0
talk 18 "PU;PA300,120;TEXT /Press SHIFT 2 Hz to repeat test/;HD;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF N_OTHIRD, @"  0
talk 18 "RCLS 2;MOV TRA,S_TORE;"  0
talk 18 "EM;D3;DA3072;PU;PA100,600;"  0
talk 18 "TEXT /THIRD ORDER INTERMODULATION PRODUCTS/;"  0
talk 18 "PU;PA100,550;TEXT /ARE AT OR BELOW THE NOISE LEVEL/;"  0
talk 18 "PU;PA100,450;TEXT /Press SHIFT 2 Hz to repeat test/; HD;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF C_HECK, @"  0
talk 18 "SMOOTH TRA,32;MKMIN;"  0
talk 18 "MOV N_OISE,MA;ADD N_OISE,N_OISE,15;"  0
talk 18 "IF L_EFT,LE,N_OISE THEN;"  0
talk 18 " N_OTHIRD;"  0
talk 18 " ELSE R_EPORT;"  0
talk 18 "ENDIF;"  0
talk 18 "@;"  0

talk 18 "FUNCDEF T_HIRDIMOD, @"  0
talk 18 "T_ESTTONES;E_QUALAMP;P_RODUCTS;M_EASURE;C_HECK;"  0
talk 18 "@;"  0
talk 18 "KEYDEF 2,T_HIRDIMOD;" 0
echo Upload complete.  You can run this program by pressing Shift 2 Hz 
echo on the analyzer's front panel.
echo.
echo User RAM bytes left:
query 18 "MEM?"

