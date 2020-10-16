/*
 *--------------------------------------
 * Program Name:
 * Author:
 * License:
 * Description:
 *--------------------------------------
*/
//DEFINE THE LABEL BELOW TO STRIP OUT SOME CONTENT FOR POSSIBLY
//FASTER COMPILE TIMES DURING RAPID ITERATIVE TESTING
//#define FASTERCOMPILE

/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External library headers */
#include <debug.h>
#include <keypadc.h>
#include <graphx.h>
#include <fileioc.h>
#include <compression.h>

#include "main.h"
#include "defs.h"
#include "resostub.h"
#include "stalstub.h"

/* Put your function prototypes here */
void	menu_Redraw(void);
void 	font_NewFile(void);
void	font_LoadEditBuffer(void);
void 	font_LoadDefaultEditBuffer(void);
uint8_t	menu_GetMenuHeight(char **s);
uint8_t	menu_GetMenuWidth(char **s);
void	menu_DrawMenuBox(int x, uint8_t y, int w, uint8_t h);
void	menu_DrawNotice(char **sa);
uint8_t	menu_DrawTabMenu(char **sa, uint8_t pos);
void	font_CommitEditBuffer(void);
int		menu_DrawFileMenu(char **sa, uint8_t id);
int		font_SaveFile(void);




/* Put all your globals here */


uint8_t fonttype;
uint8_t editbuf[LARGEST_BUF];
uint8_t numcodes;
struct fontdata_st fontdata;  //fontdata.[codepoints,lfont,sfont]
struct editor_st edit;
int filelist_len;
struct filelist_st filelist[255]; 
struct filelist_st curfile;
struct filelist_st menufile;
char namebuf[10];

char *file_menutext[] = {"\1New","\1Open...","\1Save","\1Save As...","\3-","\1Preview","\1Export","\1Import","\3-","\1Exit","\0"};
char *edit_menutext[] = {"\1Undo","\1Redo","\1Copy","\1Paste","\1Use TI-OS Glyph","\1Change Font Size","\1Delete Codepoint","\1Clear Grid","\0"};
char *help_menutext[] = {"\1Using the editor","\1File operations","\1Edit operations","\1Hotkeys 1","\1Hotkeys 2","\1Hotkeys 3","\1About the rawrfs","\0"};

char *notice_saved[]		= {"New file has","been saved!","\0"};
char *notice_overwrite[]	= {"Old file has","been overwritten!","\0"};
char *overwrite_menu[]		= {"\1Do not save","\1Overwrite existing file","\0"};


int resosize;
int stalsize;

