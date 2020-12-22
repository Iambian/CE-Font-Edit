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
#include "lfststub.h"

/* Put your function prototypes here */
void	menu_Redraw(void);
void 	font_NewFile(void);
void	font_LoadEditBuffer(void);
void 	font_LoadDefaultEditBuffer(void);
uint8_t	menu_GetMenuHeight(char **s);
int		menu_GetMenuWidth(char **s);
void	menu_DrawMenuBox(int x, uint8_t y, int w, uint8_t h);
void	menu_DrawNotice(char **sa);
uint8_t	menu_DrawTabMenu(char **sa, uint8_t pos);
void	font_CommitEditBuffer(void);
void	menu_FileNameInputDevice(char *filenamebuf, uint8_t key, int x, int y);
int		menu_DrawFileMenu(char **sa, uint8_t id);
int		font_SaveFile(void);
void	font_QuickSave(void);
void	menu_PreviewFont(void);
void	font_ModifyEditBuffer(uint8_t modify_how);
void	font_EditbufToSprite(void);
void	font_SpriteToEditbuf(void);

/* Put all your globals here */


uint8_t fonttype;
uint8_t editbuf[LARGEST_BUF];
uint8_t copyid;
uint8_t copybuf[LARGEST_BUF];
uint8_t numcodes;
struct fontdata_st fontdata;  //fontdata.[codepoints,lfont,sfont]
struct editor_st edit;
int filelist_len;
struct filelist_st filelist[255]; 
struct filelist_st curfile;
struct filelist_st menufile;
char namebuf[10];
uint8_t tempsprite[2+(24*48)];


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

char *file_menutext[] = {"\1New","\1Open...","\1Save","\1Save As...","\3-","\1Preview","\3Export","\3Import","\3-","\1Exit","\0"};
char *edit_menutext[] = {"\3Undo","\3Redo","\1Copy","\1Paste","\1Use TI-OS Glyph","\1Change Font Size","\3Delete Codepoint","\1Clear Grid","\0"};
char *help_menutext[] = {"\1Using the editor","\3File operations","\3Edit operations","\1Hotkeys 1","\3Hotkeys 2","\3Hotkeys 3","\1About the rawrfs","\0"};

char *notice_error[]		= {"ERR: Attempted to display","a textbox too wide","for the display.","git gud scrub","\0"};
char *notice_saved[]		= {"New file has","been saved!","\0"};
char *notice_overwrite[]	= {"Old file has","been overwritten!","\0"};
char *notice_noempties[]	= {"You are not allowed to","save an empty file."," ","You must first commit editor changes","by pushing [ENTER] on the editor","or by changing codepoints.","\0"};
char *notice_helpusing[]	= {	"Use the top row keys to access menus in",
								"the tabs listed at the screen bottom.",
								"PREV CODEPNT and NEXT CODEPNT tabs",
								"commits changes to local store and",
								"deactivates the editor. Push [2nd]",
								"to activate the pixel editor. Save",
								"and open files in the FILE menu.",
								"When hotkeys include ALPHA or 2ND",
								"these keys must be held while pushing",
								"the other listed key.","\0"};
char *notice_helpfileops[]	= {"","","","","","","","","","","","","\0"};
char *notice_helpeditops[]	= {"","","","","","","","","","","","","\0"};
char *notice_helphotkey1[]	= {	"[Clear] = Clears the pixel editor",
								"[Alpha+Clear] = Inverts the pixels",
								"[2nd+Mode] = Quick exit while in editor",
								"[Enter] = Save editor changes to local",
								"[2nd+arrows] = Drag the edit tool in editor",
								"[+] = Increase width of smallfont character",
								"[-] = Decrease width of smallfont character",
								"More will be available eventually.",
								"","","","","","","\0"};
