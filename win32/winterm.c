/* 
 * Copyright (C) 2001 Scott Klement
 * 
 * This file is part of TN5250
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1, or (at your option)
 * any later version.
 * 
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 */

#define _TN5250_TERMINAL_PRIVATE_DEFINED
#include "tn5250-private.h"
#include "resource.h"

#define TN5250_WNDCLASS "tn5250-win32-terminal"
#define WM_TN5250_STREAM_DATA (WM_USER+2000)
#define WM_TN5250_KEY_DATA (WM_USER+2001)
#define MB_BEEPFILE (MB_OK+2000)

/* Mapping of 5250 colors to windows colors */
#define A_5250_GREEN    RGB(0,255,0)
#define A_5250_WHITE    RGB(255,255,255)
#define A_5250_RED      RGB(255,0,0)
#define A_5250_TURQ     RGB(0,128,128)
#define A_5250_YELLOW   RGB(255,255,0)
#define A_5250_PINK     RGB(255,0,255)  
#define A_5250_BLUE     RGB(0,255,255)
#define A_5250_BLACK    RGB(0,0,0)


#define A_UNDERLINE  0x01
#define A_BLINK      0x02
#define A_NONDISPLAY 0x04
#define A_VERTICAL   0x08
#define A_REVERSE    0x10

struct _Tn5250Win32Attribute {
   COLORREF fg;
   UINT flags;
};
typedef struct _Tn5250Win32Attribute Tn5250Win32Attribute;


static Tn5250Win32Attribute attribute_map[] = 
{
 { A_5250_GREEN, 0 },
 { A_5250_GREEN, A_REVERSE },
 { A_5250_WHITE, 0 },
 { A_5250_WHITE, A_REVERSE },
 { A_5250_GREEN, A_UNDERLINE },
 { A_5250_GREEN, A_REVERSE|A_UNDERLINE },
 { A_5250_WHITE, A_UNDERLINE },
 { A_5250_BLACK, A_NONDISPLAY },
 { A_5250_RED,   0 },
 { A_5250_RED,   A_REVERSE },
 { A_5250_RED,   A_BLINK },
 { A_5250_RED,   A_REVERSE|A_BLINK },
 { A_5250_RED,   A_UNDERLINE },
 { A_5250_RED,   A_REVERSE|A_UNDERLINE },
 { A_5250_RED,   A_UNDERLINE|A_BLINK },
 { A_5250_BLACK, A_NONDISPLAY },
 { A_5250_TURQ,  A_VERTICAL },
 { A_5250_TURQ,  A_REVERSE|A_VERTICAL },
 { A_5250_YELLOW,A_VERTICAL },
 { A_5250_YELLOW,A_REVERSE|A_VERTICAL },
 { A_5250_TURQ,  A_VERTICAL|A_UNDERLINE },
 { A_5250_TURQ,  A_REVERSE|A_VERTICAL|A_UNDERLINE },
 { A_5250_YELLOW,A_VERTICAL|A_UNDERLINE },
 { A_5250_BLACK, A_REVERSE|A_NONDISPLAY },
 { A_5250_PINK,  0 },
 { A_5250_PINK,  A_REVERSE },
 { A_5250_BLUE,  0 },
 { A_5250_BLUE,  A_REVERSE },
 { A_5250_PINK,  A_UNDERLINE },
 { A_5250_PINK,  A_REVERSE|A_UNDERLINE },
 { A_5250_BLUE,  A_UNDERLINE },
 { A_5250_BLACK, A_NONDISPLAY },
};
 
static HFONT font_80;
static HFONT font_132;
static HBITMAP screenbuf;
static Tn5250Terminal *globTerm = NULL;
static Tn5250Display *globDisplay = NULL;
static HDC bmphdc;
static HBITMAP caretbm;
static HACCEL globAccel;

static void win32_terminal_init(Tn5250Terminal * This);
static void win32_terminal_term(Tn5250Terminal * This);
static void win32_terminal_destroy(Tn5250Terminal *This);
void tn5250_win32_init_fonts (Tn5250Terminal *This,
                                 const char *myfont80, const char *myfont132);
static void win32_terminal_font(Tn5250Terminal *This, const char *fontname,
                         int cols, int rows);
static int win32_terminal_width(Tn5250Terminal * This);
static int win32_terminal_height(Tn5250Terminal * This);
static int win32_terminal_flags(Tn5250Terminal * This);
static void win32_terminal_update(Tn5250Terminal * This,
				   Tn5250Display * display);
static void win32_terminal_update_indicators(Tn5250Terminal * This,
					      Tn5250Display * display);
static int win32_terminal_set_config(Tn5250Terminal *This, Tn5250Config *conf);
static int win32_terminal_waitevent(Tn5250Terminal * This);
static int win32_terminal_getkey(Tn5250Terminal * This);
void win32_terminal_queuekey(Tn5250Terminal *This, int key);
void win32_terminal_clear_screenbuf(HWND hwnd,int width,int height,int delet);
void tn5250_win32_set_beep (Tn5250Terminal *This, const char *beepfile);
void tn5250_win32_terminal_display_ruler (Tn5250Terminal *This, int f);
static void win32_terminal_beep(Tn5250Terminal * This);
void win32_terminal_draw_text(HDC hdc, int attr, const char *text, int len, int x, int y, int *spacing);
LRESULT CALLBACK 
win32_terminal_wndproc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void win32_make_new_caret(Tn5250Terminal *This);
void win32_move_caret(HDC hdc, Tn5250Terminal *This);
void win32_hide_caret(HDC hdc, Tn5250Terminal *This);
void win32_expand_text_selection(Tn5250Terminal *This);
void win32_copy_text_selection(Tn5250Terminal *This, Tn5250Display *display);
void win32_paste_text_selection(HWND hwnd, Tn5250Terminal *term, 
                                           Tn5250Display *display);

extern void msgboxf(const char *fmt, ...);

struct _MY_POINT {
   int x;
   int y;
};
typedef struct _MY_POINT MY_POINT;


#define MAX_K_BUF_LEN 64
struct _Tn5250TerminalPrivate {
   HINSTANCE	  hInst;
   HINSTANCE	  hPrev;
   HWND		  hwndMain;
   HFONT          font;
   unsigned int	  beeptype;
   char *	  beepfile;
   int		  show;
   int		  last_width, last_height;
   char *         font_80;
   char *         font_132;
   DWORD          k_buf[MAX_K_BUF_LEN];
   int            k_buf_len;
   int            font_height;
   int            font_width;
   int            caretx;
   int            carety;
   int            caretok;
   MY_POINT	  selstr;
   MY_POINT	  selend;
   int  *         spacing;       
   Tn5250Config * config;
   BYTE           copymode;
   int            resized : 1;
   int		  quit_flag : 1;
   int            display_ruler : 1;
   int            is_focused : 1;
   int            selecting: 1;
   int            selected: 1;
   int            maximized: 1;
   int            dont_auto_size: 1;
   int		  unix_like_copy: 1;
};


/* 
 *  This array converts windows "keystrokes" into character messages
 *  for those that aren't handled by the "TranslateMessage" function.
 *
 *  The smaller this array is, the better the keyboard handling will
 *  perform :)
 *
 */

typedef struct _func_key {
    SHORT keystate;
    DWORD win32_key;
    DWORD func_key;
    BYTE   ctx;
    BYTE   ext;
} keystroke_to_msg;

