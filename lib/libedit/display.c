/* display.c - display an Edit_buffer in an abstract rendering area */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)display.c	1.10 09 Sep 1994 (UKC) */
char edit_display_c_rcsid[] = "$Id$";

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <local/ukcprog.h>
#include <local/wn.h>

#include "edit.h"
#include "textbuf.h"
#include "display.h"
#include "props.h"

struct Edit_history
{
    int size;
    int insert_point;
    char** text_lines;
};


static void adjust_for_delete PROTO((size_t pos, size_t len, size_t *p_point));
static size_t truncate_point_to_buflen PROTO((Edit_buffer *buffer, size_t pos));
static void clear_display PROTO((Edit_display *d));
static void before_change PROTO((Edit_display *d));
static void after_change PROTO((Edit_display *d, bool track_cursor));

static const char* history_state_dir;
static const char* history_state_file;
static const char* history_home_file;
static Edit_history_item* the_histories;

static Edit_history* edit_create_history_by_size PROTO((int size));
static void edit_create_history PROTO((Edit_history_item *history_item));
static FILE *open_history_file PROTO((const char *mode));
static void load_history PROTO((FILE *file, Edit_history_item *history_item));
static void save_history PROTO((FILE *file, Edit_history_item *history_item));


static Edit_display *All_displays = NULL;

void
edit_close_display(d)
Edit_display *d;
{
	Edit_display *prev, *d2;

	prev = NULL;
	
	for (d2 = All_displays; d2 != NULL; d2 = d2->next) {
		if (d2 == d)
			break;
		prev = d2;
	}

	if (d2 == NULL)
		panic("display not found in ecd");

	if (prev == NULL)
		All_displays = d->next;
	else
		prev->next = d->next;

	(*d->display_ops->destroy)(d);

	free((char *)d);
}

static void
clear_display(d)
Edit_display *d;
{
	(*d->render_ops->set_area)(d->render_data, 0, 0,
				   d->display_width, d->display_height, FALSE);
}

void
edit_redraw_display(d)
Edit_display *d;
{
	before_change(d);
	
	if (d->buffer == NULL)
		clear_display(d);
	else
		(*d->display_ops->redraw_display)(d);
	
	after_change(d, FALSE);
}

void
edit_display_from(d, pixel_offset, point)
Edit_display *d;
int pixel_offset;
size_t point;
{
	before_change(d);

	if (d->buffer == NULL) {
		clear_display(d);
	}
	else {
		point = truncate_point_to_buflen(d->buffer, point);
		(*d->display_ops->display_from)(d, pixel_offset, point);
	}

	after_change(d, FALSE);
}

void
edit_get_display_info(d, p_pixel_offset, p_start_point, p_lim_point)
Edit_display *d;
int *p_pixel_offset;
size_t *p_start_point, *p_lim_point;
{
	(*d->display_ops->get_display_info)(d, p_pixel_offset,
					    p_start_point, p_lim_point);
}

bool
edit_point_to_pixel(d, point, p_x, p_y, p_width, p_height, p_baseline)
Edit_display *d;
size_t point;
int *p_x, *p_y, *p_width, *p_height, *p_baseline;
{
	return (*d->display_ops->point_to_pixel)(d, point, p_x, p_y,
						 p_width, p_height,
						 p_baseline);
}

int
edit_scroll_display(d, delta)
Edit_display *d;
int delta;
{
	int res;

	if (delta == 0 || d->display_ops->scroll_display == NULL)
		return 0;
	
	(*d->render_ops->set_updating)(d->render_data, FALSE);
	res = (*d->display_ops->scroll_display)(d, delta);
	(*d->render_ops->set_updating)(d->render_data, TRUE);
	
	return res;
}

void edit_set_history_pointer(d, offset)
Edit_display *d;
int offset;
{
	Edit_buffer *buffer = d->buffer;
	const char* text;
	size_t point = d->point;
	size_t len;

	if (buffer == NULL) 
		return;
	

	text = edit_get_history(d->history, offset);
	if ( text)
	{
	    len = d->lim-d->start;
	    edit_delete(buffer, d->start, len);
	    len = strlen(text);
	    edit_insert(buffer, d->start, text, len, TRUE);
	    d->history_offset = offset;
	    before_change(d);
	    if ( point > d->start+len)
		point = d->start+len;
	    d->point = point;
	    after_change(d, TRUE);
	}
}