char *notice_helphotkey2[]	= {"","","","","","","","","","","","","\0"};
char *notice_helphotkey3[]	= {"","","","","","","","","","","","","\0"};
char *notice_helpabout[]	= {"Copyright (C) 2020   [Formal dragon noises]"," ","Something about betaware and","being a barely usable piece of","junk. Please test for me. I'll be grateful."," ","Signed,"," ","rawrs","\0"};
char *overwrite_menu[]		= {"\1Do not save","\1Overwrite existing file","\0"};
char *unsaved_menu[]		= {"\1Go back and save changes","\1Discard changes, make new file","\0"};
char *unsavedquit_menu[]	= {"\1Go back and save changes","\1Discard changes, quit the editor","\0"};
char *unsavedopen_menu[]	= {"\1Go back and save changes","\1Overwrite session with another file","\0"};

char *testing_string[] = {
	"the quick brown fox","jumped over the slow","lazy dog",
	"THE QUICK BROWN FOX","JUMPED OVER THE SLOW","LAZY DOG"
};

int resosize;
int stalsize;
int lfstsize;

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
	lfstsize = sizeof lfststub;
	
	gfx_FillScreen(COLOR_BLUE);
	font_NewFile();
	
	
	hold = k = 0;
	while (1) {
		k 	= GetScan();
		ck 	= k & 0xC0;
		k  &= 0x3F;
		u = edit.update;
		
		if ((k == sk_Mode) && (ck & ctrl_2nd)) break;
		
		if (k|ck) {
			/* ################ HANDLE EDITING CONTEXT ################### */
			//dbg_sprintf(dbgout,"keyboard value: %i\n",k);
			if (edit.context & CX_EDITING) {
				if (ck & ctrl_Alpha) {
					/* TODO: Alpha+arrow to shift character around bitmap */
					
				} else {
					if ((k == sk_Up)    && (edit.gridy > 0)) --edit.gridy;
					if ((k == sk_Left)  && (edit.gridx > 0)) --edit.gridx;
					if ((k == sk_Down)  && (edit.gridy+1 < edit.ylim)) ++edit.gridy;
					if  (k == sk_Right) {
						//if (edit.fonttype == SFONT_T)
						//	i = editbuf[0];
						//else
							i = edit.xlim;
						if (edit.gridx+1 < i) ++edit.gridx;
					}
				}
				if (edit.fonttype == SFONT_T) {
					if ((k == sk_Add) && (editbuf[0]<16)) {
						font_EditbufToSprite();
						++editbuf[0];
						font_SpriteToEditbuf();
						u |= UPD_GRID | UPD_SIDE;
					}
					if ((k == sk_Sub) && (editbuf[0]>1)) {
						font_EditbufToSprite();
						--editbuf[0];
						font_SpriteToEditbuf();
						u |= UPD_GRID | UPD_SIDE;
					}
				}
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
						u |= UPD_GRID;
					}
					if (k && k<9) u |= UPD_PREVIEW;
				}
				if (k == sk_Clear) {
					if (ck & ctrl_Alpha)
						i = BUFMOD_INVERT;
					else
						i = BUFMOD_CLEAR;
					font_ModifyEditBuffer(i);
					u |= UPD_GRID;
				}
				if ((k == tk_Prev) || (k == tk_Next)) {
					//Commit to fontdata, then change context.
					//Updating grid only on context change.
					font_CommitEditBuffer();
					menutabs[1][0][0] = OBJ_BLOCKED; //Disable edit in non-eidt mode
					edit.context = CX_BROWSING;
					u |= UPD_GRID|UPD_TABS;
					/* TODO: Update title bar to indicate file has changed */
				}
				if (k == sk_Enter) {
					font_CommitEditBuffer();
					/* TODO: Update title bar to indicate file has changed */
				}
				if (k == tk_Edit) { /* ~~~~~~~~~~~ EDIT MENU ~~~~~~~~~~~~~~~~  */
					if (edit.fonttype != copyid)
						edit_menutext[3][0] = OBJ_BLOCKED;
					else
						edit_menutext[3][0] = OBJ_ACTIVE;
					val = menu_DrawTabMenu(edit_menutext,1);
					if (val == 1) {
						/* Undo */
					}
					if (val == 2) {
						/* Redo */
					}
					if (val == 3) {
						/* Copy */
						copyid = edit.fonttype;
						memcpy(copybuf,editbuf,LARGEST_BUF);
					}
					if ((val == 4) && (copyid == edit.fonttype)) {
						/* Paste */
						memcpy(editbuf,copybuf,LARGEST_BUF);
					}
					if (val == 5) {
						/* Use TI-OS Glyph */
						font_LoadDefaultEditBuffer();
					}
					if (val == 6) {
						/* Change font size */
						font_CommitEditBuffer();
						edit.gridx = 0;
						edit.gridy = 0;
						if (edit.fonttype == LFONT_T) {
							edit.fonttype = SFONT_T;
							edit.xlim = 16;
							edit.ylim = 12;
						}
						else if (edit.fonttype == SFONT_T) {
							edit.fonttype = LFONT_T;
							edit.xlim = 12;
							edit.ylim = 14;
						}
						else;
						font_LoadEditBuffer();
					}
					if (val == 7) {
						/* Delete codepoint */
					}
					if (val == 8) {
						/* Clear grid */
						font_ModifyEditBuffer(BUFMOD_CLEAR);
					}
					u = UPD_ALL;
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
					menutabs[1][0][0] = OBJ_INACTIVE;
					edit.context = CX_EDITING;
					while (GetScan());	//wait until keyrelease
					u |= UPD_GRID|UPD_TABS;
				}
			}
			
			
			/* ################### CONTEXTLESS MENUS ##################### */
			if (k == tk_File) { /* ~~~~~~~~~~~ FILE MENU ~~~~~~~~~~~~~~~~  */
				val = menu_DrawTabMenu(file_menutext,0);
				if (val == 1) {
					rval = 2;	//If hasn't changed, make new file anyway.
					if (edit.haschanged) {
						rval = menu_DrawTabMenu(unsaved_menu,255);
					}
					if (rval == 2) font_NewFile();
				}
				
				if (val == 2) {
					rval = 2;	//If hasn't changed, open new file anyway.
					if (edit.haschanged) {
						rval = menu_DrawTabMenu(unsavedopen_menu,255);
					}
					if (rval == 2) {
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
							edit.verifiedfile = 1;
						}
					}
				}
				
				if (val == 3) {
					if (!edit.verifiedfile) {
						val = 4; //do "Save As..." instead
					} else {
						font_QuickSave();
					}
				}
				
				if (val == 4) {
					if (!numcodes) {
						menu_DrawNotice(notice_noempties);
					} else {
						while (1) {
							rval = menu_DrawFileMenu(help_menutext,FILEACT_SAVEAS);
							if (rval<0) break;
							rval = font_SaveFile();	//-1 on cancel, 0 on saved, 1 on overwrite
							if (rval >= 0) break;
						}
						if (!(rval < 0)) {
							memcpy(&curfile,&menufile,10); //save type and name to current
						}
					}
				}
				if (val == 6) {
					menu_PreviewFont();
				}
				if (val == 7) {
					/* Export */
					
				}
				if (val == 8) {
					/* Import */
					
				}
				if (val == 10) {
					rval = 2;	//If hasn't changed, quit anyway.
					if (edit.haschanged) {
						rval = menu_DrawTabMenu(unsavedquit_menu,255);
					}
					if (rval == 2) break;
				}
				u |= UPD_ALL;
			}
			if (k == tk_Help) { /* ~~~~~~~~~~~ HELP MENU ~~~~~~~~~~~~~~~~  */
				val = menu_DrawTabMenu(help_menutext,4);
				if (val == 1) {
					/* Using */
					menu_DrawNotice(notice_helpusing);
				}
				if (val == 2) {
					/* File ops */
				}
				if (val == 3) {
					/* Edit ops */
				}
				if (val == 4) {
					/* Hotkeys 1 */
					menu_DrawNotice(notice_helphotkey1);
				}
				if (val == 5) {
					/* Hotkeys 2 */
				}
				if (val == 6) {
					/* Hotkeys 3 */
				}
				if (val == 7) {
					/* About */
					menu_DrawNotice(notice_helpabout);
				}
				u |= UPD_ALL;
			}
			/* DEBUGGING UNIT */
			if (k == sk_Math) {
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
		
		/*
		font_EditbufToSprite();
		gfx_Sprite_NoClip((gfx_sprite_t*)tempsprite,SIDEBAR_LEFT+4,y);
		y += 16;
		*/
		
		
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
	///*
	edit.fonttype = LFONT_T;
	edit.codepoint = 'A';
	edit.gridx = 0;
	edit.gridy = 0;
	edit.xlim = 12;
	edit.ylim = 14;
	//*/
	/*
	edit.fonttype = SFONT_T;
	edit.codepoint = 'A';
	edit.gridx = 0;
	edit.gridy = 0;
	edit.xlim = 16;
	edit.ylim = 12;
	*/
	edit.haschanged = 0;
	edit.verifiedfile = 0;
	strcpy(curfile.name,"UNTITLED");
	curfile.size = 0;
	curfile.filetype = 0x06;
	curfile.fontstruct = &fontdata;
	font_LoadEditBuffer();
	copyid = LFONT_T;
	memset(copybuf,0,LARGEST_BUF);
	menutabs[1][0][0] = OBJ_INACTIVE;

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
int menu_GetMenuWidth(char **s) {
	uint8_t i;
	int x,xl;
	
	for (i=xl=0; *s[i]; ++i) {
		x = gfx_GetStringWidth(s[i]);
		//dbg_sprintf(dbgout,"Witdth iter %i, width %i\n",i,x);
		if (x>xl) xl = x;
	}
	//dbg_sprintf(dbgout,"Returning with width %i\n",xl);
	return xl;
}

void menu_DrawMenuBox(int x, uint8_t y, int w, uint8_t h) {
	//dbg_sprintf(dbgout,"Writing width %i\n",w);
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
	if (w>320) { sa = notice_error; w = 319;}
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
	edit.haschanged = 1;
	
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

void menu_FileNameInputDevice(char *filenamebuf, uint8_t key, int x, int y) {
	
	
	
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
				if (!menufile.name[0]) continue; //Did we just try to save w/o a name?
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
			edit.haschanged = 0;
			edit.verifiedfile = 1;
			return 1;
		} else {
			return -1;
		}
	} else {
		SaveMenufile();
		menu_DrawNotice(notice_saved);
		edit.haschanged = 0;
		edit.verifiedfile = 1;
		return 0;
	}
}