static keystroke_to_msg keydown2msg[] =
{
/*  KeyState    Win32 VirtKey    5250 key   ctx ext  */
   { VK_SHIFT,   VK_TAB,          K_BACKTAB, 0, 0 },
   { VK_SHIFT,   VK_F1,           K_F13    , 0, 0 },   
   { VK_SHIFT,   VK_F2,           K_F14    , 0, 0 },   
   { VK_SHIFT,   VK_F3,           K_F15    , 0, 0 },   
   { VK_SHIFT,   VK_F4,           K_F16    , 0, 0 },   
   { VK_SHIFT,   VK_F5,           K_F17    , 0, 0 },   
   { VK_SHIFT,   VK_F6,           K_F18    , 0, 0 },   
   { VK_SHIFT,   VK_F7,           K_F19    , 0, 0 },   
   { VK_SHIFT,   VK_F8,           K_F20    , 0, 0 },   
   { VK_SHIFT,   VK_F9,           K_F21    , 0, 0 },   
   { VK_SHIFT,   VK_F10,          K_F22    , 0, 0 },   
   { VK_SHIFT,   VK_F11,          K_F23    , 0, 0 },   
   { VK_SHIFT,   VK_F12,          K_F24    , 0, 0 },   
   { 0,          VK_F1,           K_F1     , 0, 0 },   
   { 0,          VK_F2,           K_F2     , 0, 0 },   
   { 0,          VK_F3,           K_F3     , 0, 0 },   
   { 0,          VK_F4,           K_F4     , 0, 0 },   
   { 0,          VK_F5,           K_F5     , 0, 0 },   
   { 0,          VK_F6,           K_F6     , 0, 0 },   
   { 0,          VK_F7,           K_F7     , 0, 0 },   
   { 0,          VK_F8,           K_F8     , 0, 0 },   
   { 0,          VK_F9,           K_F9     , 0, 0 },   
   { 0,          VK_F10,          K_F10    , 0, 0 },   
   { 0,          VK_F11,          K_F11    , 0, 0 },   
   { 0,          VK_F12,          K_F12    , 0, 0 },   
   { 0,          VK_PRIOR,        K_ROLLDN , 0, 1 },
   { 0,          VK_NEXT,         K_ROLLUP , 0, 1 },
   { 0,          VK_UP,	          K_UP     , 0, 1 },
   { 0,          VK_DOWN,         K_DOWN   , 0, 1 },
   { 0,          VK_LEFT,         K_LEFT   , 0, 1 },
   { 0,          VK_RIGHT,        K_RIGHT  , 0, 1 },
   { 0,          VK_INSERT,       K_INSERT , 0, 1 },
   { 0,          VK_DELETE,       K_DELETE , 0, 1 },
   { 0,          VK_HOME,         K_HOME   , 0, 1 },
   { 0,          VK_END,          K_END    , 0, 1 },
   { 0,          VK_ADD,          K_FIELDPLUS  , 0, 0 },
   { 0,          VK_SUBTRACT,     K_FIELDMINUS , 0, 0 },
   { 0,          VK_SCROLL,       K_HELP       , 0, 0 },
   { -1, -1 },
};

static keystroke_to_msg keyup2msg[] =
{
/*  KeyState    Win32 VirtKey    5250 key     ctx ext  */
   { 0,          VK_CONTROL,      K_RESET    , 0, 0 },
   { 0,          VK_CONTROL,      K_FIELDEXIT, 0, 1 },
   { VK_MENU,    VK_SNAPSHOT,     K_SYSREQ,    1, 0 },
   { 0,          VK_SNAPSHOT,     K_PRINT,     1, 0 },
   { -1, -1 },
};

/*
 * This array translates Windows "Character messages"  into 5250
 * keys.   Some function keys are not available as character messages,
 * so those need to be handled in the array above.
 *
 */
typedef struct _key_map {
    DWORD win32_key;
    DWORD tn5250_key;
} win32_key_map;

static win32_key_map win_kb[] =
{
/* Win32 CharMsg       5250 key   */
   { 0x01,   K_ATTENTION },   /* CTRL-A */
   { 0x02,   K_ROLLDN },      /* CTRL-B */
   { 0x03,   K_SYSREQ },      /* CTRL-C */
   { 0x04,   K_ROLLUP },      /* CTRL-D */
   { 0x05,   K_ERASE },       /* CTRL-E */
   { 0x06,   K_ROLLUP },      /* CTRL-F */
   { VK_BACK,K_BACKSPACE },   /* CTRL-H (backspace key) */
   { 0x0b,   K_FIELDEXIT },   /* CTRL-K */
   { 0x0c,   K_REFRESH },     /* CTRL-L */
   { 0x0f,   K_HOME },        /* CTRL-O */
   { 0x10,   K_PRINT },       /* CTRL-P */
   { 0x12,   K_RESET },       /* CTRL-R */
   { 0x14,   K_TESTREQ },     /* CTRL-T */
   { 0x15,   K_ROLLDN },      /* CTRL-U */
   { 0x18,   K_FIELDPLUS },   /* CTRL-X */
   { 0x1b,   K_ATTENTION },   /* ESCAPE */
   { -1, -1 },
};




/****f* lib5250/tn5250_win32_terminal_new
 * NAME
 *    tn5250_win32_terminal_new
 * SYNOPSIS
 *    ret = tn5250_win32_terminal_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    Create a new Windows terminal object
 *****/
Tn5250Terminal *tn5250_win32_terminal_new(HINSTANCE hInst, 
	 HINSTANCE hPrev, int show)
{
   Tn5250Terminal *r = tn5250_new(Tn5250Terminal, 1);
   if (r == NULL)
      return NULL;

   r->data = tn5250_new(struct _Tn5250TerminalPrivate, 1);
   if (r->data == NULL) {
      g_free(r);
      return NULL;
   }

   r->data->hInst = hInst;
   r->data->hPrev = hPrev;
   r->data->show = show;
   r->data->beeptype = MB_OK;
   r->data->beepfile = NULL;
   r->data->quit_flag = 0;
   r->data->last_width = 0;
   r->data->last_height = 0;
   r->data->font_80 = NULL;
   r->data->font_132 = NULL;
   r->data->display_ruler = 0;
   r->data->spacing = NULL;
   r->data->caretx = 0;
   r->data->caretok = 0;
   r->data->carety = 0;
   r->data->quit_flag = 0;
   r->data->k_buf_len = 0;
   r->data->selstr.x = 0;
   r->data->selstr.y = 0;
   r->data->selend.x = 0;
   r->data->selend.y = 0;
   r->data->selecting = 0;
   r->data->selected = 0;
   r->data->copymode = 0;
   r->data->config = NULL;
   r->data->maximized = 0;
   r->data->dont_auto_size = 0;
   r->data->unix_like_copy = 0;

   r->conn_fd = -1;
   r->init = win32_terminal_init;
   r->term = win32_terminal_term;
   r->destroy = win32_terminal_destroy;
   r->width = win32_terminal_width;
   r->height = win32_terminal_height;
   r->flags = win32_terminal_flags;
   r->update = win32_terminal_update;
   r->update_indicators = win32_terminal_update_indicators;
   r->waitevent = win32_terminal_waitevent;
   r->getkey = win32_terminal_getkey;
   r->beep = win32_terminal_beep;
   r->config = win32_terminal_set_config;

   return r;
}


/****i* lib5250/win32_terminal_init
 * NAME
 *    win32_terminal_init
 * SYNOPSIS
 *    win32_terminal_init (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void win32_terminal_init(Tn5250Terminal * This)
{
   int i = 0, c, s;
   struct timeval tv;
   char *str;
   HDC hdc;
   RECT rect;
   int height, width;
   int len;
   char *title;

   WNDCLASSEX wndclass;

   This->data->spacing = NULL;
   This->data->caretx = 0;
   This->data->caretok = 0;
   This->data->carety = 0;
   This->data->quit_flag = 0;
   This->data->k_buf_len = 0;
   This->data->copymode = 0;
   globTerm = This;

   if (This->data->config != NULL) {

      if (tn5250_config_get(This->data->config, "copymode")) {
          if (!strcasecmp(tn5250_config_get(This->data->config,"copymode"), 
                 "text"))
             This->data->copymode = 1;
          if (!strcasecmp(tn5250_config_get(This->data->config,"copymode"), 
                 "bitmap"))
             This->data->copymode = 2;
      }


      if (tn5250_config_get (This->data->config, "ruler")) {
           tn5250_win32_terminal_display_ruler(This,
                     tn5250_config_get_bool (This->data->config, "ruler"));
      }

      if ( tn5250_config_get (This->data->config, "pcspeaker")) 
          tn5250_win32_set_beep(This, "!!PCSPEAKER!!");
      else 
          tn5250_win32_set_beep(This, 
               tn5250_config_get (This->data->config, "beepfile"));

      if ( tn5250_config_get (This->data->config, "unix_like_copy")) {
            This->data->unix_like_copy = 
               tn5250_config_get_bool(This->data->config, "unix_like_copy");
      }

   }

   
   if ( tn5250_config_get (This->data->config, "host") ) {
      if ( tn5250_config_get (This->data->config, "env.DEVNAME") ) { 
          len = strlen(tn5250_config_get(This->data->config,"host"));
          len += strlen(tn5250_config_get(This->data->config,"env.DEVNAME"));
          len += strlen("tn5250 - x - x");
          title = g_malloc(len+1);
          sprintf(title, "tn5250 - %s - %s", 
                 tn5250_config_get(This->data->config, "env.DEVNAME"),
                 tn5250_config_get(This->data->config, "host"));
      }
      else {
          len = strlen("tn5250 - x");
          len += strlen(tn5250_config_get(This->data->config, "host"));
          title = g_malloc(len+1);
          sprintf(title, "tn5250 - %s", 
                 tn5250_config_get(This->data->config, "host"));
      }
   }
   else {
      len = strlen("tn5250");
      title = g_malloc(len+1);
      strcpy(title, "tn5250");
   }

   /* build a Window Class */

   memset (&wndclass, 0, sizeof(WNDCLASSEX));
   wndclass.lpszClassName = TN5250_WNDCLASS;
   wndclass.lpszMenuName = "tn5250-win32-menu";
   wndclass.cbSize = sizeof(WNDCLASSEX);
   wndclass.style = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc = win32_terminal_wndproc;
   wndclass.hInstance = This->data->hInst;
   wndclass.hIcon = LoadIcon (This->data->hInst, MAKEINTRESOURCE(IDI_TN5250_ICON));
   wndclass.hIconSm = LoadIcon (This->data->hInst, MAKEINTRESOURCE(IDI_TN5250_ICON));
   wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
   RegisterClassEx (&wndclass);

   /* create our main window */
   This->data->hwndMain = CreateWindow (TN5250_WNDCLASS, 
	    		     title,
		             WS_OVERLAPPEDWINDOW,
			     CW_USEDEFAULT,
	       		     CW_USEDEFAULT,
		  	     CW_USEDEFAULT,
			     CW_USEDEFAULT,
	 		     NULL,
		  	     NULL,
			     This->data->hInst,
			     NULL
			 );
    g_free(title);


   /* create a bitmap act as a screen buffer.  we will draw all of
      our data to this buffer, then BitBlt it to the screen when we
      need to re-draw the screen.   This'll make it easy for us to
      re-paint sections of the screen when necessary */

   GetClientRect(This->data->hwndMain, &rect);
   width = (rect.right - rect.left) + 1;
   height = (rect.bottom - rect.top) + 1;
   bmphdc = CreateCompatibleDC(NULL);
   win32_terminal_clear_screenbuf(This->data->hwndMain, width, height, 0);

   tn5250_win32_init_fonts(This, 
             tn5250_config_get(This->data->config, "font_80"),
             tn5250_config_get(This->data->config, "font_132"));

   ShowWindow (This->data->hwndMain, This->data->show);
   UpdateWindow (This->data->hwndMain);

   globAccel = LoadAccelerators (This->data->hInst, "tn5250-win32-menu");

   /* FIXME: This might be a nice place to load the keyboard map */

}


