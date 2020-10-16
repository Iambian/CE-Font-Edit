#ifndef __MAIN_INCLUDEGUARD__
#define __MAIN_INCLUDEGUARD__

#include <stdint.h>
#include <graphx.h>
#include "defs.h"


struct fontdata_st {
	uint8_t codepoints[256];
	uint8_t lfont[255][LFONT_SIZE];
	uint8_t sfont[255][SFONT_SIZE];
};
struct editor_st {
	uint8_t codepoint;
	uint8_t fonttype;
	uint8_t update;
	uint8_t context;
	uint8_t oldcx;
	uint8_t	gridx;
	uint8_t gridy;
	uint8_t	xlim;
	uint8_t ylim;
	uint8_t cursor;
	uint8_t haschanged;
	uint8_t verifiedfile;
	
};

struct filelist_st {
	char		filetype;
	char		name[9];
	void		*fontstruct;
	uint16_t	size;
};

uint8_t	GetScan(void);
void	PrintHexPair(uint8_t num);
void	PrintLargeChar(char c, void *fontstruct);
void	PrintSmallChar(char c, void *fontstruct);
uint8_t	PopulateFileList(uint8_t filetype);
void	GetSmallCharWidth(char c, void *fontstruct);
void	SetupPalette(void);
void*	GetCharLocation(char c, void *fontstruct, uint8_t fontid);
void	PrintChar(char c, void *fontstruct, uint8_t fontid);
uint8_t	GetFontBit(uint8_t x, uint8_t y, uint8_t fonttype);
void	SetFontBit(uint8_t x, uint8_t y, uint8_t fonttype);
void	ResFontBit(uint8_t x, uint8_t y, uint8_t fonttype);
uint8_t	InvFontBit(uint8_t x, uint8_t y, uint8_t fonttype);
void*	GetDefaultLocation(char c, uint8_t fonttype);
void	LoadCursors(void);
char	KeyToChar(uint8_t key, uint8_t alphastate);
uint8_t LookupMenufile(void);
void	SaveMenufile(void);


extern uint8_t fonttype;
extern uint8_t numcodes;
extern uint8_t editbuf[];
extern struct fontdata_st fontdata;
extern struct editor_st edit;
extern char namebuf[];
extern int filelist_len;
extern struct filelist_st filelist[]; 
extern struct filelist_st curfile;
extern struct filelist_st menufile;
#endif