//Takes curfile and saves over whatever without confirmation. Usually only done
//on initial load verification.
void font_QuickSave(void) {
	menufile = curfile;
	SaveMenufile();
	edit.haschanged = 0;
}

void previewfont_drawglyph(char c, int x,uint8_t y, uint8_t fontid) {
	uint8_t color;
	
	if (!c) return;
	gfx_SetTextXY(x,y);
	if (fontdata.codepoints[(uint8_t)c] == 0xFF) {
		color = COLOR_ROSE;
	} else {
		color = COLOR_BLACK;
	}
	gfx_SetTextFGColor(color);
	PrintChar(c,&fontdata,fontid);
}

void menu_PreviewFont(void) {
	int x,tx,temp;
	uint8_t y,ty,tmp;
	uint8_t a,b,i,j,k,mode,fontid;
	char *top,*s,c;
	
	mode = 0;
	k = 0x3F;
	while (1) {
		
		if (k == sk_Mode) break;
		if ((k == sk_Left) && (mode > 0)) --mode;
		if ((k == sk_Right)&& (mode < 2)) ++mode;
		
		
		
		if (k) {
			gfx_FillScreen(CLR_MENU_BODYBG);
			gfx_PrintStringXY("Previewing: ",8,8);
			gfx_PrintString(curfile.name);
			if (mode == 0) {
/*	Draw area is (14w by 18h, though fonts are 12w by 14h
	22 characters fit on a line. 12 lines required to cover full range.
	Line height at 18 requires 216h. Acceptable results with LH=16 req 192.
	Starting Y coord can be either 24 or 48, respectively.
	Starting X coord at 6. This can be calculated.
*/
				top = "Large Font \x10";
				c = 1;
				y = 48;
				for (i = 0; i<12 ; ++i, y += 16) {
					for (j = 0, x = 0; (j<22) && c; ++j, x += 14, ++c) {
						previewfont_drawglyph(c,x,y,LFONT_T);
					}
				}
			} else if (mode == 1) {
				top = "\x11 Small Font \x10";
/*	Small font chars up to 16w, 12h. Draw area 16w 14h
	Start X is 0, Start y is 48. 20 characters per line, 13 lines
*/
				c = 1;
				y = 48;
				for (i = 0; i<13 ; ++i, y += 14) {
					for (j = 0, x = 6; (j<20) && c; ++j, x += 16, ++c) {
						previewfont_drawglyph(c,x,y,SFONT_T);
					}
				}
			} else {
				top = "\x11 Tests";
				
				for (i = 0, y = 48; i<2; ++i) {
					for (j = 0; j < 6; ++j, y += 16) {
						s = testing_string[j];
						for (a = 0, x = 0; s[a]; ++a) {
							previewfont_drawglyph(s[a],x,y,i);
							if (i)	x+=14;
							else	x+= *(GetSmallCharWidth(s[a],&fontdata));
						}
						gfx_SetColor(COLOR_GRAY);
						gfx_HorizLine(0,y+(12+2*i),320);
					}
				}
				
			}
			gfx_SetTextFGColor(COLOR_BLACK);
			gfx_PrintStringXY(top,(LCD_WIDTH-gfx_GetStringWidth(top)-8),8);
			gfx_PrintStringXY("Press [MODE] to go back",8,18);
			gfx_BlitBuffer();
		}
		k = GetScan();
	}
	
}