void main(void) {
	uint8_t k,ck,u,hold,gx,gy,fid,val,i,j,*buf;
	int delta,len,rval;
    /* Fill in the body of the main function here */
	gfx_Begin();
	SetupPalette();
	LoadCursors();
	gfx_SetTransparentColor(COLOR_TRANS);
	gfx_SetTextTransparentColor(COLOR_TRANS);
	gfx_SetTextBGColor(COLOR_TRANS);
	gfx_SetDrawBuffer();
	/* ---------------------- */
	stalsize = sizeof stalstub;	//Important for ASM program stub that applies it.
	resosize = sizeof resostub; //Same as above. Or else saving won't work.
	
	gfx_FillScreen(COLOR_BLUE);
	font_NewFile();
	
	
	hold = k = 0;
	while (1) {
		k 	= GetScan();
		ck 	= k & 0xC0;
		k  &= 0x3F;
		u = edit.update;
		
		if (k == sk_Mode) break;
		
		if (k|ck) {
			/* ################ HANDLE EDITING CONTEXT ################### */
			//dbg_sprintf(dbgout,"keyboard value: %i\n",k);
			if (edit.context & CX_EDITING) {
				if ((k == sk_Up)    && (edit.gridy > 0)) --edit.gridy;
				if ((k == sk_Left)  && (edit.gridx > 0)) --edit.gridx;
				if ((k == sk_Down)  && (edit.gridy+1 < edit.ylim)) ++edit.gridy;
				if ((k == sk_Right) && (edit.gridx+1 < edit.xlim)) ++edit.gridx;
				if (k && k<9) u |= UPD_GRID;	//Shortcut for "any arrow key"
				
				if (ck & ctrl_2nd) {
					gx = edit.gridx;
					gy = edit.gridy;
					fid = edit.fonttype;
					if (hold) {
						if (hold==1) 	SetFontBit(gx,gy,fid);
						else			ResFontBit(gx,gy,fid);
					} else {
						hold = 1+ !InvFontBit(gx,gy,fid);	//1=was set, 2= not set
					}
					if (k && k<9) u |= UPD_PREVIEW;
				}
				if (k == sk_Clear) {
					buf = editbuf;
					len = LARGEST_BUF;
					if (ck & ctrl_Alpha)	//invert
						i = 0xFF;
					else 					//clear
						i = 0x00;
					if (edit.fonttype == SFONT_T) { ++buf; --len; }
					for (;len;--len,++buf) *buf = (*buf & i) ^ i;
					u |= UPD_GRID;
				}
				if ((k == tk_Prev) || (k == tk_Next)) {
					//Commit to fontdata, then change context.
					//Updating grid only on context change.
					font_CommitEditBuffer();
					edit.context = CX_BROWSING;
					u |= UPD_GRID;
				}
			}
			if (edit.context & CX_BROWSING) {
				if ((k == tk_Prev) || (k == tk_Next)) {
					delta = 0;
					if (k == tk_Prev)	delta = -1;
					else				delta = 1;
					while (!(edit.codepoint += delta));	//repeat until nonzero.
					u |= UPD_SIDE;
				}
				if (ck & ctrl_2nd) {
					font_LoadEditBuffer();
					edit.context = CX_EDITING;
					while (GetScan());	//wait until keyrelease
					u |= UPD_GRID;
				}
			}
			
			
			/* ################### CONTEXTLESS MENUS ##################### */
			if (k == tk_File) {
				val = menu_DrawTabMenu(file_menutext,0);
				if (val == 2) {
					rval = menu_DrawFileMenu(help_menutext,FILEACT_OPEN);
					if (rval>=0) {
						font_NewFile();
						memcpy(&curfile,&filelist[rval],sizeof curfile);
						buf = (uint8_t *) curfile.fontstruct;
						memcpy(fontdata.codepoints,buf,256);
						numcodes = *fontdata.codepoints;
						*fontdata.codepoints = 0xFF;
						buf = buf + 256;
						memcpy(fontdata.lfont,buf,LFONT_SIZE*numcodes);
						buf += LFONT_SIZE*numcodes;
						memcpy(fontdata.sfont,buf,SFONT_SIZE*numcodes);
						curfile.fontstruct = &fontdata;
						font_LoadEditBuffer();
					}
					u = UPD_ALL;
				}
				u |= UPD_ALL;
			}
			if (k == tk_Edit) {
				val = menu_DrawTabMenu(edit_menutext,1);
				u |= UPD_ALL;
			}
			if (k == tk_Help) {
				val = menu_DrawTabMenu(help_menutext,4);
				u |= UPD_ALL;
			}
			/* DEBUGGING UNIT */
			if (k == sk_Math) {
				while (1) {
					rval = menu_DrawFileMenu(help_menutext,FILEACT_SAVEAS);
					if (rval<0) break;
					
					//Todo: Link this section into Save As.. then do something
					//intelligent for "save" such as autooverwriting but prompt
					//if the file was (somehow) archived.
					//Also figure out how to track changes to files (e.g. set
					//a change flag on each commit) and run a prompt if you try
					//to open a file, or create a new one.
					
					//Even further into the future, implement UX elements such
					//as bitmap shifting, small font switching/support,
					//undo/redos, and in general linking together all the menus
					//for a cohesive product. Or graying out the menu items that
					//don't have any linkages to ensure the user doesn't try
					//anything funny and maybe crash things.
					
					//Also, handle unsaved changes on quit or something like
					//that. Also tag each undo/redo state as whether or not
					//the most recent save was involved so that it can
					//intelligently tell that the file was reverted to an
					//unchanged state, to avoid unnecessary prompts.
					
					
					//REMEMBER THAT THIS CALL AND ALL CALLS LIKE IT OPERATE ON
					//"menufile" DATA STRUCTURE, NOT "curfile". YOU MUST COPY
					//TO menufile IF CHECKING THE WORKING COPY FOR SUCH THINGS
					//AS IF IN RAM OR ARCHIVE, OR IF DOING A FILE EXIST LOOKUP
					rval = font_SaveFile();	//-1 on cancel, 0 on saved, 1 on overwrite
					if (rval >= 0) break;
				}
				if (!(rval < 0)) {
					memcpy(&curfile,&menufile,10); //save type and name to current
				}
				u = UPD_ALL;
			}
			
			
		} else {
			hold = 0;	//Cleared for grid 2nd keyrepeat
		}
		
		edit.update = u;
		menu_Redraw();
	}
	gfx_End();
}


void centersidebar(char *s, uint8_t y) {
	gfx_PrintStringXY(s,(SIDEBAR_WIDTH-gfx_GetStringWidth(s))/2+SIDEBAR_LEFT,y);
}
void sidebarbracket(uint8_t w, uint8_t y,uint8_t offset) {
	int tx;
	tx = (SIDEBAR_WIDTH-w)/2+SIDEBAR_LEFT;
	gfx_SetColor(COLOR_ROSE);
	gfx_FillRectangle_NoClip(tx-2,y,w+4,4);
	gfx_SetColor(CLR_MAIN_SIDE);
	gfx_FillRectangle_NoClip(tx,y+offset,w,2);
}
/* Put other functions here */