/****i* lib5250/win32_terminal_set_config
 * NAME
 *    win32_terminal_set_config
 * SYNOPSIS
 *    win32_terminal_set_config (This, config);
 * INPUTS
 *    Tn5250Terminal  *    This       - The win32 terminal object.
 *    Tn5250Config    *    conf       - config object
 * DESCRIPTION
 *     Associates a config object with the terminal object
 *****/
static int win32_terminal_set_config(Tn5250Terminal *This, Tn5250Config *conf)
{

     This->data->config = conf;
     if (conf==NULL) return -1;
     return 0;
}


/****i* lib5250/tn5250_win32_terminal_display_ruler
 * NAME
 *    tn5250_win32_terminal_display_ruler
 * SYNOPSIS
 *    tn5250_win32_terminal_display_ruler (This, f);
 * INPUTS
 *    Tn5250Terminal  *    This       - The win32 terminal object.
 *    int                  f          - Flag, set to 1 to show ruler
 * DESCRIPTION
 *    Call this function to tell the Win32 terminal to display a
 *    "ruler" that pinpoints where the cursor is on a given screen.
 *****/
void tn5250_win32_terminal_display_ruler (Tn5250Terminal *This, int f)
{
   This->data->display_ruler = f;
}


/****i* lib5250/tn5250_win32_set_beep
 * NAME
 *    tn5250_win32_set_beep
 * SYNOPSIS
 *    tn5250_win32_set_beep
 * INPUTS
 *    Tn5250Terminal  *    This       - The win32 terminal object.
 *    const char      *    beepfile   - \path\to\file.wav
 * DESCRIPTION
 *     Call this function to change how a beep works.  If you send
 *     a NULL, we use the standard windows "SystemDefault" beep sound.
 *     If you send a "!!PCSPEAKER!!", we use the PC speaker instead.
 *     Otherwise, the filename will be a .wav file to be played.
 *     (deluxe, eh?)
 *****/
void tn5250_win32_set_beep (Tn5250Terminal *This, const char *beepfile)
{
      if (This->data->beepfile != NULL) 
	  g_free(This->data->beepfile);

      if (beepfile==NULL) 
	  This->data->beeptype = MB_OK;
      else if (!strcmp(beepfile, "!!PCSPEAKER!!"))
	  This->data->beeptype = 0xFFFFFFFF;
      else  {
	  This->data->beeptype = MB_BEEPFILE;
	  This->data->beepfile = g_malloc(strlen(beepfile)+1);
	  strcpy(This->data->beepfile, beepfile);
      }

}


/****i* lib5250/tn5250_win32_init_fonts
 * NAME
 *  tn5250_win32_init_fonts
 * SYNOPSIS
 *    tn5250_win32_init_fonts (This, font80, font132);
 * INPUTS
 *    Tn5250Terminal  *   This       - win32 terminal object
 *    const char  *       font80     - string to send when using 80 col font
 *    const char  *       font132    - string to send when using 132 col font
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_win32_init_fonts (Tn5250Terminal *This,
                                 const char *myfont80, const char *myfont132)
{

   if (myfont80 != NULL) {
       This->data->font_80 = g_malloc(strlen(myfont80) + 6);
       strcpy(This->data->font_80, myfont80);
   } else {
       This->data->font_80 = NULL;
   }
   if (myfont132 != NULL) {
       This->data->font_132 = g_malloc(strlen(myfont132) + 6);
       strcpy(This->data->font_132, myfont132);
   } else {
       This->data->font_132 = NULL;
   }

   win32_terminal_font(This, This->data->font_80, 80, 25);
}


/****i* lib5250/win32_terminal_font
 * NAME
 *    win32_terminal_font
 * SYNOPSIS
 *    win32_terminal_font (This, "Arial", 80, 24);
 * INPUTS
 *    Tn5250Terminal  *    This       - The win32 terminal object.
 *    const char      *    fontname   - name of font to try
 *    int                  cols       - number of screen cols to size font for
 *    int                  rows       - number of screen rows to size font for
 * DESCRIPTION
 *    This takes the font name, columns & rows that you pass
 *    and finds the closest match on the system.   It then
 *    re-sizes the terminal window to the size of the font
 *    that it calculated.
 *****/
static void win32_terminal_font(Tn5250Terminal *This, 
               const char *fontname, int cols, int rows)  {

   int opt_height, opt_width;
   int cli_height, cli_width;
   int win_height, win_width;
   RECT cr, wr;
   HDC hdc;
   TEXTMETRIC tm;
   HFONT oldfont;

   hdc = GetDC(This->data->hwndMain);

   /* calculate height of our entire window */

   if (!GetWindowRect(This->data->hwndMain, &wr))
       return;
   win_height = (wr.bottom - wr.top ) + 1;
   win_width =  (wr.right  - wr.left) + 1;


   /* calculate height of the client portion of the window */

   if (!GetClientRect(This->data->hwndMain, &cr))
        return;
   cli_height = (cr.bottom - cr.top ) + 1;
   cli_width =  (cr.right  - cr.left) + 1;


   /* figure out the optimal font size */

   opt_height = cli_height / rows;
   opt_width =  cli_width  / cols;


   /* create a font using a size as close as possible to the optimal
      size that we calculated */

   This->data->font = CreateFont(opt_height, opt_width, 0, 0, FW_DONTCARE, 
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, 
        fontname);

   /* set the new font as active and get rid of the old one */

   oldfont = SelectObject(hdc, This->data->font);
   DeleteObject(oldfont);

   /* calculate the actual size of the font we selected */

   GetTextMetrics(hdc, &tm);
   ReleaseDC(This->data->hwndMain, hdc);
   This->data->font_height = tm.tmHeight + tm.tmExternalLeading;
   This->data->font_width = tm.tmAveCharWidth;


   /* set up the font spacing array for this font */
   {
      int x;
      if (This->data->spacing != NULL)
            g_free(This->data->spacing);
      This->data->spacing = g_malloc((cols+1)*sizeof(int)); 
      for (x=0; x<=cols; x++) 
            This->data->spacing[x] = This->data->font_width;
   }

   /* make our window size match the font size */

   win_height = (win_height - cli_height) + This->data->font_height * rows;
   win_width =  (win_width  - cli_width ) + This->data->font_width  * cols;

   if (globDisplay != NULL) {
        This->data->caretx = tn5250_display_cursor_x(globDisplay)
                  * This->data->font_width;
        This->data->carety = tn5250_display_cursor_y(globDisplay)
                  * This->data->font_height;
   }

   if (This->data->hwndMain == GetFocus()) 
       win32_hide_caret(hdc, This);

   if ((!This->data->maximized)&&(!This->data->dont_auto_size)) {
        TN5250_LOG(("WIN32: Re-sizing window to %d by %d\n", win_width, 
                    win_height));
        SetWindowPos(This->data->hwndMain, NULL, 0, 0, win_width, win_height, 
                    SWP_NOMOVE | SWP_NOZORDER);
   }

   if (This->data->hwndMain == GetFocus()) {
       hdc = GetDC(This->data->hwndMain);      
       win32_make_new_caret(This);
       win32_move_caret(hdc, This);
       ReleaseDC(This->data->hwndMain, hdc);
#ifndef NOBLINK
       ShowCaret (This->data->hwndMain);
#endif
   }
}


