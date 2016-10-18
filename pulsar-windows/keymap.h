#pragma once
#include <map>

std::map<uint8_t, uint8_t> keymap;

void InitKeymap()
{
	keymap[27] = 0x1B;      //  ESC
	keymap[112] = 0x70;     //  F1
	keymap[113] = 0x71;     //  F2
	keymap[114] = 0x72;      // F3
	keymap[115] = 0x73;     //  F4
	keymap[116] = 0x74;      // F5
	keymap[117] = 0x75;    //   F6
	keymap[118] = 0x76;      // F7
	keymap[119] = 0x77;     //  F8
	keymap[120] = 0x78;      // F9
	keymap[121] = 0x79;     //  F10
	keymap[122] = 0x7A;    //   F11
	keymap[123] = 0x7B;     // F12



	keymap[192] = 0xC0;       //  `
	keymap[49] = 0x31;        // 1
	keymap[50] = 0x32;        // 2
	keymap[51] = 0x33;       //  3
	keymap[52] = 0x34;        // 4
	keymap[53] = 0x35;        //5
	keymap[54] = 0x36;       //  6
	keymap[55] = 0x37;      //  7
	keymap[56] = 0x38;       //  8
	keymap[57] = 0x39;    //    9
	keymap[48] = 0x30;         //0 
	keymap[189] = 0xBD;      //-
	keymap[187] = 0xBB;     //+
	keymap[8] = 0x08;     //delete


	keymap[9] = 0x09;         // Tab;
	keymap[81] = 0x51;       //    Q
	keymap[87] = 0x57;        //   W
	keymap[69] = 0x45;         //  E
	keymap[82] = 0x52;        //   R
	keymap[84] = 0x54;        //   T
	keymap[89] = 0x59;        //   Y
	keymap[85] = 0x55;         //  U
	keymap[73] = 0x49;        //   I
	keymap[79] = 0x4F;        //   O
	keymap[80] = 0x50;        //   P
	keymap[219] = 0xDB;     //     [ 
	keymap[221] = 0xDD;       //   ]
	keymap[13] = 0x0D;        // Enter

	keymap[20] = 0x14;     // Caps Lock
	keymap[65] = 0x41;     //  A
	keymap[83] = 0x53;      // S
	keymap[68] = 0x44;   //    D
	keymap[70] = 0x46;       //F
	keymap[71] = 0x47;      // G
	keymap[72] = 0x48;     //  H
	keymap[74] = 0x4A;      // J
	keymap[75] = 0x4B;      // K
	keymap[76] = 0x4C;    //   L
	keymap[186] = 0xBA;    //   ;
	keymap[222] = 0xDE;   //    " 
	keymap[220] = 0xDC;    //   \

	keymap[16] = 0x10;         //   SHIFT key
	keymap[90] = 0x5A;        //   Z
	keymap[88] = 0x58;         //  X
	keymap[67] = 0x43;         //  C
	keymap[86] = 0x56;        //   V
	keymap[66] = 0x42;        //   B
	keymap[78] = 0x4E;         //  N
	keymap[77] = 0x4D;       //   M
	keymap[188] = 0xBC;     //   <
	keymap[190] = 0xBE;      //   >
	keymap[191] = 0xBF;      //   ?
	keymap[16] = 0x10;          // SHIFT key

	keymap[17] = 0x11;      //  CTRL key
	keymap[91] = 0xA4;       //Left MENU
	keymap[18] = 0x12;        //ALT key
	keymap[32] = 0x20;      //  Spacebar
	keymap[18] = 0x12;    //    ALT key
	keymap[92] = 0xA5;     //  Right MENU key
	keymap[93] = 0x02;    //  Right mouse button
	keymap[17] = 0x11;     //   CTRL key

	keymap[145] = 0x91;
	keymap[19] = 0;
	keymap[144] = 0x90;
	keymap[111] = 0xBF;
	keymap[106] = 0x6A;
	keymap[109] = 0xBD;
	keymap[45] = 0x2D;
	keymap[36] = 0x24;
	keymap[33] = 0x21;
	keymap[103] = 0x67;
	keymap[104] = 0x68;
	keymap[105] = 0x69;
	keymap[107] = 0xBB;
	keymap[46] = 0x2E;
	keymap[35] = 0x23;
	keymap[34] = 0x22;
	keymap[100] = 0x64;
	keymap[101] = 0x65;
	keymap[102] = 0x66;
	keymap[38] = 0x26;
	keymap[97] = 0x61;
	keymap[98] = 0x62;
	keymap[99] = 0x63;
	keymap[13] = 0x0D;
	keymap[37] = 0x25;
	keymap[40] = 0x28;
	keymap[39] = 0x27;
	keymap[96] = 0x60;
	keymap[110] = 0xBE;
}