char *editor_title = "it's going to be a font editor. rawrf.";
char *menu_fileopt[] = {"\0FILE",""};
char *menu_editopt[] = {"\0EDIT",""};
char *menu_prevopt[] = {"\0PREV","CODEPNT"};
char *menu_nextopt[] = {"\0NEXT","CODEPNT"};
char *menu_helpopt[] = {"\0HELP",""};
char **menutabs[] = {menu_fileopt,menu_editopt,menu_prevopt,menu_nextopt,menu_helpopt};

char *file_viewpro[] = {"\0PRGMS",""};
char *file_viewapv[] = {"\0APPVARS",""};
char *file_chgfile[] = {"\0BROWSE","FILES"};
char *file_chgname[] = {"\0INPUT","NAME"};
char *file_helpopt[] = {"\0HELP",""};

char **filetabs[] = {file_viewpro,file_viewapv,file_chgfile,file_chgname,file_helpopt};


void menu_Redraw(void) {
	int 	x,tx,bx,temp;
	uint8_t	y,ty,by,tmp,u,fwdth;
	uint8_t	i,j,k,gx,gy,gw,*ptr,*buf,w,h,lim;
	uint8_t bgcolor,fgcolor;
	char 	***ts,**as,*s;
	
	u = edit.update;
	/* --------------------------------------------------------------------- */
	if (u & UPD_TABS)	{
		gfx_SetColor(CLR_MAIN_BODY);
		gfx_FillRectangle(0,TAB_TOP,LCD_WIDTH,TAB_HEIGHT);
		#ifndef FASTERCOMPILE
		if (edit.context & (CX_FILEEDIT|CX_FILEVIEW))
			ts = filetabs;
		else
			ts = menutabs;
		x = 1;
		y = TAB_TOP;
		for (i=0;i<5;++i,x+=64) {
			as = ts[i];
			tmp = as[0][0];
			if (tmp & OBJ_ACTIVE) {
				bgcolor = CLR_TAB_ACTIVEBG;		fgcolor = CLR_TAB_ACTIVEFG;
			} else if (tmp & OBJ_BLOCKED) {
				bgcolor = CLR_TAB_BLOCKEDBG;	fgcolor = CLR_TAB_BLOCKEDFG;
			} else {
				bgcolor = CLR_TAB_BODYBG;		fgcolor = CLR_TAB_BODYFG;
			}
			gfx_SetColor(COLOR_BLACK);
			//Inner
			gfx_Rectangle_NoClip(x+1,y+1,TAB_WIDTH-2,TAB_HEIGHT-1);
			//Outer sides
			gfx_Rectangle_NoClip(x,y+1,TAB_WIDTH,TAB_HEIGHT-1);
			gfx_HorizLine_NoClip(x+1,y,TAB_WIDTH-2);
			gfx_SetColor(bgcolor);
			gfx_SetTextFGColor(fgcolor);
			gfx_FillRectangle_NoClip(x+2,y+2,TAB_WIDTH-4,TAB_HEIGHT-2);
			ty = y + 4;
			if (!as[1][0]) ty += 5;
			tx = x+(TAB_WIDTH-gfx_GetStringWidth(as[0]+1))/2;
			gfx_PrintStringXY(as[0]+1,tx,ty);
			if (!as[1][0]) continue;
			tx = x+(TAB_WIDTH-gfx_GetStringWidth(as[1]))/2;
			gfx_PrintStringXY(as[1],tx,ty+10);
		}
		#endif
		gfx_BlitRectangle(gfx_buffer,0,TAB_TOP,LCD_WIDTH,TAB_HEIGHT);
	}
	/* --------------------------------------------------------------------- */
	if (u & (UPD_SIDE|UPD_PREVIEW)) {
		/* Add conditionals to update only font preview */
		gfx_SetColor(CLR_MAIN_SIDE);
		gfx_SetTextFGColor(COLOR_BLACK);
		gfx_FillRectangle_NoClip(SIDEBAR_LEFT,SIDEBAR_TOP,SIDEBAR_WIDTH,SIDEBAR_HEIGHT);
		
		#ifndef FASTERCOMPILE
		buf = editbuf;
		w = edit.xlim;
		h = edit.ylim;
		if (edit.fonttype == SFONT_T) {
			s = "SMALL FONT";
			fwdth = *buf++;
		} else if (edit.fonttype == LFONT_T) {
			s = "LARGE FONT";
			fwdth = 12;
		} else {
			s = "FMT UNKNOWN";
			fwdth = 24;
		}
		y = SIDEBAR_TOP + 8;
							
		gfx_SetColor(CLR_MAIN_BODY);
		gfx_FillRectangle_NoClip(SIDEBAR_LEFT+8,y-2,SIDEBAR_WIDTH-(8+8),12);
		centersidebar(s,y);
		y += 2+8+2+8;
		
		gfx_SetTextXY(SIDEBAR_LEFT+4,y);
		gfx_PrintString("FILE: ");
		gfx_PrintString(curfile.name);
		y += 8+8;
		
		tmp = fontdata.codepoints[edit.codepoint];
		gfx_PrintStringXY("CHR: ",SIDEBAR_LEFT+4,y);
		if (tmp == 0xFF)	gfx_PrintString("---");
		else				gfx_PrintUInt(tmp+1,1);
		gfx_PrintString(" of ");
		gfx_PrintUInt(numcodes,1);
		y += 8+8;
		
		gfx_SetTextXY(SIDEBAR_LEFT+22,y);
		gfx_PrintUInt(edit.codepoint,3);
		gfx_PrintString(" [0x");
		PrintHexPair(edit.codepoint);
		gfx_PrintChar(']');
		x = (SIDEBAR_WIDTH-w)/2+SIDEBAR_LEFT;
		gfx_SetTextXY(x,y+10);
		gfx_SetColor(COLOR_WHITE);
		gfx_FillRectangle_NoClip(x,y+10,w,h);
		PrintChar(edit.codepoint,NULL,edit.fonttype+0x80);	//Force default
		y += 8+2+h+2;
		
		sidebarbracket(w,y,2);
		y += 6;
		
		by = y;
		//Point at which we would care about if updating only preview
		if (w>16)	lim = 3;
		else		lim = 5;
		x = (SIDEBAR_WIDTH - w*lim - 4*(lim-1))/2 + SIDEBAR_LEFT;
		gfx_SetColor(COLOR_WHITE);
		for (i = 0; i < lim; ++i, x += w+4) {
			j = (int)i - (lim-1)/2 + edit.codepoint;
			if (!j) continue;
			gfx_FillRectangle_NoClip(x,y,w,h);
			gfx_SetTextXY(x,y);
			if (fontdata.codepoints[j] == 0xFF)
				fgcolor = COLOR_ROSE;
			else
				fgcolor = COLOR_BLACK;
			gfx_SetTextFGColor(fgcolor);
			if ((j == edit.codepoint) && (edit.context == CX_EDITING))
				PrintChar(0,NULL,edit.fonttype);	//Use edit buffer
			else
				PrintChar(j,&fontdata,edit.fonttype);
		}
		y += h+2;
		
		sidebarbracket(w,y,0);
		y += 6;
		
		gfx_SetTextFGColor(COLOR_BLACK);
		gfx_PrintStringXY("CHR W/H: ",SIDEBAR_LEFT+4,y);
		gfx_PrintUInt(fwdth,2);
		gfx_PrintString(" x ");
		gfx_PrintUInt(h,2);
		y += 8+8;
		
		#endif
		
		if (u & UPD_PREVIEW)	//Preview sprite being edited (editing with grid)
			gfx_BlitRectangle(gfx_buffer,SIDEBAR_LEFT,by,SIDEBAR_WIDTH,h);
		else					//Refresh the whole sidebar (cpt mov't, etc)
			gfx_BlitRectangle(gfx_buffer,SIDEBAR_LEFT,SIDEBAR_TOP,SIDEBAR_WIDTH,SIDEBAR_HEIGHT);
	}
	/* --------------------------------------------------------------------- */
	if (u & UPD_GRID) {
		gfx_SetColor(CLR_MAIN_BODY);
		gfx_SetTextFGColor(COLOR_BLACK);
		gfx_FillRectangle(TITLE_LEFT,TITLE_HEIGHT,LCD_WIDTH-SIDEBAR_WIDTH,SIDEBAR_HEIGHT);
		
		#ifndef FASTERCOMPILE
		ptr = editbuf;
		if (edit.fonttype == SFONT_T) {
			fwdth = *ptr++;
			x = GRID_LEFT;
			y = GRID_TOP+12;
			gw = 12;	//Width of grid items
		} else if (edit.fonttype = LFONT_T) {
			fwdth = 12;
			x = GRID_LEFT+24;
			y = GRID_TOP;
			gw = 12;	//Width of grid items
		} else {
			fwdth = 12;
			x = GRID_LEFT;
			y = GRID_TOP;
			gw = 12;	//Width of grid items
		}
		//dbg_sprintf(dbgout,"Pointer: %X\n",ptr);
		for (gy=0;gy<edit.ylim;++gy,y+=gw) {
			for (gx=0;gx<edit.xlim;++gx,x+=gw) {
				if (edit.context & CX_EDITING) {
					if (edit.fonttype == SFONT_T) {
						if (gx>=fwdth) fgcolor = CLR_OUTOFBOUNDS;
						else {
							if (gx==8) ++ptr;
							if (*ptr & (0x80 >> (gx&7)))
								fgcolor = CLR_GRID_ON;
							else
								fgcolor = CLR_GRID_OFF;
						}
					} else if (edit.fonttype == LFONT_T) {
						if (gx==5) ++ptr;
						if (gx>=5)	tmp = gx-5;
						else		tmp = gx;
						if (*ptr & (0x80 >> tmp))
							fgcolor = CLR_GRID_ON;
						else
							fgcolor = CLR_GRID_OFF;
					} else {	//Reserved for future use
						fgcolor = CLR_GRID_SLEEP;
					}
				} else fgcolor = CLR_GRID_SLEEP;
				gfx_SetColor(fgcolor);
				gfx_FillRectangle_NoClip(x,y,gw-1,gw-1);
				if ((gx == edit.gridx) && (gy == edit.gridy)) {
					if (fgcolor != CLR_GRID_SLEEP) {
						gfx_SetColor(COLOR_MAGENTA);
						gfx_Rectangle_NoClip(x,y,gw-1,gw-1);
						if (gw>3) gfx_Rectangle_NoClip(x+1,y+1,gw-3,gw-3);
					}
				}
			}
			x -= gw*edit.xlim;
			++ptr;
		}
		#endif
		
		gfx_BlitRectangle(gfx_buffer,TITLE_LEFT,TITLE_HEIGHT,LCD_WIDTH-SIDEBAR_WIDTH,SIDEBAR_HEIGHT);
	}
	/* --------------------------------------------------------------------- */
	if (u & UPD_TOP ) {
		s = editor_title;
		gfx_SetColor(CLR_MAIN_TOP);
		gfx_SetTextFGColor(COLOR_WHITE);
		gfx_FillRectangle(TITLE_LEFT,TITLE_TOP,TITLE_WIDTH,TITLE_HEIGHT);
		gfx_PrintStringXY(s,(LCD_WIDTH-gfx_GetStringWidth(s))/2,2);
		/* Set other status indicators in top right. e.g. alpha/ins status */
		gfx_BlitRectangle(gfx_buffer,TITLE_LEFT,TITLE_TOP,TITLE_WIDTH,TITLE_HEIGHT);
	}
	
	edit.update = u & ~(UPD_TABS|UPD_SIDE|UPD_GRID|UPD_TOP|UPD_PREVIEW);
}

