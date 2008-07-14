#include "defs.h"
#include "gui.h"
#include "misc.h"
#include "tokens.h"
#include "songdata/songdata.h"
#include "controller/controller.h"

#include "window_playback.h"
#include "window_play.h"
#include "window_menubar.h"
#include "window_files.h"
#include "window_info.h"


#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>

static u_char	*parse_title ( Window *, u_char *, int );
static void init_info ( Window * );

Config * conf;

Window * playback;
Window * menubar;
Window * files;
Window * info;
Window * play;
Window * active;

/* colors */
u_int32_t * colors;

int
show_list ( Window *window )
{
	int x = window->width-4, y = window->height-2, i;
	u_char buf[BUFFER_SIZE+1];
	const u_char *line;
	u_int32_t color;
	WINDOW *win = window->win;
	flist *ftmp;
	wlist *list = window->contents.list;

	if ( !list )
		return 0;

	// do we need to reposition the screen
	if ( list->length > y-1 )
	{
		if ( ( list->where - 4 ) == list->wheretop )
		{
			list->wheretop--;
			if ( list->wheretop < 0 )
				list->wheretop = 0;
		}
		else if ( ( list->where + 3 ) == ( list->wheretop + y ) )
		{
			if ( ++list->wheretop > ( list->length - y ) )
				list->wheretop = list->length - y;
		}
		else if ( ( ( list->where - 5 ) < list->wheretop ) || ( ( list->where + 3 ) > ( list->wheretop + y ) ) )
		{
			list->wheretop = list->where - ( y/2 );
			if ( list->wheretop <  0 )
				list->wheretop = 0;
			if ( list->wheretop > ( list->length - y ) )
				list->wheretop = list->length - y;
		}
	}
	// find the list->top entry
	list->top=list->head;
	if ( ( list->wheretop > 0 ) & ( list->length > y ) )
	{
		for ( i = 0; ( i < list->wheretop ) && ( list->top->next ); i++ )
			list->top = list->top->next;
	}

	ftmp = list->top;
	for ( i = 0; i < y; i++ )
		if ( ftmp )
		{
			if ( window->format )
			{
				memset ( buf, 0, sizeof ( buf ) );
				line = parse_tokens ( window,ftmp, buf, BUFFER_SIZE, window->format );
			}
			else
				line = ftmp->filename;
			if ( window == play )
			{
				if ( ftmp == list->playing )
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[PLAY_SELECTED_PLAYING];
					else
						color = colors[PLAY_UNSELECTED_PLAYING];
				else
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[PLAY_SELECTED];
					else
						color = colors[PLAY_UNSELECTED];
			}
			else	// window==files
				if ( ftmp->flags & ( F_DIR | F_PLAYLIST ) )
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[FILE_SELECTED_DIRECTORY];
					else
						color = colors[FILE_UNSELECTED_DIRECTORY];
				else
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[FILE_SELECTED];
					else
						color = colors[FILE_UNSELECTED];
			if ( ftmp->flags & F_PAUSED )
				color |=  A_BLINK;

			/* BUG gevonden in OWEE tijdelijke fix
				if ((ftmp == list->selected) && (window->flags & W_ACTIVE))
			{
				list->startposSelected = my_mvwnaddstr(win, (i+1), 2, color, x, line, list->startposSelected);
			}else{
				my_mvwnaddstr(win, (i+1), 2, color, x, line, 0);
			}*/
			my_mvwnaddstr ( win, ( i+1 ), 2, color, x, line, 0 );

			ftmp = ftmp->next;
		}
		else  /* blank the line */
			if ( window == play )
				my_mvwnaddstr ( win, ( i+1 ), 2, colors[PLAY_WINDOW], x, "", 0 );
			else
				my_mvwnaddstr ( win, ( i+1 ), 2, colors[FILE_WINDOW], x, "", 0 );

	if ( window->flags & W_LIST )
		do_scrollbar ( window );
	update_title ( window );
	update_panels();
	if ( conf->c_flags & C_FIX_BORDERS )
		redrawwin ( win );
	return 1;
}