/****i* lib5250/win32_terminal_term
 * NAME
 *    win32_terminal_term
 * SYNOPSIS
 *    win32_terminal_term (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void win32_terminal_term(Tn5250Terminal /*@unused@*/ * This)
{
   PostQuitMessage(0);
}

/****i* lib5250/win32_terminal_destroy
 * NAME
 *    win32_terminal_destroy
 * SYNOPSIS
 *    win32_terminal_destroy (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void win32_terminal_destroy(Tn5250Terminal * This)
{
   if (This->data->font_80 !=NULL)
      g_free(This->data->font_80);
   if (This->data->font_132 !=NULL)
      g_free(This->data->font_132);
   if (This->data->beepfile != NULL)
      g_free(This->data->beepfile);
   if (This->data != NULL)
      g_free(This->data);
   g_free(This);
   DeleteDC(bmphdc);
}

/****i* lib5250/win32_terminal_width
 * NAME
 *    win32_terminal_width
 * SYNOPSIS
 *    ret = win32_terminal_width (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    Returns the current width (in chars) of the terminal.
 *****/
static int win32_terminal_width(Tn5250Terminal *This)
{
   RECT r;
   GetClientRect(This->data->hwndMain, &r);
   return ( (r.right - r.left) / This->data->font_width ) + 1;
}

/****i* lib5250/win32_terminal_height
 * NAME
 *    win32_terminal_height
 * SYNOPSIS
 *    ret = win32_terminal_height (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    Returns the current height of the terminal.
 *****/
static int win32_terminal_height(Tn5250Terminal *This)
{
   RECT r;
   GetClientRect(This->data->hwndMain, &r);
   return ( (r.bottom-r.top) / (This->data->font_height) ) + 1;
}

/****i* lib5250/win32_terminal_flags
 * NAME
 *    win32_terminal_flags
 * SYNOPSIS
 *    ret = win32_terminal_flags (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int win32_terminal_flags(Tn5250Terminal /*@unused@*/ * This)
{
   int f = 0;
   f |= TN5250_TERMINAL_HAS_COLOR;
   return f;
}

/****i* lib5250/win32_terminal_update
 * NAME
 *    win32_terminal_update
 * SYNOPSIS
 *    win32_terminal_update (This, display);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    Tn5250Display *      display    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void win32_terminal_update(Tn5250Terminal * This, Tn5250Display *display)
{
   int y, x;
   int mx, my;
   HDC hdc;
   unsigned char attr, c;
   unsigned char text[132*27];
   RECT cr;
   HBRUSH oldbrush;
   HPEN oldpen;
   int len;
   TEXTMETRIC tm;

   /* we do all drawing to the screen buffer, then bitblt that to the
      actual window when WM_PAINT occurs */

   SelectObject(bmphdc, screenbuf);

   /* clear the screen buffer (one big black rectangle) */

   GetClientRect(This->data->hwndMain, &cr);
   oldbrush = SelectObject(bmphdc, GetStockObject(BLACK_BRUSH));
   oldpen = SelectObject(bmphdc, GetStockObject(BLACK_PEN));
   Rectangle(bmphdc, cr.left, cr.top, cr.right, cr.bottom);
   SelectObject(bmphdc, oldbrush);
   SelectObject(bmphdc, oldpen);

   if (This->data->resized ||
       This->data->last_height != tn5250_display_height(display)  ||
       This->data->last_width != tn5250_display_width(display)) {
          if (tn5250_display_width (display)<100) {
              This->data->dont_auto_size = 0;
              win32_terminal_font(This, This->data->font_80, 
                  tn5250_display_width(display),
                  tn5250_display_height(display)+1);
          } else {
              This->data->dont_auto_size = 1;
              win32_terminal_font(This, This->data->font_132, 
                  tn5250_display_width(display),
                  tn5250_display_height(display)+1);
          }
          This->data->last_height = tn5250_display_height(display);
          This->data->last_width  = tn5250_display_width (display);
          This->data->resized = 0;
   }

   SelectObject(bmphdc, This->data->font);
   SetTextAlign(bmphdc, TA_TOP|TA_LEFT|TA_NOUPDATECP);

   attr = 0x20;
   len = 0;

   for (y = 0; y < tn5250_display_height(display); y++) {

      for (x = 0; x < tn5250_display_width(display); x++) {
	 c = tn5250_display_char_at(display, y, x);
	 if ((c & 0xe0) == 0x20) {	/* ATTRIBUTE */
            if (len>0) 
                win32_terminal_draw_text(bmphdc, attr, text, len, mx, my, 
                  This->data->spacing);
            len = 0;
	    attr = (c & 0xff);
	 } else {                       /* DATA */
            if (len==0) {
                mx = x; 
                my = y;
            }
            if ((c==0x1f) || (c==0x3F)) {
                if (len>0)
                     win32_terminal_draw_text(bmphdc, attr, text, len, mx, my,
                       This->data->spacing);
                len = 0;
                c = ' ';
                win32_terminal_draw_text(bmphdc, 0x21, &c, 1, x, y, 
                  This->data->spacing);
            } else if ((c < 0x40 && c > 0x00) || (c == 0xff)) {
                text[len] = ' ';
                len++;
            } else {
                text[len] = tn5250_char_map_to_local(
                               tn5250_display_char_map(display), c);
                len++;
            }
	 }			
      }			

      if (len>0) 
          win32_terminal_draw_text(bmphdc, attr, text, len, mx, my,
            This->data->spacing);
      len = 0;

   }

   This->data->caretok = 0;
   
   win32_terminal_update_indicators(This, display);
   return;
}

/****i* lib5250/win32_terminal_draw_text
 * NAME
 *    win32_terminal_draw_text
 * SYNOPSIS
 *    win32_terminal_draw_text (hdc, a, "Hello", 5, 12, 5);
 * INPUTS
 *    HDC               hdc          - Device context to draw onto
 *    int               attr         - 5250 attribute byte
 *    const char *      text         - text to draw
 *    int               len          - length of text
 *    int               x            - position to start (along x axis)
 *    int               y            - position to start (along y axis)
 *    int        *      spacing      - pointer to array specifying char spacing
 * DESCRIPTION
 *    This draws text on the terminal in the specified attribute
 *****/
void win32_terminal_draw_text(HDC hdc, int attr, const char *text, int len, int x, int y, int *spacing) {
 
    static UINT flags;
    static RECT rect;

    flags = attribute_map[attr-0x20].flags;
    
    /* hmm..  how _do_ you draw something that's invisible? */
    if (flags&A_NONDISPLAY) 
       return;

    /* create a rect to "opaque" our text.  (defines the background area
       that the text is painted on) */

    rect.top = y * globTerm->data->font_height;
    rect.bottom = rect.top + globTerm->data->font_height;
    rect.left = x * globTerm->data->font_width;
    rect.right = rect.left + (globTerm->data->font_width * len);

    /* this builds an array telling Windows how to space the text.
       Some fonts end up being slightly smaller than the text metrics
       tell us that they are... so without this, the caret (aka "cursor")
       will not appear in the right place.  */

    /* FIXME:
       this is a hack copied from cursesterm.c which does underlines
       since vertical lines aren't available.  If I weren't so lazy,
       I could do proper vertical lines, here :)  */


    /* set up colors for this drawing style */

    SetBkMode(hdc, OPAQUE);
    if (flags&A_REVERSE) {
         SetBkColor(hdc, attribute_map[attr-0x20].fg);
         SetTextColor(hdc, A_5250_BLACK);
    } else {
         SetBkColor(hdc, A_5250_BLACK);
         SetTextColor(hdc, attribute_map[attr-0x20].fg);
    }


#ifdef LOG_DRAWTEXT
    /* debugging: log what we're drawing */
    {
        char *dbg;
        dbg = g_malloc(len+1);
        memcpy(dbg, text, len);
        dbg[len]=0;
        TN5250_LOG(("WIN32: draw text(%d, %d) %s\n", x,y , dbg));
        g_free(dbg);
    }
#endif

    /* draw the text */

    if (ExtTextOut(hdc, rect.left, rect.top, ETO_CLIPPED|ETO_OPAQUE, &rect, 
       text, len, spacing)==0) {
         msgboxf("ExtTextOut(): Error %d\n", GetLastError());
    }

    /* draw underlines */
    /* Note: We don't use the underlining capability of the font itself
             because on some fonts, it changes the font height, and that
             would just mess us up.   */

    if (flags&A_UNDERLINE) {
       HPEN savepen;
       savepen = SelectObject(hdc, 
                   CreatePen(PS_SOLID, 0, attribute_map[attr-0x20].fg));
       MoveToEx(hdc, rect.left, rect.bottom-1, NULL);
       LineTo(hdc, rect.right, rect.bottom-1);
       savepen = SelectObject(hdc, savepen);
       DeleteObject(savepen);
    }
    if (flags&A_VERTICAL) { 
       HPEN savepen;
       if (flags&A_REVERSE) 
           savepen = SelectObject(hdc, CreatePen(PS_SOLID, 0, A_5250_BLACK));
       else
           savepen = SelectObject(hdc, CreatePen(PS_SOLID, 0, 
                                               attribute_map[attr-0x20].fg));
       for (x=rect.left; x<=rect.right; x+=spacing[0]) {
           MoveToEx(hdc, x, rect.top, NULL);
           LineTo  (hdc, x, rect.bottom);
       }
       MoveToEx(hdc, rect.right, rect.top, NULL);
       LineTo  (hdc, rect.right, rect.bottom);
       savepen = SelectObject(hdc, savepen);
       DeleteObject(savepen);
    }

    return;
}