int
edit_scroll_history(d, direction)
Edit_display *d;
int direction;
{
	int offset = d->history_offset+direction;
	Edit_buffer *buffer = d->buffer;

	if (   direction == 0
	    || buffer == NULL) 
		return 0;
	edit_set_history_pointer(d, offset);
	return 1;
}

bool
edit_pixel_to_point(d, x, y, p_point)
Edit_display *d;
int x, y;
size_t *p_point;
{
	return (*d->display_ops->pixel_to_point)(d, x, y, p_point);
}

bool
edit_ensure_visible(d, point)
Edit_display *d;
size_t point;
{
	point = truncate_point_to_buflen(d->buffer, point);

	if ((*d->display_ops->visible)(d, point))
		return FALSE;
	
	before_change(d);
	(*d->display_ops->redraw_with_point_visible)(d, point);
	after_change(d, FALSE);

	return TRUE;
}

bool
edit_set_display_size(d, width, height)
Edit_display *d;
int width, height;
{
	if (!(*d->display_ops->update_display_size)(d, width, height))
		return FALSE;

	d->display_width = width;
	d->display_height = height;

	return TRUE;
}

void
edit_get_display_size(d, p_width, p_height)
Edit_display *d;
int *p_width, *p_height;
{
	*p_width = d->display_width;
	*p_height = d->display_height;
}

Edit_display *
edit_create_display(want_cursor, keep_cursor_visible, render_data, render_ops)
bool want_cursor, keep_cursor_visible;
char *render_data;
Edit_render_ops *render_ops;
{
	Edit_display *d;

	d = (Edit_display *)e_malloc(sizeof(Edit_display));
	d->keymap = edit_get_default_keymap();
	d->buffer = NULL;
	d->point = 0;
	d->mark = 0;
	d->start = 0;
	d->lim = 0;
	d->goal_column = 0;
	d->render_data = render_data;
	d->render_ops = render_ops;
	d->render_context = NULL;
	d->display_width = 0;
	d->display_height = 0;
	d->display_data = NULL;
	d->display_ops = NULL;
	d->want_cursor = want_cursor;
	d->keep_cursor_visible = keep_cursor_visible;
	d->quit_requested = FALSE;
	d->last_key = 0;
	d->last_modifiers = 0;
	d->user_data = NULL;
	d->sel_startpc = NULL;
	d->sel_endpc = NULL;
	d->history = NULL;
	d->history_offset = 0;
	d->next = All_displays;
	All_displays = d;

	return d;
}

void
edit_set_user_data(d, user_data)
Edit_display *d;
char *user_data;
{
	d->user_data = user_data;
}

char *
edit_get_user_data(d)
Edit_display *d;
{
	return d->user_data;
}

void  edit_set_history(d, h)
Edit_display *d;
Edit_history* h;
{
    if ( d->history != h )
    {
	d->history = h;
	d->history_offset = -1;
    }
}

void  edit_save_in_history(d)
Edit_display *d;
{
    if ( d->history && d->buffer)
    {
	Edit_buffer* buffer = d->buffer;
	char* text = edit_copy_bytes(buffer, d->start, d->lim, NULL);
	edit_append_history(d->history, text);
	d->history_offset = 0;
    }

}

void
edit_get_recent_history(d,count,history)
Edit_display *d;
int* count;
const char** history;
{
    int i = 0;
    const char* text;
    if ( d->history  )
    {
	while (i < *count && (text =  edit_get_history(d->history, i) ) )
	    history[i++] = text;
    }
    *count = i;
}

void
edit_get_render_data_and_ops(d, p_render_data, p_render_ops)
Edit_display *d;
char **p_render_data;
Edit_render_ops **p_render_ops;
{
	*p_render_data = d->render_data;
	*p_render_ops = d->render_ops;
}