Window * move_files_selector ( int c )
{
	return move_selector ( files, c );
}

Window *
move_selector ( Window *window, int c )
{
	flist *file;
	wlist *list = window->contents.list;
	int j, maxx, maxy, length;

	if ( !list )
		return NULL;
	if ( !list->selected )
		return NULL;

	//BUG gevonden in owee: list->startposSelected = 0; //Reset the scrolling pos in the flist

	getmaxyx ( window->win, maxy, maxx );
	length = maxy - 1;

	switch ( c )
	{
		case KEY_ENTER:
		case '\n':
		case '\r':
			if ( ( window == play ) && ( list->playing ) )
			{
				list->selected = next_valid ( list, list->head, KEY_HOME );
				list->where = 1;
				list->wheretop = 0;

				for ( j = 0; list->selected->next && list->selected != list->playing; j++ )
				{
					list->selected = next_valid ( list, list->selected->next, KEY_DOWN );
					list->where++;
				}
				return window;
			}
			break;
		case KEY_HOME:
			list->selected = next_valid ( list, list->head, c );
			list->where = 1;
			list->wheretop = 0;
			return window;
		case KEY_END:
			list->selected = next_valid ( list, list->tail, c );
			list->where = list->length;
			return window;
		case KEY_DOWN:
			if ( ( file = next_valid ( list, list->selected->next, c ) ) )
			{
				list->selected = file;
				list->where++;
				return window;
			}
			break;
		case KEY_UP:
			if ( ( file = next_valid ( list, list->selected->prev, c ) ) )
			{
				list->selected = file;
				list->where--;
				return window;
			}
			break;
		case KEY_NPAGE:
			for ( j = 0; list->selected->next && j < length-1; j++ )
			{
				list->selected = next_valid ( list, list->selected->next, KEY_DOWN );
				list->where++;
			}
			list->selected = next_valid ( list, list->selected, c );
			return window;
		case KEY_PPAGE:
			for ( j = 0; list->selected->prev && j < length-1; j++ )
			{
				list->selected = next_valid ( list, list->selected->prev, KEY_UP );
				list->where--;
			}
			list->selected = next_valid ( list, list->selected, c );
			return window;
		default:
			break;
	}
	return NULL;
}

int
update_info ( Window *window )
{
	WINDOW *win = window->win;
	int i = window->width;
	flist *file = NULL;

	file = *window->contents.show;

	clear_info ( window );

	if ( file )
	{
		if ( file->flags & F_DIR )
			my_mvwnprintw ( win, 1 + window->yoffset, 10, colors[INFO_TEXT], i-11, "%s", "(Directory)" );
		else
		{
			my_mvwnprintw ( win, 1 + window->yoffset, 10, colors[INFO_TEXT], i-11, "%s", file->title );

			if ( file->artist )
				my_mvwnprintw ( win, 2 + window->yoffset, 10, colors[INFO_TEXT], i-11, "%s", file->artist );
			else
				my_mvwnprintw ( win, 2 + window->yoffset, 10, colors[INFO_TEXT], i-11, "Unknown" );

			if ( file->album )
				my_mvwnprintw ( win, 3 + window->yoffset, 10, colors[INFO_TEXT], i-11, "%s", file->album );
			else
				my_mvwnprintw ( win, 3 + window->yoffset, 10, colors[INFO_TEXT], i-11, "Unknown" );
			if ( conf->c_flags & C_USE_GENRE )
			{
				if ( file->genre )
					my_mvwnprintw ( win, 4 + window->yoffset, 10, colors[INFO_TEXT], i-11, "%s", file->genre );
				else
					my_mvwnprintw ( win, 4 + window->yoffset, 10, colors[INFO_TEXT], i-11, "Unknown" );
			}

		}
	}
	update_title ( window );
	update_panels();
	return 1;
}

