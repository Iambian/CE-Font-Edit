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


/* Put your function prototypes here */
void	menu_Redraw(void);
void 	font_NewFile(void);
void	font_LoadEditBuffer(void);
void 	font_LoadDefaultEditBuffer(void);
uint8_t	menu_GetMenuHeight(char **s);
uint8_t	menu_GetMenuWidth(char **s);
void	menu_DrawMenuBox(int x, uint8_t y, int w, uint8_t h);
uint8_t	menu_DrawTabMenu(char **sa, uint8_t pos);
void	font_CommitEditBuffer(void);







/* Put all your globals here */


uint8_t fonttype;
uint8_t editbuf[LARGEST_BUF];
uint8_t numcodes;
struct fontdata_st fontdata;  //fontdata.[codepoints,lfont,sfont]
struct editor_st edit;
int filelist_len;
struct filelist_st filelist; 


char *file_menutext[] = {"\1New","\1Open...","\1Save","\1Save As...","\3-","\1Preview","\1Export","\1Import","\3-","\1Exit","\0"};
char *edit_menutext[] = {"\1Undo","\1Redo","\1Copy","\1Paste","\1Use TI-OS Glyph","\1Change Font Size","\1Delete Codepoint","\1Clear Grid","\0"};
char *help_menutext[] = {"\1Using the editor","\1File operations","\1Edit operations","\1Hotkeys 1","\1Hotkeys 2","\1Hotkeys 3","\1About the rawrfs","\0"};











void main(void) {
	uint8_t k,ck,u,hold,gx,gy,fid,val,i;
	int delta;
    /* Fill in the body of the main function here */
	gfx_Begin();
	SetupPalette();
	gfx_SetTransparentColor(COLOR_TRANS);
	gfx_SetTextTransparentColor(COLOR_TRANS);
	gfx_SetTextBGColor(COLOR_TRANS);
	gfx_SetDrawBuffer();
	/* ---------------------- */
	gfx_FillScreen(COLOR_BLUE);
	font_NewFile();
	
	
	hold = k = 0;
	while (1) {
		k = GetScan();	ck = k & 0xC0;	k &= 0x3F;
		u = edit.update;
		
		if (k == sk_Mode) break;
		
		if (k|ck) {
			/* ################ HANDLE EDITING CONTEXT ################### */
			if (edit.context & CX_EDITING) {
				if ((k == sk_Up)    && (edit.gridy > 0)) --edit.gridy;
				if ((k == sk_Left)  && (edit.gridx > 0)) --edit.gridx;
				if ((k == sk_Down)  && (edit.gridy+1 < edit.ylim)) ++edit.gridy;
				if ((k == sk_Right) && (edit.gridx+1 < edit.xlim)) ++edit.gridx;
				if (k<9) u |= UPD_GRID;	//Shortcut for "any arrow key"
				
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
					u |= UPD_GRID | UPD_PREVIEW;
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
				}
				u |= UPD_SIDE;
			}
			
			
			/* ################### CONTEXTLESS MENUS ##################### */
			if (k == tk_File) {
				val = menu_DrawTabMenu(file_menutext,0);
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
		gfx_PrintString("UNTITLED");	//DEBUG: POINT LATER TO 8CH EDIT BUFFER
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

uint8_t menu_DrawTabMenu(char **sa, uint8_t pos) {
	int x,tx,bx,w;
	uint8_t y,ty,by,h;
	uint8_t i,j,k,mpos,mprev,bgc,fgc;
	char *s;
	
	w = 2+2+4+4+menu_GetMenuWidth(sa);	//Menu borders and side margins
	//dbg_sprintf(dbgout,"Retrieved width in drawmenutab\n");
	h = 2+2+menu_GetMenuHeight(sa);					//Just menu borders. Margins built in
	x = pos*(LCD_WIDTH/5)+1;
	if (x+w > LCD_WIDTH) x = LCD_WIDTH-w;
	y = LCD_HEIGHT-TAB_HEIGHT-h;
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