/****i* lib5250/win32_terminal_update_indicators
 * NAME
 *    win32_terminal_update_indicators
 * SYNOPSIS
 *    win32_terminal_update_indicators (This, display);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 *    Tn5250Display *      display    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void win32_terminal_update_indicators(Tn5250Terminal *This, Tn5250Display *display)
{
   int inds = tn5250_display_indicators(display);
   char ind_buf[80];
   HDC hdc;
   static unsigned char c;

   SelectObject(bmphdc, screenbuf);
   SetTextAlign(bmphdc, TA_TOP|TA_LEFT|TA_NOUPDATECP);
   SelectObject(bmphdc, This->data->font);

   memset(ind_buf, ' ', sizeof(ind_buf));
   memcpy(ind_buf, "5250", 4);
   if ((inds & TN5250_DISPLAY_IND_MESSAGE_WAITING) != 0)
      memcpy(ind_buf + 23, "MW", 2);
   if ((inds & TN5250_DISPLAY_IND_INHIBIT) != 0)
      memcpy(ind_buf + 9, "X II", 4);
   else if ((inds & TN5250_DISPLAY_IND_X_CLOCK) != 0)
      memcpy(ind_buf + 9, "X CLOCK", 7);
   else if ((inds & TN5250_DISPLAY_IND_X_SYSTEM) != 0)
      memcpy(ind_buf + 9, "X SYSTEM", 8);
   if ((inds & TN5250_DISPLAY_IND_INSERT) != 0)
      memcpy(ind_buf + 30, "IM", 2);
   if ((inds & TN5250_DISPLAY_IND_FER) != 0)
      memcpy(ind_buf + 33, "FER", 3);
   sprintf(ind_buf+72,"%03.3d/%03.3d",tn5250_display_cursor_x(display)+1,
      tn5250_display_cursor_y(display)+1);

   win32_terminal_draw_text(bmphdc, 0x22, ind_buf, 79, 0, 
               tn5250_display_height(display), This->data->spacing);

   This->data->caretx=tn5250_display_cursor_x(display)*This->data->font_width;
   This->data->carety=tn5250_display_cursor_y(display)*This->data->font_height;

   globDisplay = display;

   if (This->data->display_ruler) {
       HPEN savepen;
       RECT rect;
       int x, y;
       int savemixmode;
       x = This->data->caretx + This->data->font_width / 2;
       y = This->data->carety + This->data->font_height / 2;
       GetClientRect(This->data->hwndMain, &rect);
       savepen = SelectObject(bmphdc, GetStockObject(WHITE_PEN));
       MoveToEx(bmphdc, x, rect.top, NULL);
       LineTo  (bmphdc, x, rect.bottom);
       MoveToEx(bmphdc, rect.left, y, NULL);
       LineTo  (bmphdc, rect.right, y);
       savepen = SelectObject(hdc, savepen);
   }

   This->data->selected = This->data->selecting = 0;
   This->data->caretok = 0;

   InvalidateRect(This->data->hwndMain, NULL, FALSE);
   UpdateWindow(This->data->hwndMain);
}


/****i* lib5250/win32_terminal_waitevent
 * NAME
 *    win32_terminal_waitevent
 * SYNOPSIS
 *    ret = win32_terminal_waitevent (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int win32_terminal_waitevent(Tn5250Terminal * This)
{
   fd_set fdr;
   int result = 0;
   int sm;
   static MSG msg;


   if (This->data->quit_flag)
      return TN5250_TERMINAL_EVENT_QUIT;

   if (This->conn_fd != -1) {
        if (WSAAsyncSelect(This->conn_fd, This->data->hwndMain, 
               WM_TN5250_STREAM_DATA, FD_READ) == SOCKET_ERROR) {
           TN5250_LOG(("WIN32: WSAASyncSelect failed, reason: %d\n", 
                 WSAGetLastError()));
           return TN5250_TERMINAL_EVENT_QUIT;
        }
   }

   result = TN5250_TERMINAL_EVENT_QUIT;
   while ( GetMessage(&msg, NULL, 0, 0) ) {
      if (!TranslateAccelerator (This->data->hwndMain, globAccel, &msg))
           DispatchMessage(&msg);
      if (msg.message == WM_TN5250_STREAM_DATA) {
         result = TN5250_TERMINAL_EVENT_DATA;
         break;
      }
      if (msg.message == WM_CHAR || msg.message == WM_TN5250_KEY_DATA) {
         result = TN5250_TERMINAL_EVENT_KEY;
         break;
      }
   }

   if (This->conn_fd != -1) 
        WSAAsyncSelect(This->conn_fd, This->data->hwndMain, 0, 0);

   return result;
}


/****i* lib5250/win32_terminal_beep
 * NAME
 *    win32_terminal_beep
 * SYNOPSIS
 *    win32_terminal_beep (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    This plays a beep, either using a .wav file or
 *    by using the stock Windows methods.
 *****/
static void win32_terminal_beep (Tn5250Terminal *This)
{
   if (This->data->beeptype != MB_BEEPFILE) {
        TN5250_LOG (("WIN32: beep\n"));
        MessageBeep(This->data->beeptype);
   } else {
      TN5250_LOG (("WIN32: PlaySound\n"));
      if (!PlaySound(This->data->beepfile, NULL, SND_ASYNC|SND_FILENAME)) {
          TN5250_LOG (("WIN32: PlaySound failed, switching back to beep\n"));
          tn5250_win32_set_beep(This, NULL);
          MessageBeep(This->data->beeptype);
      }
   }
}


/****i* lib5250/win32_get_key
 * NAME
 *    win32_get_key
 * SYNOPSIS
 *    key = win32_get_key (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    Read the next key from the keyboard buffer.
 *****/
static int win32_get_key (Tn5250Terminal *This)
{
   int i, j;
   int have_incomplete_match = -1;
   int have_complete_match = -1;
   int complete_match_len;

   if (This->data->k_buf_len == 0)
      return -1;

   i = This->data->k_buf[0];
   This->data->k_buf_len --;
   for (j=0; j<This->data->k_buf_len; j++) 
        This->data->k_buf[j] = This->data->k_buf[j+1];

#if 0
   {
       char *blah;
       blah = g_malloc(This->data->k_buf_len+1);
       for (j=0; j<This->data->k_buf_len; j++) 
           blah[j] = This->data->k_buf[j];
       blah[This->data->k_buf_len] = '\0';
       TN5250_LOG(("WIN32: getkey %c, %d bytes left:\n", i, This->data->k_buf_len));
       TN5250_LOG(("WIN32: buffer %s\n", blah));
       g_free(blah);
   }
#endif
       
   return i;

}


/****i* lib5250/win32_terminal_getkey
 * NAME
 *    win32_terminal_getkey
 * SYNOPSIS
 *    key = win32_terminal_getkey (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    Read the next key from the terminal, and do any
 *    required translations.
 *****/
static int win32_terminal_getkey (Tn5250Terminal *This)
{
   int ch;

   /* we don't actually read the keyboard here... that's done by
      win32_terminal_wndproc.  */

   ch = win32_get_key (This);
   switch (ch) {
   case K_CTRL('Q'):
      This->data->quit_flag = 1;
      return -1;
   case 0x0a:
      return 0x0d;
   }

   return ch;
}


/****i* lib5250/win32_terminal_queuekey
 * NAME
 *    win32_terminal_queuekey
 * SYNOPSIS
 *    key = win32_terminal_queuekey (This, key);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    int                  key        -
 * DESCRIPTION
 *    Add a key to the terminal's keyboard buffer
 *****/
void win32_terminal_queuekey(Tn5250Terminal *This, int key) {

    if (This->data->k_buf_len<MAX_K_BUF_LEN) {
        This->data->k_buf[This->data->k_buf_len] = key;
        This->data->k_buf_len ++;
    }

}