void change_active ( int next )
{
	Window * new;
	if ( next )
	{
		if ( active->next == NULL )
		{
			return;
		}
		new = active->next;
	}
	else
	{
		if ( active->prev == NULL )
		{
			return;
		}
		new = active->prev;
	}
	active->deactivate ( active );
	active->update ( active );
	active = new;
	active->activate ( active );
	active->update ( active );
	info->contents.show = &active->contents.list->selected;
	info->update ( info );
	doupdate();
}

__inline__ void
clear_info ( Window * win )
{
	int i = win->height - 2;
	for ( ; i; i-- )
		my_mvwnclear ( win->win, i, 9, win->width - 10 );
	update_panels();
	if ( conf->c_flags & C_FIX_BORDERS )
		redrawwin ( win->win );
}

int
active_win ( Window *window )
{
	WINDOW *win = window->win;
	PANEL *panel = window->panel;
	wborder ( win, ACS_VLINE | colors[WIN_ACTIVE], ACS_VLINE | colors[WIN_ACTIVE], ACS_HLINE | colors[WIN_ACTIVE], ACS_HLINE | colors[WIN_ACTIVE], ACS_ULCORNER | colors[WIN_ACTIVE], ACS_URCORNER | colors[WIN_ACTIVE], ACS_LLCORNER | colors[WIN_ACTIVE], ACS_LRCORNER | colors[WIN_ACTIVE] );
	window->flags |= W_ACTIVE;
	update_title ( window );
	top_panel ( panel );
	update_panels();
	return 1;
}

int
inactive_win ( Window *window )
{
	WINDOW *win = window->win;
	wborder ( win, ACS_VLINE | colors[WIN_INACTIVE], ACS_VLINE | colors[WIN_INACTIVE], ACS_HLINE | colors[WIN_INACTIVE], ACS_HLINE | colors[WIN_INACTIVE], ACS_ULCORNER | colors[WIN_INACTIVE], ACS_URCORNER | colors[WIN_INACTIVE], ACS_LLCORNER | colors[WIN_INACTIVE], ACS_LRCORNER | colors[WIN_INACTIVE] );
	window->flags &= ~W_ACTIVE;
	update_title ( window );
	update_panels();
	return 1;
}

int
clear_menubar ( Window *window )
{
	wmove ( window->win, 0, 0 );
	wclrtoeol ( window->win );
	wbkgd ( window->win, colors[MENU_WINDOW] );
	return 1;
}

int
std_menubar ( Window *window )
{
	char version_str[128];
	struct tm * currentTime;
	time_t now2;
	int x = window->width-2;
	now2 = time ( NULL );
	currentTime = localtime ( &now2 );
	clear_menubar ( window );
	my_mvwaddstr ( window->win, 0, ( ( x-strlen ( window->title_dfl ) ) /2 ), colors[MENU_TEXT], window->title_dfl );
	snprintf ( version_str, 128, "%.2d-%.2d-%.4d %.2d:%.2d v%s", currentTime->tm_mday, currentTime->tm_mon + 1, currentTime->tm_year + 1900, currentTime->tm_hour - currentTime->tm_isdst, currentTime->tm_min,  "4.0" /* TODO: VERSION*/ );
	my_mvwaddstr ( window->win, 0, x - strlen ( version_str ) + 2, colors[MENU_TEXT], version_str );

	update_panels();
	if ( conf->c_flags & C_FIX_BORDERS )
		redrawwin ( window->win );
	doupdate();
	return 1;
}

__inline__ void
printf_menubar ( char *text )
{
	clear_menubar ( menubar );
	my_mvwaddstr ( menubar->win, 0, 10, colors[MENU_TEXT], text );
	update_panels();
	doupdate();

}