bool
edit_insert(buffer, pos, text, len, update_proplist)
Edit_buffer *buffer;
size_t pos;
const char *text;
size_t len;
bool update_proplist;
{
	Edit_display *d;
	
	if (buffer->textbuf->ops->insert == NULL) {
		errf("Buffer is read-only");
		return FALSE;
	}
	
	for (d = All_displays; d != NULL; d = d->next) {
		if (d->buffer == buffer) {
			before_change(d);
			
			if (d->display_ops->about_to_insert != NULL)
				(*d->display_ops->about_to_insert)(d, pos, len);

			if (pos <= d->point)
				d->point += len;
			if (pos <= d->lim)
				d->lim += len;
			
			if (pos < d->mark)
				d->mark += len;
			if (pos < d->start)
				d->start += len;
		}
	}

	if (update_proplist)
	  edit__update_proplist_points(buffer->proplist, pos, (long)len);

	(*buffer->textbuf->ops->insert)(buffer->textbuf->bufdata,
					pos, text, len);
	
	for (d = All_displays; d != NULL; d = d->next) {
		if (d->buffer == buffer) {
			(*d->display_ops->done_insert)(d, pos, len);
			after_change(d, TRUE);
		}
	}

	return TRUE;
}

static void
before_change(d)
Edit_display *d;
{
	if ( d->sel_startpc)
	    wn_clr_xclipboard();
	(*d->render_ops->set_updating)(d->render_data, FALSE);

	if (d->buffer != NULL &&
	    d->want_cursor && (*d->display_ops->visible)(d, d->point)) {
		(*d->display_ops->draw_cursor)(d, d->point, FALSE);
	}
}

static void
after_change(d, track_cursor)
Edit_display *d;
bool track_cursor;
{
	if (d->buffer != NULL &&
	    track_cursor &&
	    d->want_cursor &&
	    d->keep_cursor_visible &&
	    !(*d->display_ops->visible)(d, d->point)) {
		(*d->display_ops->redraw_with_point_visible)(d, d->point);
	}

	if (d->buffer != NULL &&
	    d->want_cursor && (*d->display_ops->visible)(d, d->point)) {
		(*d->display_ops->draw_cursor)(d, d->point, TRUE);
	}
	
	(*d->render_ops->set_updating)(d->render_data, TRUE);
}

static void
adjust_for_delete(pos, len, p_point)
size_t pos, len, *p_point;
{
	if (*p_point >= pos) {
		if (*p_point < pos + len)
			*p_point = pos;
		else
			*p_point -= len;
	}
}

bool
edit_delete(buffer, pos, len)
Edit_buffer *buffer;
size_t pos, len;
{
	Edit_display *d;

	if (buffer->textbuf->ops->delete == NULL) {
		errf("Buffer is read-only");
		return FALSE;
	}
	
	for (d = All_displays; d != NULL; d = d->next) {
		if (d->buffer == buffer) {
			before_change(d);
			
			if (d->display_ops->about_to_delete != NULL)
				(*d->display_ops->about_to_delete)(d, pos, len);

			adjust_for_delete(pos, len, &d->point); 
			adjust_for_delete(pos, len, &d->mark); 
			adjust_for_delete(pos, len, &d->start); 
			adjust_for_delete(pos, len, &d->lim); 
		}
	}

	edit__update_proplist_points(buffer->proplist, pos, -(long)len);

	(*buffer->textbuf->ops->delete)(buffer->textbuf->bufdata, pos, len);
	
	for (d = All_displays; d != NULL; d = d->next) {
		if (d->buffer == buffer) {
			(*d->display_ops->done_delete)(d, pos, len);
			after_change(d, TRUE);
		}
	}

	return TRUE;
}

void
edit_set_buffer(d, buffer)
Edit_display *d;
Edit_buffer *buffer;
{
	/* If this display has the global selection, clear it
	** before changing the buffer */
	if ( d->sel_startpc)
	    wn_clr_xclipboard();

	d->buffer = buffer;
	d->point = 0;
	d->mark = 0;
	d->start = 0;
	d->lim = edit_get_buffer_length(buffer);

	(*d->display_ops->note_new_buffer)(d);
}

Edit_buffer *
edit_get_buffer(d)
Edit_display *d;
{
	return d->buffer;
}

void
edit_set_keymap(d, keymap)
Edit_display *d;
Edit_keymap *keymap;
{
	d->keymap = keymap;
}

Edit_keymap *
edit_get_keymap(d)
Edit_display *d;
{
	return d->keymap;
}

size_t
edit_set_point(d, point)
Edit_display *d;
size_t point;
{
	point = truncate_point_to_buflen(d->buffer, point);
	
	if (d->want_cursor) {
		if ((*d->display_ops->visible)(d, d->point))
			(*d->display_ops->draw_cursor)(d, d->point, FALSE);

		if ((*d->display_ops->visible)(d, point))
			(*d->display_ops->draw_cursor)(d, point, TRUE);
	}
	    
	d->point = point;
	
	return point;
}

