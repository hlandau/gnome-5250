#ifndef __GTK5250_TERMINAL_H__
#define __GTK5250_TERMINAL_H__

/* GNOME 5250
 * Copyright (C) 1999 Jason M. Felice
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <tn5250.h>

#define GTK5250_TERMINAL_TYPE \
    (gtk5250_terminal_get_type ())
#define GTK5250_TERMINAL(obj) \
    GTK_CHECK_CAST (obj, GTK5250_TERMINAL_TYPE, Gtk5250Terminal)
#define GTK5250_TERMINAL_CLASS(klass) \
    GTK_CHECK_CLASS_CAST (klass, GTK5250_TERMINAL_TYPE, Gtk5250TerminalClass)
#define GTK5250_IS_TERMINAL(obj) \
    GTK_CHECK_TYPE (obj, GTK5250_TERMINAL_TYPE)

typedef struct _Gtk5250Terminal Gtk5250Terminal;
typedef struct _Gtk5250TerminalClass Gtk5250TerminalClass;

struct _Gtk5250Terminal
{
  GtkWidget	    widget;

  Tn5250Terminal    tn5250_impl;  /* Our implementation of the 5250 object */
  gint		    conn_tag;	  /* Tag from gtk_input_add_full() */
  gint		    pending;	  /* Pending events for waitevent to return */
  guint		    next_keyval;

  GdkFont*	    font;
  gint		    font_w, font_h;

  GdkPixmap*	    store;
  GdkWindow*	    client_window;
  GdkGC*	    bg_gc;
  GdkGC*	    fg_gc;
  GdkColorContext*  color_ctx;

  gushort	    red[8];
  gushort	    green[8];
  gushort	    blue[8];
  gulong	    colors[8];
  guint		    blink_timeout;

  gint		    blink_state : 1;	/* Is blink currently on? */

  gint		    cx, cy, w, h;       /* Cursor position/display size. */
  guint		    cells[27][132];	/* Data currently on display. */
  gchar		    ind_buf[80];	/* Indicator buffer. */
};

struct _Gtk5250TerminalClass
{
  GtkWidgetClass    parent_class;
};

GtkType		gtk5250_terminal_get_type	(void);
GtkWidget*	gtk5250_terminal_new		(void);
Tn5250Terminal*	gtk5250_terminal_get_impl	(Gtk5250Terminal *This);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK5250_TERMINAL_H__ */

/* vi:set sts=2 sw=2: */