int
update_title ( Window *window )
{
	WINDOW *win = window->win;
	u_char title[BUFFER_SIZE+1], *p = NULL, horiz = ACS_HLINE;
	u_int32_t color;
	int i = 0, left, right, center, x = window->width-2;

	if ( window->flags & W_ACTIVE )
	{
		if ( conf->c_flags & C_FIX_BORDERS )
			horiz = '=', color = colors[WIN_ACTIVE];
		else
			horiz = 'i', color = colors[WIN_ACTIVE];
	}
	else
		color = colors[WIN_INACTIVE];

	memset ( title, 0, sizeof ( title ) );
	p = parse_title ( window, title, BUFFER_SIZE );
	i = strlen ( p ) +4;

	if ( ( i > x ) | ( i <= 4 ) )
	{
		if ( ( i = strlen ( window->title_dfl ) ) < x )
		{
			i += 4;
			p = ( u_char * ) window->title_dfl;
		}
		else
		{
			p = "";
			i = 4;
		}
	}

	center = x-i- ( x-i ) /2;
	right = ( x-i ) /2;
	left = x-i-right;

	if ( p && ( i <= x ) )
	{
		mvwhline ( win, 0, 1,  ACS_HLINE | color, left );
		if ( window->flags & W_ACTIVE )
			my_mvwprintw ( win, 0, 1+center, colors[WIN_ACTIVE_TITLE], "[ %s ]", p );
		else
			my_mvwprintw ( win, 0, 1+center, colors[WIN_INACTIVE_TITLE], "[ %s ]", p );
		mvwhline ( win, 0, 1+center+i, ACS_HLINE | color, right );
		return 1;
	}
	return 0;
}

void
do_scrollbar ( Window *window )
{
	int x, y; /* window dimensions, etc */
	int top, bar, bottom; /* scrollbar portions */
	double value; /* how much each notch represents */
	u_int32_t color, barcolor;
	wlist *list = window->contents.list;
	WINDOW *win = window->win;

	getmaxyx ( win, y, x );
	y -= 2;
	x -= 1;

	if ( list->length < y + 1 )
	{
		if ( window->flags & W_ACTIVE )
			active_win ( window );
		else
			inactive_win ( window );
//		update_panels();
		return;
	}

	value = list->length / ( double ) y;

	/* calculate the sizes of our status bar */
	if ( value <= 1 )
	{
		// only one screen
		top = 0;
		bar = y;
		bottom = 0;
	}
	else
	{
		top = ( int ) ( ( ( double ) ( list->wheretop ) / value ) + ( double ).5 );
		bar = ( int ) ( ( y / value ) + ( double ).5 );
		if ( bar<1 )
			bar = 1;
		bottom = y - top - bar ;
	}

	/* because of rounding we may end up with too much, correct for that */
	if ( bottom < 0 )
		top += bottom;

	if ( window->flags & W_ACTIVE )
	{
		color = colors[WIN_ACTIVE_SCROLL];
		barcolor = colors[WIN_ACTIVE_SCROLLBAR];
	}
	else
	{
		color = colors[WIN_INACTIVE_SCROLL];
		barcolor = colors[WIN_INACTIVE_SCROLLBAR];
	}

	if ( conf->c_flags & C_FIX_BORDERS )
	{
		mvwvline ( win, 1, x, ACS_CKBOARD | A_ALTCHARSET | color, top );
		mvwvline ( win, 1 + top, x, ACS_CKBOARD | A_ALTCHARSET | barcolor, bar );
		if ( bottom > 0 )
			mvwvline ( win, 1 + top + bar, x, ACS_CKBOARD | A_ALTCHARSET | color, bottom );
	}
	else
	{
		mvwvline ( win, 1, x, ACS_BOARD | A_ALTCHARSET | color, top );
		mvwvline ( win, 1 + top, x, ACS_BLOCK | A_ALTCHARSET | barcolor, bar );
		if ( bottom > 0 )
			mvwvline ( win, 1 + top + bar, x, ACS_BOARD | A_ALTCHARSET | color, bottom );
	}
//	update_panels();
}

