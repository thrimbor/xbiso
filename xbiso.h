#define MAX_DEPTH	0xFF				//maximum directory recursion
#define	BUFFSIZE	0x40000				//256k , seems to segf <=64k
#define	XMEDHEAD	"\x4d\x49\x43\x52\x4f\x53\x4f\x46\x54\x2a\x58\x42\x4f\x58\x2a\x4d\x45\x44\x49\x41"
#define FILE_RO		0x01
#define FILE_HID	0x02
#define FILE_SYS	0x04
#define FILE_DIR	0x10
#define	FILE_ARC	0x20
#define FILE_NOR	0x80