/****i* lib5250/win32_terminal_new_screenbuf
 * NAME
 *    win32_terminal_new_screenbuf
 * SYNOPSIS
 *    win32_terminal_new_screenbuf (hwnd, width, height);
 * INPUTS
 *    HWND                 hwnd       -
 *    int                  width      -
 *    int                  height     -
 * DESCRIPTION
 *    Create/Resize the bitmap that we use as the screen buffer.
 *****/
void win32_terminal_clear_screenbuf(HWND hwnd,int width,int height,int delet) {

   HDC hdc;
   HBRUSH oldbrush;
   HPEN oldpen;

   if (delet) 
       DeleteObject(screenbuf);

   hdc = GetDC(hwnd);
   screenbuf = CreateCompatibleBitmap(hdc, width, height);
   ReleaseDC(hwnd, hdc);

   SelectObject(bmphdc, screenbuf);
   oldbrush = SelectObject(bmphdc, GetStockObject(BLACK_BRUSH));
   oldpen = SelectObject(bmphdc, GetStockObject(BLACK_PEN));
   Rectangle(bmphdc, 0, 0, width-1, height-1);
   SelectObject(bmphdc, oldbrush);
   SelectObject(bmphdc, oldpen);

   return;
}


/****i* lib5250/win32_terminal_wndproc
 * NAME
 *    win32_terminal_wndproc
 * SYNOPSIS
 *    Called as a result of the DispatchMesage() API.
 * INPUTS
 *    HWND		   hwnd       -
 *    UINT                 msg        -
 *    WPARAM               wParam     -
 *    LPARAM               lParam     -
 * DESCRIPTION
 *    Windows calls this function to report events such
 *    as mouse clicks, keypresses and the receipt of network data.
 *****/
LRESULT CALLBACK 
win32_terminal_wndproc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

     PAINTSTRUCT ps;
     HDC hdc;
     int h, w, x, y;
     RECT rect;
     static BYTE ks[256];
     MSG m;
     int ctx, ext;
     static int handledkey=0;
     HMENU	hMenu;

     switch (msg) {
        case WM_CREATE:
           /* add buttons & other controls here */
           break;

        case WM_DESTROY:
           PostQuitMessage(0);
           return 0;
           break;

        case WM_COMMAND:
	   hMenu = GetMenu(hwnd);
           switch (LOWORD(wParam)) {
              case IDM_APP_EXIT:
		  SendMessage(hwnd, WM_CLOSE, 0, 0);
		  return 0;
                  break;
              case IDM_APP_ABOUT:
                  msgboxf("%s version %s:\n"
                          "Copyright (C) 1997-2002 by Michael Madore,"
                          " Jason M. Felice, and Scott Klement\n"
                          "\n"
                          "Portions of this software were contributed "
                          "by many people.  See the AUTHORS file for\n"
                          "details.\n"
                          "\n"
                          "For license information, see the LICENSE file "
                          "that was installed with this software.\n"
#ifdef HAVE_LIBSSL
                          "\n"
                          "OpenSSL:\n"
                          "This product includes software developed by the "
                          "OpenSSL Toolkit (http://www.openssl.org/)\n"
                          "This product includes cryptographic software "
                          "written by Eric Young (eay@crypsoft.com).\n"
                          "This product includes software written by Tim "
                          "Hudson (tjh@cryptsoft.com).\n"
                          "\n"
                          "For OpenSSL license information, see the LICENSE "
                          "file that was included in the OpenSSL package.\n"
#endif
                          ,PACKAGE, VERSION);
                  return 0;
                  break;
              case IDM_EDIT_COPY:
                  if (globTerm->data->selected) {
                       win32_expand_text_selection(globTerm);
                       win32_copy_text_selection(globTerm, globDisplay);
		       globTerm->data->selected = 0;
                       InvalidateRect(hwnd, NULL, FALSE);
                       UpdateWindow(hwnd);
                       return 0;
                  }
	          break;
              case IDM_EDIT_PASTE:
                  win32_paste_text_selection(hwnd, globTerm, globDisplay);
                  break;
           }
           break;

        case WM_SIZE:
           w = LOWORD(lParam);
           h = HIWORD(lParam);
           if (wParam==SIZE_MAXIMIZED) 
              globTerm->data->maximized = 1;
           else
              globTerm->data->maximized = 0;
           if (h>0 && w>0) {
              win32_terminal_clear_screenbuf(hwnd, w, h, 1);
              if (globTerm!=NULL && globDisplay!=NULL) {
                  globTerm->data->resized = 1;
                  win32_terminal_update(globTerm, globDisplay);
              }
           }
           return 0;
           break;

        case WM_SYSKEYUP:
        case WM_KEYUP:
           ext = (HIWORD (lParam) & 0x0100) >> 8;
           GetKeyboardState(ks); 
           ks[0] = 0xff;   /* so that keystate=0 always works. */
           x = 0;
           while (keyup2msg[x].win32_key != -1) {
               if ((int)wParam == keyup2msg[x].win32_key &&
                   (keyup2msg[x].ctx || !handledkey) &&
                   keyup2msg[x].ext == ext &&
                   (ks[keyup2msg[x].keystate]&0x80)) {
                        win32_terminal_queuekey(globTerm,keyup2msg[x].func_key);
                        PostMessage(hwnd, WM_TN5250_KEY_DATA, 0, 0);
                        return 0;
               }
               x++;
           } 
           break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
           ctx = HIWORD (lParam) & 0x2000;
           ext = (HIWORD (lParam) & 0x0100) >> 8;
           GetKeyboardState(ks); 
           ks[0] = 0xff;   /* so that keystate=0 always works. */
           x = 0;
           handledkey = 0;
           while (keydown2msg[x].win32_key != -1) {
               if ((int)wParam == keydown2msg[x].win32_key &&
                   keydown2msg[x].ctx == ctx &&
                   keydown2msg[x].ext == ext &&
                   (ks[keydown2msg[x].keystate]&0x80)) {
                        int repeat = LOWORD (lParam);
                        for (; repeat>0; repeat--)  {
                             win32_terminal_queuekey(globTerm,
                                                     keydown2msg[x].func_key);
                        }
                        PostMessage(hwnd, WM_TN5250_KEY_DATA, 0, 0);
                        TN5250_LOG(("WM_KEYDOWN: handling key\n"));
                        handledkey = 1;
                        return 0;
               }
               x++;
           } 
           /* if we didn't handle a keystroke, let Windows send it
              back to us as a character message */
           m.hwnd = hwnd;
           m.message = msg;
           m.wParam = wParam;
           m.lParam = lParam;
           m.time = GetMessageTime();
           x = GetMessagePos();
           memcpy(&(m.pt), &x, sizeof(m.pt));
           TranslateMessage(&m);
           break;

        case WM_CHAR:
           x=0;
           while (win_kb[x].win32_key != -1) {
                if (wParam == win_kb[x].win32_key) {
                    wParam = win_kb[x].tn5250_key;
                    break;
                }
                x++;
           }
           handledkey = 1;
           TN5250_LOG(("WM_CHAR: handling key\n"));
           win32_terminal_queuekey(globTerm,(int)wParam);
           break;

        case WM_TN5250_KEY_DATA:
        case WM_TN5250_STREAM_DATA:
           /* somewhere in this program we're signalling waitevent()
              to process an event. */
           return 0;
           break;

        case WM_ERASEBKGND:
           /* don't let background get erased, that causes "flashing" */
           return 0;
           break;

        case WM_SETFOCUS:
           win32_make_new_caret(globTerm);
           hdc = GetDC(hwnd);
           globTerm->data->caretok = 0;
           win32_move_caret(hdc, globTerm);
           ReleaseDC(hwnd, hdc);
#ifndef NOBLINK
           ShowCaret(hwnd);
#endif
           globTerm->data->is_focused = 1;
           return 0;
  
        case WM_KILLFOCUS:
           win32_hide_caret(hdc, globTerm);
           globTerm->data->is_focused = 0;
           return 0;

        case WM_PAINT: 
           hdc = BeginPaint (hwnd, &ps); 
           GetClientRect(hwnd, &rect);
           x = rect.left;
           y = rect.top;
           h = (rect.bottom - rect.top) + 1;
           w = (rect.right - rect.left) + 1;
           SelectObject(bmphdc, screenbuf);
#ifdef NOBLINK
           if (hwnd == GetFocus()) {
               win32_move_caret(bmphdc, globTerm);
           }
#endif
           if (BitBlt(hdc, x, y, w, h, bmphdc, x, y, SRCCOPY)==0) {
                TN5250_LOG(("WIN32: BitBlt failed: %d\n", GetLastError()));
                TN5250_ASSERT(0);
           }
#ifndef NOBLINK
           if (hwnd == GetFocus()) {
               win32_move_caret(hdc, globTerm);
           }
#endif
           if (globTerm->data->selected) {
                SetROP2(hdc, R2_NOT);
                if (globTerm->data->selecting)
                     SelectObject(hdc, GetStockObject(NULL_BRUSH) );
                else
                     SelectObject(hdc, GetStockObject(WHITE_BRUSH) );
                Rectangle(hdc, globTerm->data->selstr.x, 
                               globTerm->data->selstr.y,
                               globTerm->data->selend.x, 
                               globTerm->data->selend.y);
           }
           EndPaint (hwnd, &ps);
           return 0;
           break;

        case WM_RBUTTONDOWN:
           if (globTerm->data->unix_like_copy) {
                win32_paste_text_selection(hwnd, globTerm, globDisplay);
                return 0;
           }
           break;

        case WM_LBUTTONDOWN:
           if (globDisplay != NULL) {
                globTerm->data->selecting = 1;
                globTerm->data->selstr.x = (short)LOWORD(lParam);
                globTerm->data->selstr.y = (short)HIWORD(lParam);
                SetCapture(hwnd);
                SetCursor (LoadCursor (NULL, IDC_CROSS));
                if (globTerm->data->selected) {
                     globTerm->data->selected = 0;
                     InvalidateRect(hwnd, NULL, FALSE);
                     UpdateWindow(hwnd);
                }
                return 0;
           }
           break;

        case WM_MOUSEMOVE:
           if (globTerm->data->selecting && globDisplay!=NULL) {
                globTerm->data->selected = 1;
                globTerm->data->selend.x = (short)LOWORD(lParam);
                globTerm->data->selend.y = (short)HIWORD(lParam);
                SetCursor (LoadCursor (NULL, IDC_CROSS));
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
                return 0;
           }
           break;

        case WM_LBUTTONUP:
           if (globTerm->data->selecting && globDisplay!=NULL) {
                globTerm->data->selecting = 0;
                globTerm->data->selend.x = (short)LOWORD(lParam);
                globTerm->data->selend.y = (short)HIWORD(lParam);
                ReleaseCapture();
                SetCursor (LoadCursor (NULL, IDC_ARROW));
                win32_expand_text_selection(globTerm);
                if (globTerm->data->unix_like_copy) {
                     win32_copy_text_selection(globTerm, globDisplay);
                }
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
                return 0;
           }
           break;

#ifdef LOG_KEYCODES
        case WM_SYSCOMMAND:
           x = LOWORD(lParam);
           y = HIWORD(lParam);
           TN5250_LOG(("WIN32: WM_SYSCOMMAND: (%d,%d) %d\n", x, y, wParam));
           break;

        case WM_DEADCHAR:
        case WM_SYSDEADCHAR:
           TN5250_LOG(("WIN32: WM_DEADCHAR: Dead character %d\n", wParam));
           break;

        default:
           TN5250_LOG(("WIN32: Unhandled msg %d\n", msg));
           break;
#endif

     }     

     return DefWindowProc (hwnd, msg, wParam, lParam);
}