void
edit_move_point(d, point)
Edit_display *d;
size_t point;
{
	edit_ensure_visible(d, edit_set_point(d, point));
}

static size_t
truncate_point_to_buflen(buffer, pos)
Edit_buffer *buffer;
size_t pos;
{
	size_t buflen;

	buflen = edit_get_buffer_length(buffer);

	if (pos > buflen)
		pos = buflen;

	return pos;
}

void
edit_set_mark(d, mark)
Edit_display *d;
size_t mark;
{
	d->mark = truncate_point_to_buflen(d->buffer, mark);
}

size_t
edit_get_mark(d)
Edit_display *d;
{
	return d->mark;
}

void
edit_set_point_limits(d, start, lim)
Edit_display *d;
size_t start, lim;
{
	if (start <= lim) {
		d->start = truncate_point_to_buflen(d->buffer, start);
		d->lim = truncate_point_to_buflen(d->buffer, lim);
	}
	else {
		d->start = truncate_point_to_buflen(d->buffer, lim);
		d->lim = truncate_point_to_buflen(d->buffer, start);
	}

	if ( d->history)
	{
	    const char* htext = edit_get_history(d->history, 0);
	    char* etext = edit_copy_bytes(d->buffer, d->start, d->lim, NULL);

	    d->history_offset = -1;

	    /*  Set up so that edit_f_scroll_history_up() gets
	    **  the last item or next to last as to whether
	    **  the last item matches the current text
	    */
	    if (htext && !strcmp(htext, etext))
		d->history_offset = 0;
	    else
		d->history_offset = -1;
	    free(etext);
	     
	}
}

void
edit_get_point_limits(d, p_start, p_lim)
Edit_display *d;
size_t *p_start, *p_lim;
{
	*p_start = d->start;
	*p_lim = d->lim;
}

bool
edit_set_want_cursor(d, want_cursor)
Edit_display *d;
bool want_cursor;
{
	bool had_cursor;

	had_cursor = d->want_cursor;
	d->want_cursor = want_cursor;

	if (had_cursor != want_cursor &&
	    (*d->display_ops->visible)(d, d->point)) {
		(*d->display_ops->draw_cursor)(d, d->point, want_cursor);
	}

	return had_cursor;
}

size_t
edit_get_point(d)
Edit_display *d;
{
	return d->point;
}

Edit_display *
edit__get_display_list()
{
	return All_displays;
}


static Edit_history* edit_create_history_by_size(size)
int size;
{
     int i;
     Edit_history* h = (Edit_history*)e_malloc(sizeof(*h));
     h->size = size;
     h->insert_point = 0;
     h->text_lines = (char**)e_malloc(size*sizeof(char*));
     for(i = 0; i < size; i++)
	 h->text_lines[i] = NULL;
     return h;
}

static void edit_create_history(history_item)
Edit_history_item* history_item;
{

    char buff[50];
    const char* request_size;
    int size = history_item->default_size;

    /* see if the user has requested a different size as an X-resource */
    sprintf(buff,"%sHistorySize", history_item->name);
    request_size = wn_get_default(buff);
    if ( request_size)
    {
	int rsize = atoi(request_size);
	if (rsize)
	    size = rsize;
    }
    *history_item->history_addr = edit_create_history_by_size(size);
}



void  edit_destroy_history(h)
Edit_history* h;
{
    int i;
    for(i = 0; i < h->size; i++)
	if ( h->text_lines[i])
	    free(h->text_lines[i]);
    free(h->text_lines);
    free(h);
}


const char*  edit_get_history(h, num)
Edit_history* h;
int num;
{
    int extract_point;
    if ( h == NULL)
	return NULL;

    if ( (unsigned)num >= h->size)
	return NULL;

    extract_point = h->insert_point - num -1;
    if ( extract_point < 0)
	extract_point += h->size;

    return h->text_lines[extract_point];

}

