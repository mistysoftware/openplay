#include "Dialogs.r"

data 'MBAR' (128) {
	$"0003 0080 0081 0082"                                                                                /* ...�.�.� */
};

data 'MENU' (128) {
	$"0080 0000 0000 0000 0000 FFFF FFFF 0114 0641 626F 7574 C900 0000 0000"                              /* .�........����...About�..... */
};

data 'MENU' (129) {
	$"0081 0000 0000 0000 0000 FFFF FFBF 0446 696C 650B 4F70 656E 2041 6374 6976 6500"                    /* .�........����.File.Open Active. */
	$"4F00 000C 4F70 656E 2050 6173 7369 7665 0000 0000 0F4F 7065 6E20 4163 7469 7665"                    /* O...Open Passive.....Open Active */
	$"2055 49C9 0000 0000 104F 7065 6E20 5061 7373 6976 6520 5549 C900 0000 0005 436C"                    /*  UI�.....Open Passive UI�.....Cl */
	$"6F73 6500 5700 0001 2D00 0000 0004 5175 6974 0051 0000 00"                                          /* ose.W...-.....Quit.Q... */
};

data 'MENU' (130) {
	$"0082 0000 0000 0000 0000 FFFF FFFF 0753 7065 6369 616C 0B53 656E 6420 5061 636B"                    /* .�........����.Special.Send Pack */
	$"6574 0050 0000 0B53 656E 6420 5374 7265 616D 0053 0000 0749 7341 6C69 7665 0000"                    /* et.P...Send Stream.S...IsAlive.. */
	$"0000 1541 6363 6570 7469 6E67 2043 6F6E 6E65 6374 696F 6E73 0000 0000 1241 6374"                    /* ...Accepting Connections.....Act */
	$"6976 6520 456E 756D 6572 6174 696F 6E00 0000 000B 5365 7420 5469 6D65 6F75 7400"                    /* ive Enumeration.....Set Timeout. */
	$"0000 000F 4765 7420 4D6F 6475 6C65 2049 6E66 6F00 0000 0011 4765 7420 436F 6E66"                    /* ....Get Module Info.....Get Conf */
	$"6967 2053 7472 696E 6700 0000 0000"                                                                 /* ig String..... */
};

data 'MENU' (200, "Popup menu") {
	$"00C8 0000 0000 0000 0000 FFFF FFFF 0B70 6C61 6365 686F 6C64 6572 00"                                /* .�........����.placeholder. */
};

data 'WIND' (128) {
	$"0062 0092 018A 01F8 0008 0000 0100 0000 0000 0A4E 6577 2057 696E 646F 77"                           /* .b.�.�.�..........�New Window */
};

resource 'dlgx' (128) {
	versionZero {
		15
	}
};

resource 'dlgx' (129) {
	versionZero {
		15
	}
};

resource 'dlgx' (130) {
	versionZero {
		15
	}
};

data 'DLOG' (128) {
	$"0028 0014 00A0 0104 0412 0000 0100 0000"
	$"0000 0080 0000 300A"
};

data 'DLOG' (129) {
	$"0028 0014 0148 01A8 0412 0100 0100 0000"
	$"0000 0081 0000 300A"
};

data 'DLOG' (130) {
	$"0028 0014 00A0 0104 0412 0100 0100 0000"
	$"0000 0082 0000 300A"
};

data 'DITL' (128) {
	$"0002 0000 0000 0059 009F 006D 00E5 0402"
	$"4F4B 0000 0000 0059 004E 006D 0094 0406"
	$"4361 6E63 656C 0000 0000 0014 000F 0028"
	$"00C3 0702 0080"
};

data 'DITL' (129) {
	$"0006 0000 0000 00DF 0140 00F3 018B 0408"
	$"5573 6520 5468 6973 0000 0000 0103 0140"
	$"0117 018B 0406 4361 6E63 656C 0000 0000"
	$"0012 000C 00E4 00AA 8000 0000 0000 0012"
	$"00B3 0012 00B3 8000 0000 0000 0002 000B"
	$"0012 0056 8809 4761 6D65 204C 6973 7402"
	$"0000 0000 0002 00B3 0012 00FE 880E 4D6F"
	$"6475 6C65 2053 6563 7469 6F6E 0000 0000"
	$"00E9 0032 00FD 00AA 040D 4A6F 696E 2053"
	$"656C 6563 7465 6400"
};

data 'DITL' (130) {
	$"0003 0000 0000 0059 00A0 006D 00E6 0402"
	$"4F4B 0000 0000 0059 0050 006D 0096 0406"
	$"4361 6E63 656C 0000 0000 001C 0058 002C"
	$"00A3 1000 0000 0000 001C 0016 002D 0055"
	$"8808 5469 6D65 6F75 743A"
};

data 'CNTL' (128, "Protocols") {
	$"0000 0000 0014 00B4 0000 0100 0046 00C8 03F1 0000 0000 0950 726F 746F 636F 6C3A"                    /* .......�.....F.�.�....�Protocol: */
};
