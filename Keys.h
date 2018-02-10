//CONSTANTES:

#define				keyArrowLeft			0x7b
#define				keyArrowRight			0x7c
#define				keyArrowUp				0x7e
#define				keyArrowDown			0x7d
#define				keyCommand				0x37
#define				keyOption				0x3A
#define				keyShift				0x38
#define				keyControl				0x3B
#define				keySpace				0x31
#define				keyF1					0x7A

//MACROS:

#define IsKeyDown(keyMap, theKey) (((unsigned char *)(keyMap))[(theKey) / 8] & 1 << ((theKey) % 8))