void edit_append_history(h, text)
Edit_history* h;
char* text;
{
    int i,j;
    int insert_point = h->insert_point;

    /* Ignore empty lines, or duplicates of the last */
    if (   !text
	|| !strlen(text) )
    {
	if ( text)
	    free(text);
	return;
    }

    /*  If it is a duplicate, just move the old copy 
    */
    i = insert_point;
    while(1)
    {
       i = (i ? i : h->size)-1;
       if ( i == insert_point || h->text_lines[i] == NULL)
	   break;
       if ( !strcmp( h->text_lines[i], text))
       {
	   free(h->text_lines[i]);
	   while(1)
	   {
	      j = i+1;
	      if (j == h->size)
		  j = 0;
	      if (j == insert_point)
	      {
		 h->text_lines[i] = text;
		 return;
	      }
	      h->text_lines[i] =  h->text_lines[j];
	      i = j;
	   }
       }
    }


    if ( h->text_lines[insert_point])
	free(h->text_lines[insert_point]);
    h->text_lines[insert_point++] = text;
    if ( insert_point == h->size)
	insert_point = 0;
    h->insert_point = insert_point;
}

static FILE* open_history_file(mode)
const char* mode;
{
    char buff[100];
    FILE* file = NULL;
    if ( history_state_file)
    {
	if ( history_state_dir )
	     sprintf(buff,"%s/%s", history_state_dir, history_state_file);
	else
	     sprintf(buff,"%s", history_state_file);
	file = fopen(buff, mode);
    }

    if ( !file && history_home_file)
    {
	const char* home = getenv("HOME");
	if ( home)
	{
	    sprintf(buff,"%s/%s", home, history_home_file);
	    file = fopen(buff, mode);
	}
    }
    return file;
}

static void load_history(file, history_item)
FILE* file;
Edit_history_item* history_item;
{
    int first_entry_offset;
    int next_char;
    Edit_history* history = *history_item->history_addr;
    fscanf(file,"%d\n", &first_entry_offset);  /* temp for old files */
    while (':' == (next_char = getc(file)))
    {
	char* text;
	int len;
	fscanf(file,"%d:", &len);
	text = e_malloc(len+1);
	fread(text, 1, len, file);
	text[len] = 0;
	edit_append_history(history, text);
    }

    /* we should have ended with a newline.
    ** if not, search for the end of line */
    while ( next_char != '\n' && next_char != EOF)
	next_char = getc(file);
}

static void save_history(file, history_item)
FILE* file;
Edit_history_item* history_item;
{
    Edit_history* history = *history_item->history_addr;
    if ( history)
    {
	int offset;

	/* See if the history is empty */
	if ( history->text_lines[0] == NULL )
	    return;

	fprintf(file,"%s\n",history_item->name);
	for (offset = history->size; offset--;)
	{
	    const char* text = edit_get_history(history, offset);
	    if ( text)
	    {
		int len = strlen(text);
		fprintf(file, ":%d:", len);
		fwrite(text, 1, len,file);
	    }
	}
	fprintf(file, "\n");
	edit_destroy_history(history);
	*history_item->history_addr = NULL;
    }
}

void  edit_initialize_history(state_dir, state_file, home_file,histories)
const char* state_dir;
const char* state_file;
const char* home_file;
Edit_history_item* histories;
{
    Edit_history_item* history_item;
    FILE* file;
    /* save the data */
    history_state_dir = state_dir;
    history_state_file = state_file;
    history_home_file = home_file;
    the_histories = histories;

    /* create all the history objects */
    for ( history_item = histories; history_item->name; history_item++)
	edit_create_history(history_item);

    /* try to open a history file */
    file = open_history_file("r");
    if (file)
    {
	 /* read the name of the next item */
	 char history_name[50];
	 while (fgets(history_name,sizeof(history_name),file) && *history_name)
	 {
	    history_name[strlen(history_name)-1] = 0;   /* kill new-line */

	    /* find that item in the histories list
	    ** if found, load the saved history
	    */
	    for ( history_item = histories; history_item->name; history_item++)
	    {
		if ( !strcmp(history_item->name, history_name))
		{
		    load_history(file, history_item);
		    break;
		}
	    }
	    if ( !history_item)
	    {
		/* here if we did not find a match on the name,
		** search for the next new-line to recover
		*/
		int next_char;
		do
		    next_char = getc(file);
		while ( next_char != '\n' && next_char != EOF);

	    }
	 }
	 fclose(file);
    }


}

void  edit_terminate_history()
{
    FILE* file;
    Edit_history_item* history_item = the_histories;
    if ( history_item == NULL)
       return;

    file = open_history_file("w");
    if ( file == NULL )
       return;

    while ( history_item->name)
	save_history(file, history_item++);
    fclose(file);
}