/****i* lib5250/win32_make_new_caret
 * NAME
 *    win32_make_new_caret
 * SYNOPSIS
 *    win32_make_new_caret (globTerm);
 * INPUTS
 *    Tn5250Terminal  *          This -
 * DESCRIPTION
 *    If you're wondering, "Caret" is the Windows term for
 *    the cursor used when typing at the keyboard.  In Windows
 *    terminology, cursor=mouse, caret=keyboard.
 *
 *    There is only one cursor in Windows, shared by all Windows apps.
 *    So, we create it when focus returns to us, and destroy it when
 *    we lose focus.  This is where it gets created.
 *****/
void win32_make_new_caret(Tn5250Terminal *This) {
#ifdef NOBLINK
    /* We make the Windows Caret invisible, so we can maintain control
       of the caret without the user seeing it blink */
    {
    unsigned char *bits;
    int size, bytewidth;
    HPEN savepen;
    HDC hdc;

    bytewidth = (This->data->font_width + 15) / 16 * 2;
    size = bytewidth * This->data->font_height;
    bits = g_malloc(size);
    memset(bits, 0x00, size);
    caretbm = CreateBitmap(This->data->font_width, 
                   This->data->font_height, 1, 1, bits);
    g_free(bits);
    CreateCaret(This->data->hwndMain, caretbm, 
         This->data->font_height, This->data->font_width);
    }
#endif
#ifdef LINE_CURSOR
    /* Here we create a small bitmap to use as the caret
       we simply draw a line at the bottom of the bitmap */
    {
    unsigned char *bits;
    int size, bytewidth;
    HPEN savepen;
    HDC hdc;

    bytewidth = (This->data->font_width + 15) / 16 * 2;
    size = bytewidth * This->data->font_height;
    bits = g_malloc(size);
    memset(bits, 0x00, size);
    caretbm = CreateBitmap(This->data->font_width, 
                   This->data->font_height, 1, 1, bits);
    g_free(bits);
    hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, caretbm);
    savepen = SelectObject(hdc, 
               CreatePen(PS_SOLID, 0, A_5250_WHITE));
    MoveToEx(hdc, 0, This->data->font_height-2, NULL);
    LineTo(hdc, This->data->font_width, This->data->font_height-2);
    savepen = SelectObject(hdc, savepen);
    DeleteObject(savepen);
    DeleteDC(hdc);
    CreateCaret(This->data->hwndMain, caretbm, 
         This->data->font_height, This->data->font_width);
    }
#else
    /* for the standard "blinking block", we just use the windows default
       shape for the caret */
    CreateCaret(This->data->hwndMain, NULL, This->data->font_width,
         This->data->font_height);
#endif
}

/****i* lib5250/win32_move_caret
 * NAME
 *    win32_move_caret
 * SYNOPSIS
 *    win32_move_caret (globTerm);
 * INPUTS
 *    Tn5250Terminal    *         This -
 * DESCRIPTION
 *    Move the caret to a position on the screen.
 *    to the coordinates in This->data->caretx, This->data->carety
 *****/
 
void win32_move_caret(HDC hdc, Tn5250Terminal *This) {

    /* move the Windows caret */

    SetCaretPos(This->data->caretx, This->data->carety);

#ifdef NOBLINK
    /* Since the Windows caret is invisible, make our own box now */
    if (! This->data->caretok) 
    {
       HPEN savepen;
       HBRUSH savebrush;
       int savemode;
       savepen = SelectObject(hdc, GetStockObject(WHITE_PEN));
       savebrush = SelectObject(hdc, GetStockObject(WHITE_BRUSH));
       savemode = SetROP2(hdc, R2_NOT);
       Rectangle(hdc, This->data->caretx, This->data->carety,
                      (This->data->caretx + This->data->font_width),
                      (This->data->carety + This->data->font_height));
       SetROP2(hdc, savemode);
       SelectObject(hdc, savepen);
       SelectObject(hdc, savebrush);
       This->data->caretok = 1;
    }
#endif
    return;
}


/****i* lib5250/win32_hide_caret
 * NAME
 *    win32_hide_caret
 * SYNOPSIS
 *    win32_hide_caret (globTerm);
 * INPUTS
 *    Tn5250Terminal    *         This -
 * DESCRIPTION
 *    Hide the caret, usually done when the window loses focus
 *****/
void win32_hide_caret(HDC hdc, Tn5250Terminal *This) {

    HideCaret (This->data->hwndMain);
    DestroyCaret ();
    
#ifdef NOBLINK
    {
       HPEN savepen;
       HBRUSH savebrush;
       int savemode;

     /* save the drawing parms */

       savepen = SelectObject(hdc, GetStockObject(WHITE_PEN));
       savebrush = SelectObject(hdc, GetStockObject(WHITE_BRUSH));
       savemode = SetROP2(hdc, R2_NOT);

     /* if cursor is currently there, undraw it. */

       if (This->data->caretok) {
          Rectangle(hdc, This->data->caretx, This->data->carety,
                      (This->data->caretx + This->data->font_width),
                      (This->data->carety + This->data->font_height));
       }

      
     /* draw a hollow box around the cursor position */

       SelectObject(hdc, GetStockObject(WHITE_PEN));
       SelectObject(hdc, GetStockObject(NULL_BRUSH));
       SetROP2(hdc, R2_COPYPEN);
       Rectangle(hdc, This->data->caretx, This->data->carety,
                      (This->data->caretx + This->data->font_width),
                      (This->data->carety + This->data->font_height));

    /* restore the drawing parms */

       SetROP2(hdc, savemode);
       SelectObject(hdc, savepen);
       SelectObject(hdc, savebrush);
       This->data->caretok = 1;
       InvalidateRect(This->data->hwndMain, NULL, FALSE);
    }
#endif

    return;
}