u_char *
parse_title ( Window *win, u_char *title, int len )
{
	u_char *p = ( u_char * ) win->title_dfl;

	if ( win->title_fmt )
	{
		if ( win->flags & W_LIST && win->contents.list && win->contents.list->selected )
			p = ( u_char * ) parse_tokens ( win, win->contents.list->selected, title, len, win->title_fmt );
		else if ( ! ( win->flags & W_LIST ) && ( win==info ) )
			p = ( u_char * ) parse_tokens ( win, *win->contents.show, title, len, win->title_fmt );
//		else if (info->contents.show)
//			p = (u_char *)parse_tokens(win, *info->contents.show, title, len, win->title_fmt);
	}

	return p;
}


void gui_init ( Config * init_conf,   u_int32_t init_colors[], wlist * mp3list, wlist * playlist )
{

	conf = init_conf;
	colors = init_colors;

	curs_set ( 0 );
	leaveok ( stdscr, TRUE );
	cbreak ();
	noecho ();
	use_default_colors ();
	start_color ();
	nonl ();
	init_ansi_pair ();
	wbkgd ( stdscr, colors[FILE_WINDOW] );

	/*
	* malloc() for the windows and set up our initial callbacks ...
	*/

	info = window_info_init ( conf );
	play = window_play_init ( conf );
	playback = window_playback_init ( conf );
	menubar = window_menubar_init ( conf );
	files = window_files_init ( conf );
	active = files;


	files->prev = play;
	files->next = play;
	info->prev = files;
	info->next = play;
	play->prev = files;
	play->next = files;

	/* Eigenlijk nog verplaatsen */
	files->contents.list = mp3list;
	info->contents.show = &mp3list->selected;
	play->contents.list = playlist;

	/*
	* reading the config must take place AFTER initscr() and friends
	*/

	if ( !files->win || !info->win || !play->win || !menubar->win )
		bailout ( 0 );

	menubar->activate ( menubar );
	init_info ( info );
	init_info ( playback );
	play->deactivate ( play );

	info->deactivate ( info );
	active->activate ( active );
	playback->deactivate ( playback );

	play->update ( play );
	files->update ( files );
	info->update ( info );
	show_playinfo();
	doupdate ();

}

static void
init_info ( Window * window )
{
	WINDOW * win = window->win;
	my_mvwaddstr ( win, 1 + window->yoffset, 2, colors[INFO_TEXT], "Title :" );
	my_mvwaddstr ( win, 2 + window->yoffset, 2, colors[INFO_TEXT], "Artist:" );
	my_mvwaddstr ( win, 3 + window->yoffset, 2, colors[INFO_TEXT], "Album :" );
	if ( conf->c_flags & C_USE_GENRE )
		my_mvwaddstr ( win, 4 + window->yoffset, 2, colors[INFO_TEXT], "Genre :" );

	update_panels ();
	doupdate ();
}

int
update_menu ( Input * inputline )
{
	clear_menubar ( menubar );
	update_anchor ( inputline );
	my_wnprintw ( inputline->win, colors[MENU_TEXT] | A_BLINK, inputline->flen + inputline->plen, "%s", inputline->prompt );
	my_wnprintw ( inputline->win, colors[MENU_TEXT], inputline->flen + inputline->plen, " %s", inputline->anchor );
	update_panels ();
	doupdate ();
	return 1;
}

void
show_playinfo ( void )
{
	int elapsed, remaining, length;
	remaining = engine_get_remaining();
	elapsed = engine_get_elapsed();
	length = engine_get_length();


	playback->contents.show = &play->contents.list->playing;
	playback->update ( playback );

	my_mvwnprintw2 ( playback->win, 1, 1, colors[PLAYBACK_TEXT], 35, " Time  : %02d:%02d / %02d:%02d (%02d:%02d)",
	                 ( int ) elapsed / 60, ( ( int ) elapsed ) % 60, ( int ) remaining / 60, ( ( int ) remaining ) % 60, ( int ) ( length ) / 60, ( int ) ( length ) % 60 );
	update_panels();
	doupdate ();
}

void gui_shutdown ( void )
{
	window_files_shutdown();
	window_info_shutdown();
	window_menubar_shutdown();
	window_playback_shutdown();
	window_play_shutdown();
}


void poll_keyboard ( void )
{
	active->input ( active );
}