void font_ModifyEditBuffer(uint8_t modify_how) {
	size_t	len;
	uint8_t i,*buf;
	
	buf = editbuf;
	len = LARGEST_BUF;
	if ((modify_how == BUFMOD_CLEAR) || (modify_how == BUFMOD_INVERT)) {
		if (modify_how == BUFMOD_CLEAR)
			i = 0x00; //clear
		else
			i = 0xFF; //invert
		if (edit.fonttype == SFONT_T) { ++buf; --len; }
		for (;len;--len,++buf) *buf = (*buf & i) ^ i;
	}
	return;
}



void font_EditbufToSprite(void) {
	uint8_t *ptr,x,y;
	
	ptr = tempsprite;
	*ptr = edit.xlim;
	++ptr;
	*ptr = edit.ylim;
	++ptr;
	
	for (y = 0; y < edit.ylim; ++y) {
		for (x = 0; x < edit.xlim; ++x) {
			*ptr = (GetFontBit(x,y,edit.fonttype)) ? 0x00 : 0xFF;
			++ptr;
		}
	}
}
void font_SpriteToEditbuf(void) {
	uint8_t *ptr,x,y;
	
	ptr = tempsprite+2;
	
	for (y = 0; y < edit.ylim; ++y) {
		for (x = 0; x < edit.xlim; ++x) {
			if (*ptr)
				ResFontBit(x,y,edit.fonttype);
			else
				SetFontBit(x,y,edit.fonttype);
			++ptr;
		}
	}
}