/****i* lib5250/win32_expand_text_selection
 * NAME
 *    win32_expand_text_selection
 * SYNOPSIS
 *    win32_expand_text_selection (globTerm);
 * INPUTS
 *    Tn5250Terminal  *          This    -
 * DESCRIPTION
 *    This converts the mouse selection points (defined by
 *    This->data->selstr & This->data->selend) to a rectangle of
 *    selected text (by aligning the points with the text start/end pos)
 *****/
void win32_expand_text_selection(Tn5250Terminal *This) {

      RECT cr;
      int cx, cy;
      int x;

      TN5250_ASSERT(This!=NULL);
      TN5250_ASSERT(This->data->font_width>0);

   /* change the points so that selstr is the upper left corner 
      and selend is the lower right corner.                      */

#define TN5250_FLIPEM(a, b)  if (a>b) { x = a; a = b; b = x; }
      TN5250_FLIPEM(This->data->selstr.x, This->data->selend.x)
      TN5250_FLIPEM(This->data->selstr.y, This->data->selend.y)
#undef TN5250_FLIPEM

   /* constrain the coordinates to the window's client area */

      GetClientRect(This->data->hwndMain, &cr);

      if (This->data->selend.x < cr.left) This->data->selend.x = cr.left;
      if (This->data->selend.x > cr.right) This->data->selend.x = cr.right;
      if (This->data->selend.y < cr.top) This->data->selend.y = cr.top;
      if (This->data->selend.y > cr.bottom) This->data->selend.y = cr.bottom;

      if (This->data->selstr.x < cr.left) This->data->selstr.x = cr.left;
      if (This->data->selstr.x > cr.right) This->data->selstr.x = cr.right;
      if (This->data->selstr.y < cr.top) This->data->selstr.y = cr.top;
      if (This->data->selstr.y > cr.bottom) This->data->selstr.y = cr.bottom;


   /* move selection start position to nearest character */

      cx = This->data->selstr.x / This->data->font_width;
      This->data->selstr.x = cx * This->data->font_width;
      cy = This->data->selstr.y / This->data->font_height;
      This->data->selstr.y = cy * This->data->font_height;


   /* move selection end position to nearest character */

      cx = This->data->selend.x / This->data->font_width;
      if (This->data->selend.x % This->data->font_width) 
          cx++;
      This->data->selend.x = cx * This->data->font_width;
      cy = This->data->selend.y / This->data->font_height;
      if (This->data->selend.y % This->data->font_height)
          cy++;
      This->data->selend.y = cy * This->data->font_height;

}


/****i* lib5250/win32_copy_text_selection
 * NAME
 *    win32_copy_text_selection
 * SYNOPSIS
 *    win32_copy_text_selection (globTerm, globDisplay);
 * INPUTS
 *    Tn5250Terminal  *          This    -
 *    Tn5250Display   *          display -
 * DESCRIPTION
 *    This retrieves the text in the selected area and copies it to
 *    the global Windows clipboard.
 *****/
void win32_copy_text_selection(Tn5250Terminal *This, Tn5250Display *display)
{
      int x, y;
      int cx, cy;
      int sx, sy, ex, ey;
      unsigned char c;
      unsigned char *buf;
      int bp;
      int bufsize;
      HGLOBAL hBuf;
      HBITMAP hBm;
      HDC hdc;

      TN5250_ASSERT(This!=NULL);
      TN5250_ASSERT(display!=NULL);
      TN5250_ASSERT(This->data->font_width>0);

   /* figure out the dimensions (in text chars) and make a global buffer */

      sx = This->data->selstr.x / This->data->font_width;
      ex = This->data->selend.x / This->data->font_width;
      sy = This->data->selstr.y / This->data->font_height;
      ey = This->data->selend.y / This->data->font_height;

      while (ey>tn5250_display_height(display)) ey--;

      bufsize = ((ex-sx)+1) * ((ey-sy)+1) + 1;
      hBuf = GlobalAlloc(GHND|GMEM_SHARE, bufsize);
      TN5250_ASSERT(hBuf!=NULL);


   /* populate the global buffer with the text data, inserting CR/LF 
      in between each line that was selected */

      buf = GlobalLock(hBuf); 
      bp = -1;
      for (y = sy; y < ey; y++) {

           for (x = sx; x < ex; x++) {
                c = tn5250_display_char_at(display, y, x);
	        if (((c & 0xe0) == 0x20 )||(c < 0x40 && c > 0x00)||(c == 0xff)) 
                     c = ' ';
                else 
                     c = tn5250_char_map_to_local( 
                                tn5250_display_char_map(display), c);
                bp++;
                if (bp==bufsize) break;
                buf[bp] = c;
           }

           if (y != (ey-1)) {
                bp++;
                if (bp==bufsize) break;
                buf[bp] = '\r';
                bp++;
                if (bp==bufsize) break;
                buf[bp] = '\n';
           }

      }
      GlobalUnlock(hBuf);

      /* create a bitmap version of the copy buffer as well... 
         this allows image programs to paste the buffer as a bitmap */

      cx = (This->data->selend.x - This->data->selstr.x) + 1; 
      cy = (This->data->selend.y - This->data->selstr.y) + 1;

      hdc = GetDC(This->data->hwndMain);
      hBm = CreateCompatibleBitmap(hdc, cx, cy);
      ReleaseDC(This->data->hwndMain, hdc);
      hdc = CreateCompatibleDC(NULL);
      SelectObject(hdc, hBm);
      BitBlt(hdc, 0, 0, cx, cy, bmphdc, This->data->selstr.x, 
              This->data->selstr.y, SRCCOPY);
      DeleteDC(hdc);
      

      /* finally, copy both the global buffer & the bitmap to the 
         clipboard.  After this, we should not try to use (or free!)
         the buffer/bitmap... they're Windows' property now! 
       */

      OpenClipboard (This->data->hwndMain);
      EmptyClipboard ();

      switch (This->data->copymode) {
          case 1:  /* plain text only */
             SetClipboardData(CF_TEXT, hBuf);
             DeleteObject(hBm);
             break;
          case 2:  /* bitmap only */
             SetClipboardData(CF_BITMAP, hBm);
             GlobalFree(hBuf);
             break;
          default:
             SetClipboardData(CF_TEXT, hBuf);
             SetClipboardData(CF_BITMAP, hBm);
             break;
      }
             
      CloseClipboard ();
}


/****i* lib5250/win32_paste_text_selection
 * NAME
 *    win32_paste_text_selection
 * SYNOPSIS
 *    win32_paste_text_selection (globTerm, globDisplay);
 * INPUTS
 *    Tn5250Terminal  *          This    -
 *    Tn5250Display   *          display -
 * DESCRIPTION
 *    Convert data in the windows clipboard into keystrokes
 *    and paste them into the keyboard buffer.
 *    Note: Increasing the MAX_K_BUF_LEN will speed this
 *           routine up...
 *****/
void win32_paste_text_selection(HWND hwnd, Tn5250Terminal *term, 
                                           Tn5250Display *display) {

    HGLOBAL hBuf;
    unsigned char *pBuf;
    unsigned char *pNewBuf;
    int size, pos;
    int thisrow;

    pNewBuf = NULL;

    /* 
     * If there's any data that we can paste, read it from
     *  the clipboard. 
     */
  
    if (IsClipboardFormatAvailable(CF_TEXT)) {

        OpenClipboard(hwnd);

        hBuf = GetClipboardData(CF_TEXT);

        if (hBuf != NULL) {
            size = GlobalSize(hBuf);
            pNewBuf = g_malloc(size);
            pBuf = GlobalLock(hBuf);
            strncpy(pNewBuf, pBuf, size);
            pNewBuf[size] = '\0'; /* just a precaution */
            GlobalUnlock(hBuf);
        }

        CloseClipboard();

    }

    /*
     *  convert text data into keyboard messages,  just as if someone
     *  was typing this data at the keyboard.
     */

    if (pNewBuf != NULL) {
        int dump_count = MAX_K_BUF_LEN;
        size = strlen(pNewBuf);
        thisrow = 0;
        for (pos=0; pos<size; pos++) {
            switch (pNewBuf[pos]) {
               case '\r':
                 while (thisrow > 0)  {
                    win32_terminal_queuekey(term, K_LEFT);
                    thisrow --;
                    if (term->data->k_buf_len == dump_count) {
                         tn5250_display_do_keys(display);
                    }
                 }
                 break;
               case '\n':
                 thisrow = 0;
                 win32_terminal_queuekey(term, K_DOWN);
                 tn5250_display_do_keys(display);
                 break;
               default:
                 thisrow++;
                 win32_terminal_queuekey(term, pNewBuf[pos]);
                 break;
            }
            if (term->data->k_buf_len == dump_count) {
                 tn5250_display_do_keys(display);
            }
        }
        g_free(pNewBuf);
        PostMessage(hwnd, WM_TN5250_KEY_DATA, 0, 0);
    }

}