void font_NewFile(void) {
	uint8_t i;
	memset(fontdata.codepoints,0xFF,256);
	memset(fontdata.lfont,0,255*LFONT_SIZE);
	memset(fontdata.sfont,0,255*SFONT_SIZE);
	for (i=0;i<255;++i) fontdata.sfont[i][0] = 2;
	fonttype = LFONT_T;
	memset(editbuf,0,LARGEST_BUF);
	numcodes = 0;
	edit.update = UPD_ALL;
	edit.context = CX_EDITING;
	edit.fonttype = LFONT_T;
	edit.codepoint = 'A';
	edit.gridx = 0;
	edit.gridy = 0;
	edit.xlim = 12;
	edit.ylim = 14;
	strcpy(curfile.name,"UNTITLED");
	curfile.size = 0;
	curfile.filetype = 0x06;
	curfile.fontstruct = &fontdata;
	font_LoadEditBuffer();
}

void font_LoadEditBuffer(void) {
	memcpy(editbuf,GetCharLocation(edit.codepoint,&fontdata,edit.fonttype),LARGEST_BUF);
}
void font_LoadDefaultEditBuffer(void) {
	memcpy(editbuf,GetDefaultLocation(edit.codepoint,edit.fonttype),LARGEST_BUF);
}

uint8_t menu_GetMenuHeight(char **s) {
	uint8_t y,i;
	i = y = 0;
	while (*s[i]) {
		y += 12;
		++i;
	}
	return y;
}
uint8_t menu_GetMenuWidth(char **s) {
	uint8_t i;
	int x,xl;
	
	for (i=xl=0; *s[i]; ++i) {
		x = gfx_GetStringWidth(s[i]);
		if (x>xl) xl = x;
	}
	return xl;
}

