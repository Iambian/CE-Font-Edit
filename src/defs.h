#ifndef __DEFS_INCLUDEGUARD__
#define __DEFS_INCLUDEGUARD__

#define VERSION_INFO "v 0.2"
#define SAVE_VERSION 1

#define max(a,b) ((a>b)?(a):(b))

/* Defines regarding colors */
#define COLOR_DARKER (0<<6)
#define COLOR_DARK (1<<6)
#define COLOR_LIGHT (2<<6)
#define COLOR_LIGHTER (3<<6)

#define COLOR_RED (3<<4)
#define COLOR_MAROON (2<<4)
#define COLOR_DARKRED (1<<4)

#define COLOR_LIME (3<<2)
#define COLOR_GREEN (2<<2)
#define COLOR_DARKGREEN (1<<2)

#define COLOR_BLUE (3<<0)
#define COLOR_NAVY (2<<0)
#define COLOR_DARKBLUE (1<<0)

#define COLOR_MAGENTA (COLOR_RED|COLOR_BLUE)
#define COLOR_PURPLE (COLOR_MAROON|COLOR_NAVY)
#define COLOR_YELLOW (COLOR_RED|COLOR_LIME)
#define COLOR_CYAN (COLOR_LIME|COLOR_BLUE)
#define COLOR_LIGHTGRAY (COLOR_RED|COLOR_BLUE|COLOR_LIME)
#define COLOR_GRAY (COLOR_MAROON|COLOR_GREEN|COLOR_NAVY)
#define COLOR_DARKGRAY (COLOR_DARKRED|COLOR_DARKGREEN|COLOR_DARKBLUE)
#define COLOR_BLACK 0
#define COLOR_ROSE (COLOR_RED|COLOR_DARKBLUE|COLOR_DARKGREEN|COLOR_LIGHTER)
#define COLOR_WHITE (COLOR_LIGHTGRAY|COLOR_LIGHTER)

#define TRANSPARENT_COLOR (COLOR_LIGHTER|COLOR_MAGENTA)
#define COLOR_TRANS	TRANSPARENT_COLOR

/* In-program color equates */
//Main screen turn on
#define CLR_MAIN_BODY (COLOR_MAROON|COLOR_GREEN|COLOR_BLUE|COLOR_LIGHTER)
#define CLR_MAIN_SIDE (COLOR_MAROON|COLOR_GREEN|COLOR_BLUE|COLOR_LIGHT)
#define CLR_MAIN_TOP  (COLOR_DARKRED|COLOR_DARKGREEN|COLOR_NAVY|COLOR_LIGHT)
#define CLR_MAIN_TEXT  COLOR_BLACK

#define CLR_GRID_ON		 COLOR_BLACK
#define CLR_GRID_OFF	 COLOR_WHITE
#define CLR_GRID_SLEEP	(COLOR_GRAY|COLOR_LIGHT)

#define CLR_OUTOFBOUNDS  COLOR_ROSE

#define CLR_MENUDEFAULT	(COLOR_LIME|COLOR_MAROON|COLOR_NAVY)
#define CLR_MENU_BODYBG (CLR_MENUDEFAULT|COLOR_LIGHTER)
#define CLR_MENU_BODYFG	 COLOR_BLACK
#define CLR_MENU_BORDER	 CLR_MENUDEFAULT
#define CLR_MENU_SLCTBG	(COLOR_BLACK|COLOR_LIGHTER)
#define CLR_MENU_SLCTFG	 COLOR_WHITE
#define CLR_MENU_BLOKBG  NONE
#define CLR_MENU_BLOKFG (COLOR_GRAY|COLOR_LIGHTER)

#define CLR_TABDEFAULT 	   NONE
#define CLR_TAB_BODYBG 	  (COLOR_LIGHTGRAY|COLOR_LIGHT)
#define CLR_TAB_BODYFG 	   COLOR_BLACK
#define CLR_TAB_ACTIVEBG  (COLOR_GREEN|COLOR_DARKBLUE|COLOR_DARKRED)
#define CLR_TAB_ACTIVEFG   COLOR_WHITE
#define CLR_TAB_BLOCKEDBG (COLOR_GRAY|COLOR_LIGHTER)
#define CLR_TAB_BLOCKEDFG  COLOR_GRAY