void menu_DrawMenuBox(int x, uint8_t y, int w, uint8_t h) {
	gfx_SetColor(CLR_MENU_BORDER);
	gfx_Rectangle_NoClip(x,y,w,h);
	gfx_Rectangle_NoClip(x+1,y+1,w-2,h-2);
	gfx_SetColor(CLR_MENU_BODYBG);
	gfx_FillRectangle_NoClip(x+2,y+2,w-4,h-4);
	gfx_SetTextFGColor(CLR_MENU_BODYFG);
}

void menu_DrawNotice(char **sa) {
	int x,w;
	uint8_t y,ty,h,i;
	char *s;
	
	w = 2+2+4+4+menu_GetMenuWidth(sa);
	h = 2+2+menu_GetMenuHeight(sa);
	x = (LCD_WIDTH-w)/2;
	y = (LCD_HEIGHT-h)/2;
	ty = y+4;
	menu_DrawMenuBox(x,y,w,h);
	i = 0;
	do {
		s = sa[i];
		
		if (!*s) break;
		gfx_PrintStringXY(s,(LCD_WIDTH-gfx_GetStringWidth(s))/2,ty);
		ty += 12;
		++i;
	} while (1);
	gfx_BlitRectangle(gfx_buffer,x,y,w,h);
	while (!GetScan());	//wait until keyboard read
	while (GetScan());	//wait until release
}