/* Scan code equates */
#define sk_Down		0x01
#define sk_Left		0x02
#define sk_Right	0x03
#define sk_Up		0x04
#define sk_DnLeft	0x05
#define sk_DnRight	0x06
#define sk_UpLeft	0x07
#define sk_UpRight	0x08
#define sk_Enter	0x09
#define sk_Add		0x0A
#define sk_Sub		0x0B
#define sk_Mul		0x0C
#define sk_Div		0x0D
#define sk_Power	0x0E
#define sk_Clear	0x0F
#define sk_Chs		0x11
#define sk_3		0x12
#define sk_6		0x13
#define sk_9		0x14
#define sk_RParen	0x15
#define sk_Tan		0x16
#define sk_Vars		0x17
#define sk_DecPnt	0x19
#define sk_2		0x1A
#define sk_5		0x1B
#define sk_8		0x1C
#define sk_LParen	0x1D
#define sk_Cos		0x1E
#define sk_Prgm		0x1F
#define sk_Stat		0x20
#define sk_0		0x21
#define sk_1		0x22
#define sk_4		0x23
#define sk_7		0x24
#define sk_Comma	0x25
#define sk_Sin		0x26
#define sk_Apps		0x27
#define sk_Graphvar	0x28
#define sk_Store	0x2A
#define sk_Ln		0x2B
#define sk_Log		0x2C
#define sk_Square	0x2D
#define sk_Recip	0x2E
#define sk_Math		0x2F
//#define sk_Alpha	0x30
#define sk_Graph	0x31
#define sk_Trace	0x32
#define sk_Zoom		0x33
#define sk_Window	0x34
#define sk_Yequ 	0x35
//#define sk_2nd	0x36
#define sk_Mode		0x37
#define sk_Del		0x38
//Extended keyboard
#define ctrl_Alpha	0x80
#define ctrl_2nd	0x40

/* Renamed keys to tabs */
#define tk_File		sk_Yequ
#define tk_Edit		sk_Window
#define tk_Prev		sk_Zoom
#define tk_Next		sk_Trace
#define tk_Help		sk_Graph


/* Dimensions and size fields */
#define SFONT_SIZE (2*12+1)
#define LFONT_SIZE (2*14)
#define SFONT_T	0
#define LFONT_T 1

#define LARGEST_BUF max(SFONT_SIZE,LFONT_SIZE)

/* Contexts */
#define CX_EDITING	0x01
#define CX_BROWSING	0x02
#define CX_FILEVIEW	0x04
#define CX_FILEEDIT	0x08

#define FILEACT_OPEN	0x00
#define FILEACT_SAVEAS	0x01

/* Menu defines */
#define UPD_ALL		(0xFF & ~UPD_PREVIEW)
#define UPD_TABS	0x01
#define	UPD_SIDE	0x02
#define	UPD_GRID	0x04
#define UPD_TOP		0x08
#define UPD_PREVIEW	0x10

#define OBJ_INACTIVE 0x00
#define OBJ_ACTIVE	0x01
#define OBJ_BLOCKED	0x02
#define	OBJ_LOCKED	0x04
#define	OBJ_ALOCKED	0x08

/* Cursor defines. Use gfx_PrintChar to display them. */
#define CURSOR_SOLID	1
#define CURSOR_ALPHA	2
#define CURSOR_LOWER	3
#define CURSOR_NUMBER	4
#define CURSOR_CHECKER	5

#define CURSOR_TIMEON	30
#define CURSOR_TIMEMAX	60

/* Defines regarding dimensions of things */
#define TITLE_LEFT	0
#define TITLE_TOP	0
#define TITLE_WIDTH LCD_WIDTH
#define TITLE_HEIGHT 12
#define GRID_LEFT 8
#define GRID_TOP 24
#define GRID_WIDTH 196
#define GRID_HEIGHT 168
#define SIDEBAR_LEFT (LCD_WIDTH-SIDEBAR_WIDTH)
#define SIDEBAR_TOP TITLE_HEIGHT
#define SIDEBAR_WIDTH 112
#define SIDEBAR_HEIGHT (LCD_HEIGHT-(TITLE_HEIGHT+TAB_HEIGHT))
#define TAB_LEFT	1
#define TAB_TOP		(LCD_HEIGHT-TAB_HEIGHT)
#define TAB_WIDTH	((LCD_WIDTH/5)-2)
#define TAB_HEIGHT	24
//Dimensions of menu-only updating area
#define TABC_LEFT	1
#define TABC_TOP	(LCD_HEIGHT-24)
#define TABC_WIDTH 	(LCD_WIDTH-2)
#define TABC_HEIGHT	24
#define SBAR_LEFT (LCD_WIDTH-SIDEBAR_WIDTH)
#define SBAR_TOP TOPBAR_HEIGHT
#define SBAR_WIDTH 112
#define SBAR_HEIGHT (LCD_HEIGHT-(TOPBAR_HEIGHT+TAB_HEIGHT))





#endif