uint8_t menu_DrawTabMenu(char **sa, uint8_t pos) {
	int x,tx,bx,w;
	uint8_t y,ty,by,h;
	uint8_t i,j,k,mpos,mprev,bgc,fgc;
	char *s;
	
	w = 2+2+4+4+menu_GetMenuWidth(sa);	//Menu borders and side margins
	//dbg_sprintf(dbgout,"Retrieved width in drawmenutab\n");
	h = 2+2+menu_GetMenuHeight(sa);					//Just menu borders. Margins built in
	if (pos<5) {
		x = pos*(LCD_WIDTH/5)+1;
		if (x+w > LCD_WIDTH) x = LCD_WIDTH-w;
		y = LCD_HEIGHT-TAB_HEIGHT-h;
	} else {
		x = (LCD_WIDTH-w)/2;
		y = (LCD_HEIGHT-h)/2;
	}
	menu_DrawMenuBox(x,y,w,h);
	
	mprev = mpos = 0;
	k = 0x3F;	//unused code to prime the menu display
	
	//dbg_sprintf(dbgout,"Entering loop, vars %i %i %i %i\n",x,y,w,h);
	while (1) {
		//dbg_sprintf(dbgout,"Registered keypress %i\n",k);
		if (k) {
			if ((k & ctrl_2nd) && !(*sa[mpos] & OBJ_BLOCKED)) {
				i = mpos+1;
				break;
			}
			k &= 0x3F;	//Remove 2nd/alpha
			if ((k == sk_Mode) || ((k >=sk_Graph) && (k <= sk_Yequ))) {
				i = 0;
				break;
			}
			if ((k == sk_Up) && (mpos > 0))		--mpos;
			if ((k == sk_Down) && *sa[mpos+1])	++mpos;
			//dbg_sprintf(dbgout,"Entering for loop with %i",*sa[0]);
			for (i = 0, ty = y+2; *sa[i]; ++i, ty += 12) {
				s = sa[i]+1;
				//dbg_sprintf(dbgout,"Iter %i, Str %s, ypos %i\n",i,s,ty);
				if ((i == mprev) || (i == mpos)) {
					if (i == mpos)	bgc = CLR_MENU_SLCTBG;
					else			bgc = CLR_MENU_BODYBG;
					gfx_SetColor(bgc);
					gfx_FillRectangle_NoClip(x+2,ty,w-4,12);
				}
				if (*sa[i] & OBJ_BLOCKED)	fgc = CLR_MENU_BLOKFG;
				else if (i == mpos)			fgc = CLR_MENU_SLCTFG;
				else						fgc = CLR_MENU_BODYFG;
				gfx_SetTextFGColor(fgc);
				gfx_PrintStringXY(s,x+2+4,ty+2);
			}
			mprev = mpos;
			gfx_BlitRectangle(gfx_buffer,x,y,w,h);
		}
		k = GetScan();
	}
	while (GetScan());	//Wait until keyboard release before continuing.
	return i;
}

void font_CommitEditBuffer(void) {
	uint8_t *ssrc,*lsrc,*buf,codepoint,fcode;
	size_t  copylen;
	
	codepoint = edit.codepoint;
	if (!codepoint) return;
	fcode = fontdata.codepoints[codepoint];
	
	if (fcode == 0xFF) {
		fcode = numcodes;
		++numcodes;
		fontdata.codepoints[codepoint] = fcode;
		/* --- */
		ssrc = fontdata.sfont[fcode];
		memmove(fontdata.sfont[fcode+1],ssrc,(254-fcode)*SFONT_SIZE);
		memset(ssrc,0,SFONT_SIZE);
		*ssrc = 16;		//default width of 16
		/* --- */
		lsrc = fontdata.lfont[fcode];
		memmove(fontdata.lfont[fcode+1],lsrc,(254-fcode)*LFONT_SIZE);
		memset(lsrc,0,LFONT_SIZE);
	}
	if (edit.fonttype == SFONT_T) {
		copylen = SFONT_SIZE;
		buf = fontdata.sfont[fcode];
	} else if (edit.fonttype == LFONT_T) {
		copylen = LFONT_SIZE;
		buf = fontdata.lfont[fcode];
	} else return;
	memcpy(buf,editbuf,copylen);
	return;
}

int	menu_DrawFileMenu(char **sa, uint8_t id) {
	int 	x,tx,lx,rx,bx,w,tw,lw,rw,bw,tempx,temp,rval;
	uint8_t	y,ty,ly,ry,by,h,th,lh,rh,bh,tempy;
	uint8_t i,j,k,u,mpos,mtop,mmax,mprev,tidx,bgc,fgc,*ptr,*buf;
	uint8_t typeid,newtype,blinktimer,alphastate,letterpos;
	char *s,c,numbuf[5];
	
	if (id) u = CX_FILEEDIT;
	else	u = CX_FILEVIEW;
	edit.oldcx = edit.context;
	edit.context = u;
	edit.update = UPD_TABS;
	if (id)	filetabs[3][0][0] = OBJ_INACTIVE;
	else	filetabs[3][0][0] = OBJ_BLOCKED;
	filetabs[4][0][0] = OBJ_BLOCKED;	//No help for you. For now.
	
	menu_Redraw();
	//our filetypes are never zero. can copy with rest of name string
	strcpy((char*)&menufile.filetype,(char*)&curfile.filetype);
	alphastate = CURSOR_ALPHA;
	letterpos = blinktimer = 0;
	
	
	w = 214;
	h = (id) ? 176: 156;
	x = (LCD_WIDTH-w)/2;
	y = (LCD_HEIGHT-h)/2;
	
	gfx_SetColor(CLR_MENU_BORDER);
	gfx_SetTextFGColor(COLOR_BLACK);
	gfx_FillRectangle_NoClip(x,y,w,h);	//Main box.
	gfx_BlitRectangle(gfx_buffer,x,y,w,h);
	
	tx = x+2;		ty = y+2;		tw = w-4;	th = 12;
	lx = tx;		ly = ty+16;		lw = 120;	lh = 136;
	rx = tx+lw+4;	ry = ly;		rw = 86;	rh = lh;
	bx = tx;		by = ry+lh+4;	bw = tw;	bh = 16;
	
	rval = 0;
	typeid = menufile.filetype;
	menufile.filetype = 0;  //prime for first run
	mmax = mtop = mpos = i = 0;
	k = 0x3F;
	
	while (1) {
		if (k == sk_Mode) {
			while (GetScan());
			if (id && (u & CX_FILEEDIT)) {
				u = CX_FILEVIEW;
				k = 0x3F;
				continue;
			}
			rval = -1;
			break;
		}
		if (k & ctrl_2nd) {
			while (GetScan());
			if (id && (u & CX_FILEVIEW)) {
				u = CX_FILEEDIT;
				k = 0x3F;
				continue;
			}
			if (id) {
				rval = 0;
				break;
			} else {
				rval = mpos;
				break;
			}
		}
		if ((k & ctrl_Alpha) && (u & CX_FILEEDIT)) {
			while (GetScan());	//wait until key release
			++alphastate;
			if (alphastate > CURSOR_NUMBER) alphastate = CURSOR_ALPHA;
		}
		
		if (k) {
			c = KeyToChar(k,alphastate);
			
			//dbg_sprintf(dbgout,"Returned key, cval, char: %i, %i, %c\n",k,c,c);
			if (((alphastate == CURSOR_SOLID) || (alphastate == CURSOR_NUMBER)) &&
				!letterpos && (typeid == 0x06)) {
				c = 0;	//Disallow numbers as first character of progname.
				//dbg_sprintf(dbgout,"Failed illegal first char prgm name test\n");
			}
			if ((c > 0x60) && (typeid == 0x06)) {
				c = 0;	//Disallow lowercase letters as first character of programs.
				//dbg_sprintf(dbgout,"Failed illegal lowercase letters test.\n");
			}
			if ((u & CX_FILEEDIT) && c) {
				//Overwrite mode. Maybe support logic for insert later?
				//dbg_sprintf(dbgout,"Outputting letter %c\n",c);
				menufile.name[letterpos] = c;
				if (letterpos < 7) ++letterpos;
			}
		}
		
		if (k == sk_Yequ) {
			u = CX_FILEVIEW;
			k = sk_Left;
		}
		if (k == sk_Window) {
			u = CX_FILEVIEW;
			k = sk_Right;
		}
		if (k == sk_Zoom) {
			u = CX_FILEVIEW;
			k = 0x3F;
		}
		if ((k == sk_Trace) && id) {
			u = CX_FILEEDIT;
			k = 0x3F;
		}
		if (k == sk_Graph) {
			/* File help menu */
			k = 0x3F;
		}
		
		
		gfx_SetColor(CLR_MENU_BODYBG);
		if ((k == sk_Del) && (u & CX_FILEEDIT)) {
			//It'll bring the zero-terminator with it
			memmove(&menufile.name[letterpos],&menufile.name[letterpos+1],8-letterpos);
			if (!menufile.name[letterpos] && letterpos) {
				--letterpos;
			}
		}
		
		if ((k == sk_Left) || (k == sk_Right) || (k == 0x3F)) {
			if ((u & CX_FILEVIEW) || (k == 0x3F)) {
				/* Changing topbar */
				if ((k == sk_Right) && (typeid != 0x15)) typeid = 0x15;				
				if ((k == sk_Left)  && (typeid != 0x06)) {
					typeid = 0x06;
					if (id) {
						if (id && (menufile.name[0] < 0x3A)) menufile.name[0] = 'A';
						for (i=0;i<8;++i) {
							if (menufile.name[i] > 0x60) menufile.name[i] -= 0x20;
						}
					}
				}
				
				gfx_FillRectangle(tx,ty,tw,th);
				if (id)	s = "Save as...";
				else	s = "Open";
				gfx_PrintStringXY(s,tx+4,ty+2);
				if (typeid==0x06) 	s = "Programs \x10";
				else				s = "\x11 Appvars";
				gfx_PrintStringXY(s,tx+tw-gfx_GetStringWidth(s)-4,ty+2);
				gfx_BlitRectangle(gfx_buffer,tx,ty,tw,th);
				if (typeid != menufile.filetype) {
					filelist_len = mmax = PopulateFileList(typeid);
					mpos = 0;
					k = 0x3F;	//Update all screens
					menufile.filetype = typeid;
				}
			}
			if (u & CX_FILEEDIT) {
				if ((k == sk_Left) && (letterpos>0)) --letterpos;
				if ((k == sk_Right) && (menufile.name[letterpos+1])) ++letterpos;
				if ( k != 0x3F)	blinktimer = 0;
			}
		}
		if ((k == sk_Up) || (k == sk_Down) || (k == 0x3F)) {
			/* Draw central area */
			if ((k == sk_Up)  && (mpos > 0   ))	--mpos;
			if ((k == sk_Down)&& (mpos+1<mmax))	++mpos;
			if ((mmax-mtop) < 0) mtop = mpos;
			if ((mmax-mtop) > 7) mtop = mpos-7;
			gfx_FillRectangle(lx,ly,lw,lh);
			gfx_FillRectangle(rx,ry,rw,rh);
			if (mmax) {
				tempy = ly+2;
				//dbg_sprintf(dbgout,"Starting loop with mmax: %i\n",mmax);
				for (i = mtop; (i < (mtop+7)) && (i < mmax); ++i, tempy += 16) {
					//dbg_sprintf(dbgout,"i=%i, mtop=%i,mpos=%i\n",i,mtop,mpos);
					if (i == mpos) {
						gfx_SetColor(COLOR_BLACK|COLOR_LIGHTER);
						gfx_FillRectangle_NoClip(lx,tempy,lw,16);
						gfx_FillRectangle_NoClip(rx,tempy,rw,16);
						fgc = COLOR_WHITE;
					} else {
						fgc = COLOR_BLACK;
					}
					gfx_SetTextFGColor(fgc);
					gfx_SetTextXY(lx+4,tempy);
					for(j=0;c = filelist[i].name[j];++j) {
						PrintLargeChar(c,filelist[i].fontstruct);
					}
					gfx_SetTextXY(rx+4,tempy);
					temp = filelist[i].size;
					for(j=5;j;--j) {
						numbuf[j-1] = (temp%10)+'0';
						temp /= 10;
					}
					for (j=0;j<5;++j) {
						PrintLargeChar(numbuf[j],filelist[i].fontstruct);
					}
				}
				gfx_SetColor(CLR_MENU_BODYBG);
				gfx_SetTextFGColor(COLOR_BLACK);
			}
			gfx_BlitRectangle(gfx_buffer,lx,ly,lw,lh);
			gfx_BlitRectangle(gfx_buffer,rx,ry,rw,rh);
		}
		if (id) {
			/* Changing bottom bar */
			gfx_FillRectangle(bx,by,bw,bh);
			gfx_PrintStringXY("File name:",bx+4,by+4);
			if (u & CX_FILEVIEW)
				bgc = COLOR_LIGHTGRAY;
			else
				bgc = COLOR_WHITE;
			gfx_SetColor(bgc);
			gfx_SetTextFGColor(COLOR_BLACK);
			gfx_FillRectangle_NoClip(bx+72,by+2,132,12);
			gfx_SetColor(CLR_MENU_BODYBG);
			gfx_SetTextXY(bx+80,by+4);
			for (i=0;i<8;++i) {
				c = menufile.name[i];
				if ((u & CX_FILEEDIT) && ( i == letterpos)) {
					if (blinktimer < CURSOR_TIMEON) {
						c = alphastate;
					}
				}
				if (!c) break;
				gfx_PrintChar(c);
			}
			gfx_BlitRectangle(gfx_buffer,bx,by,bw,bh);
		}
		
		k = GetScan();
		++blinktimer;
		if (blinktimer > CURSOR_TIMEMAX) { blinktimer = 0;}
	}
	
	while (GetScan());
	edit.context = edit.oldcx;
	return rval;
}


//-1 on cancel, 0 on saved, 1 on overwrite
int	font_SaveFile(void) {
	uint8_t i;
	
	if (!LookupMenufile()) {
		i = menu_DrawTabMenu(overwrite_menu,255);
		if (i == 2) {
			SaveMenufile();
			menu_DrawNotice(notice_overwrite);
			return 1;
		} else {
			return -1;
		}
	} else {
		SaveMenufile();
		menu_DrawNotice(notice_saved);
		return 0;
	}
}











