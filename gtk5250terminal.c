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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtk5250terminal.h"

#define DEFAULT_FONT "fixed"
#define BORDER_WIDTH 3

/* Color definitions. */
#define A_5250_WHITE		0x100
#define A_5250_RED		0x200
#define A_5250_TURQ		0x300
#define A_5250_YELLOW		0x400
#define A_5250_PINK		0x500
#define A_5250_BLUE		0x600
#define A_5250_BLACK		0x700
#define A_5250_GREEN		0x800
#define A_5250_COLOR_MASK	0xf00

#define A_5250_REVERSE		0x1000
#define A_5250_UNDERLINE	0x2000
#define A_5250_BLINK		0x4000
#define A_5250_VERTICAL		0x8000

/* Pseudo-attribute we use to determine whether we have to draw this cell
 * or not. */
#define A_5250_DIRTYFLAG        0x10000

static void gtk5250_terminal_class_init (Gtk5250TerminalClass *klass);
static void gtk5250_terminal_init (Gtk5250Terminal *term);
static void gtk5250_terminal_realize (GtkWidget *widget);
static void gtk5250_terminal_unrealize (GtkWidget *widget);
static void gtk5250_terminal_size_request (GtkWidget *widget,
    GtkRequisition *requisition);
static void gtk5250_terminal_size_allocate (GtkWidget *widget,
    GtkAllocation *allocation);
static gint gtk5250_terminal_expose (GtkWidget *widget, GdkEventExpose *event);
static void gtk5250_terminal_destroy (GtkObject *object);
static void gtk5250_terminal_draw_char (Gtk5250Terminal *term, gint y, gint x, guint ch);
static void gtk5250_terminal_blink_timeout (Gtk5250Terminal *term);
static void gtk5250_terminal_map (GtkWidget *widget);
static void gtk5250_terminal_unmap (GtkWidget *widget);
static gboolean gtk5250_terminal_key_press_event (GtkWidget *widget, GdkEventKey *event);
static void gtk5250_input_handler (gpointer data, gint source, GdkInputCondition cond);

  /* tn5250 implementation handlers */
static void gtkterm_init (Tn5250Terminal *This);
static void gtkterm_term (Tn5250Terminal *This);
static void gtkterm_destroy (Tn5250Terminal *This);
static int gtkterm_width (Tn5250Terminal *This);
static int gtkterm_height (Tn5250Terminal *This);
static int gtkterm_flags (Tn5250Terminal *This);
static void gtkterm_update (Tn5250Terminal *This, Tn5250Display *dsp);
static void gtkterm_update_indicators (Tn5250Terminal *This, Tn5250Display *dsp);
static int gtkterm_waitevent (Tn5250Terminal *This);
static int gtkterm_getkey (Tn5250Terminal *This);
static void gtkterm_beep (Tn5250Terminal *This);
static int gtkterm_config (Tn5250Terminal *This, Tn5250Config *config);

static GtkWidgetClass *parent_class = NULL;

static gushort default_red[8] = 
  { 0xffff, 0xffff, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000 };
static gushort default_green[8] =
  { 0xffff, 0x0000, 0xc8c8, 0xffff, 0x8080, 0x0000, 0x0000, 0xffff };
static gushort default_blue[8] =
  { 0xffff, 0x0000, 0xc8c8, 0x0000, 0x8080, 0xffff, 0x0000, 0x0000 };

static int attribute_map[] =
{A_5250_GREEN,
 A_5250_GREEN | A_5250_REVERSE,
 A_5250_WHITE,
 A_5250_WHITE | A_5250_REVERSE,
 A_5250_GREEN | A_5250_UNDERLINE,
 A_5250_GREEN | A_5250_UNDERLINE | A_5250_REVERSE,
 A_5250_WHITE | A_5250_UNDERLINE,
 0x00,
 A_5250_RED,
 A_5250_RED | A_5250_REVERSE,
 A_5250_RED | A_5250_BLINK,
 A_5250_RED | A_5250_BLINK | A_5250_REVERSE,
 A_5250_RED | A_5250_UNDERLINE,
 A_5250_RED | A_5250_UNDERLINE | A_5250_REVERSE,
 A_5250_RED | A_5250_UNDERLINE | A_5250_BLINK,
 0x00,
 A_5250_TURQ | A_5250_VERTICAL,
 A_5250_TURQ | A_5250_VERTICAL | A_5250_REVERSE,
 A_5250_YELLOW | A_5250_VERTICAL,
 A_5250_YELLOW | A_5250_VERTICAL | A_5250_REVERSE,
 A_5250_TURQ | A_5250_UNDERLINE | A_5250_VERTICAL,
 A_5250_TURQ | A_5250_UNDERLINE | A_5250_REVERSE | A_5250_VERTICAL,
 A_5250_YELLOW | A_5250_UNDERLINE | A_5250_VERTICAL,
 0x00,
 A_5250_PINK,
 A_5250_PINK | A_5250_REVERSE,
 A_5250_BLUE,
 A_5250_BLUE | A_5250_REVERSE,
 A_5250_PINK | A_5250_UNDERLINE,
 A_5250_PINK | A_5250_UNDERLINE | A_5250_REVERSE,
 A_5250_BLUE | A_5250_UNDERLINE,
 0x00};

/*
 *  Return the Gtk5250Terminal's type id, registering the type if necessary.
 */
GtkType gtk5250_terminal_get_type ()
{
  static GtkType terminal_type = 0;
  if (!terminal_type)
    {
      GtkTypeInfo type_info =
      {
	"Gtk5250Terminal",
	sizeof (Gtk5250Terminal),
	sizeof (Gtk5250TerminalClass),
	(GtkClassInitFunc) gtk5250_terminal_class_init,
	(GtkObjectInitFunc) gtk5250_terminal_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL
      };
      terminal_type = gtk_type_unique (gtk_widget_get_type (), &type_info);
    }
  return terminal_type;
}

/*
 *  Initialize the Gtk5250TerminalClass structure.
 */
static void gtk5250_terminal_class_init (Gtk5250TerminalClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  widget_class = (GtkWidgetClass*) klass;
  object_class = (GtkObjectClass*) klass;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  object_class->destroy = gtk5250_terminal_destroy;
  widget_class->realize = gtk5250_terminal_realize;
  widget_class->unrealize = gtk5250_terminal_unrealize;
  widget_class->size_request = gtk5250_terminal_size_request;
  widget_class->size_allocate = gtk5250_terminal_size_allocate;
  widget_class->expose_event = gtk5250_terminal_expose;
  widget_class->map = gtk5250_terminal_map;
  widget_class->unmap = gtk5250_terminal_unmap;
  widget_class->key_press_event = gtk5250_terminal_key_press_event;
}

/*
 *  Initialize the Gtk5250Terminal structure.
 */
static void gtk5250_terminal_init (Gtk5250Terminal *term)
{
  gint n;

  term->font = gdk_font_load (DEFAULT_FONT);
  gdk_font_ref (term->font);
  term->font_w = gdk_char_width (term->font, 'M') + 1;
  term->font_h = gdk_char_height (term->font, 'M');

  term->store = NULL;
  term->client_window = NULL;
  term->bg_gc = NULL;
  term->fg_gc = NULL;
  term->color_ctx = NULL;
  term->blink_timeout = 0;
  term->blink_state = 1;
  term->cx = term->cy = 0;
  term->w = 80;
  term->h = 24;

  for (n = 0; n < 8; n++) {
    term->red[n] = default_red[n];
    term->green[n] = default_green[n];
    term->blue[n] = default_blue[n];
  }

  memset (term->cells, 0, sizeof (term->cells));
  memset (term->ind_buf, ' ', sizeof (term->ind_buf));

  term->conn_tag = 0;
  term->pending = 0;
  term->next_keyval = 0;

  term->tn5250_impl.init = gtkterm_init;
  term->tn5250_impl.term = gtkterm_term;
  term->tn5250_impl.destroy = gtkterm_destroy;
  term->tn5250_impl.width = gtkterm_width;
  term->tn5250_impl.height = gtkterm_height;
  term->tn5250_impl.flags = gtkterm_flags;
  term->tn5250_impl.update = gtkterm_update;
  term->tn5250_impl.update_indicators = gtkterm_update_indicators;
  term->tn5250_impl.waitevent = gtkterm_waitevent;
  term->tn5250_impl.getkey = gtkterm_getkey;
  term->tn5250_impl.beep = gtkterm_beep;
  term->tn5250_impl.config = gtkterm_config;

  term->tn5250_impl.conn_fd = -1;
  term->tn5250_impl.data = (void*)term;
}

/*
 *  Create a new Gtk5250Terminal widget.
 */
GtkWidget *gtk5250_terminal_new ()
{
  Gtk5250Terminal *term;
  term = gtk_type_new (gtk5250_terminal_get_type ());
  return GTK_WIDGET (term);
}

/*
 *  Get our terminal implementation object (used by lib5250).
 */
Tn5250Terminal *gtk5250_terminal_get_impl (Gtk5250Terminal *This)
{
  g_return_val_if_fail(This != NULL,NULL);
  g_return_val_if_fail(GTK5250_IS_TERMINAL(This),NULL);

  return &This->tn5250_impl;
}

/*
 *  Realize the widget.
 */
static void gtk5250_terminal_realize (GtkWidget *widget)
{
  Gtk5250Terminal *term;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint nallocated;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED | GTK_CAN_FOCUS);
  term = GTK5250_TERMINAL (widget);

  /* main window */
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK
    | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  /* terminal window */
  attributes.x = BORDER_WIDTH;
  attributes.y = BORDER_WIDTH;
  attributes.width = widget->allocation.width - (BORDER_WIDTH * 2);
  attributes.height = widget->allocation.height - (BORDER_WIDTH * 2);
  term->client_window = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (term->client_window, term);

  term->bg_gc = gdk_gc_new (term->client_window);
  term->fg_gc = gdk_gc_new (term->client_window);
  term->color_ctx = gdk_color_context_new (gtk_widget_get_visual (widget),
	gtk_widget_get_colormap (widget));

  memset (term->colors, 0, sizeof (term->colors));
  nallocated = 0;
  gdk_color_context_get_pixels (term->color_ctx,
      term->red, term->green, term->blue, 8,
      term->colors, &nallocated);

  term->store = gdk_pixmap_new (term->client_window, term->font_w * 80,
      (term->font_h + 4) * 26, -1);

  term->blink_timeout = gtk_timeout_add (500,
      (GtkFunction)gtk5250_terminal_blink_timeout, term);
}

/*
 *  Unrealize the widget.
 */
static void gtk5250_terminal_unrealize (GtkWidget *widget)
{
  Gtk5250Terminal *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));

  term = GTK5250_TERMINAL (widget);

  gdk_window_unref (term->client_window);
  term->client_window = NULL;

  gdk_pixmap_unref (term->store);
  term->store = NULL;

  gtk_timeout_remove (term->blink_timeout);
  term->blink_timeout = 0;

  gdk_color_context_free (term->color_ctx);
  term->color_ctx = NULL;

  gdk_gc_destroy (term->bg_gc);
  term->bg_gc = NULL;

  gdk_gc_destroy (term->fg_gc);
  term->fg_gc = NULL;

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

/*
 *  Request our preferred size.
 */
static void gtk5250_terminal_size_request (GtkWidget *widget,
    GtkRequisition *requisition)
{
  Gtk5250Terminal *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));
  g_return_if_fail (requisition != NULL);

  term = GTK5250_TERMINAL (widget);

  requisition->width = term->font_w * term->w + (2 * BORDER_WIDTH);
  requisition->height = (term->font_h + 4) * (term->h + 2) +
    (2 * BORDER_WIDTH);
}

/*
 *  Set our size based on what GTK+ has computed.
 */
static void gtk5250_terminal_size_allocate (GtkWidget *widget,
    GtkAllocation *allocation)
{
  Gtk5250Terminal *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));
  g_return_if_fail (allocation != NULL);

  term = GTK5250_TERMINAL (widget);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
	  allocation->x, allocation->y,
	  allocation->width, allocation->height);
      gdk_window_move_resize (term->client_window,
	  BORDER_WIDTH, BORDER_WIDTH,
	  allocation->width - BORDER_WIDTH *2,
	  allocation->height - BORDER_WIDTH *2);
    }
} 

/*
 *  Draw the display.
 */
static gint gtk5250_terminal_expose (GtkWidget *widget, GdkEventExpose *event)
{
  Gtk5250Terminal *term;
  GdkColor pen;
  gint y, x;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK5250_IS_TERMINAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;

  term = GTK5250_TERMINAL (widget); 

  /* gdk_window_clear (widget->window); */

  pen.pixel = term->colors[(A_5250_TURQ >> 8) - 1];
  gdk_gc_set_foreground (term->fg_gc, &pen);
  gdk_draw_line (term->store,
      term->fg_gc,
      0, (term->font_h + 4) * term->h + 4, 
      widget->allocation.width - (2 * BORDER_WIDTH),
      (term->font_h + 4) * term->h + 4 );

  for (y = 0; y < 27; y++)
    {
      for (x = 0; x < 132; x++)
	{
	  if (((term->cells[y][x] & A_5250_DIRTYFLAG) != 0) ||
	    ((term->cells[y][x] & A_5250_BLINK) != 0) ||
	    (term->cy == y && term->cx == x))
	    {
	      gtk5250_terminal_draw_char (term, y, x, term->cells[y][x]);
	      term->cells[y][x] &= ~((guint)A_5250_DIRTYFLAG);
	    }
	}
    }

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      /* Draw indicators. */
       gdk_draw_rectangle (term->store, term->bg_gc, 1,
	  0, term->h * (term->font_h + 4) + 5,
	  (80 * term->font_w), term->font_h + 4);

       pen.pixel = term->colors[(A_5250_WHITE >> 8) - 1];
       gdk_gc_set_foreground (term->fg_gc, &pen);
       gdk_draw_text (term->store, term->font, term->fg_gc,
	   1, (term->h + 1) * (term->font_h + 4) + 4,
	   term->ind_buf, 80);

      gdk_draw_pixmap (term->client_window, term->fg_gc,
	  term->store,
	  event->area.x, event->area.y, /* Source coordinates */
	  event->area.x, event->area.y, /* Dest coordinates */
	  event->area.width, event->area.height);

    }

  return FALSE;
}

static void gtk5250_terminal_draw_char (Gtk5250Terminal *term, gint y, gint x, guint ch)
{
  GdkColor pen;
  GtkWidget *widget;
  GdkGC *fg, *bg;
  gint color_idx;
  gchar c;
  
  color_idx = ((ch & A_5250_COLOR_MASK) >> 8) - 1;
  widget = (GtkWidget *)term;

  if (color_idx < 0)
    {
      color_idx = 6;
      ch = ' ';
    }

  /* Draw the cursor (in blue) when it blinks */
  if(y == term->cy && x == term->cx && term->blink_state)
    {
      ch = ch ^ A_5250_REVERSE;
      if ((ch & A_5250_REVERSE) != 0)
	color_idx = (A_5250_BLUE >> 8) - 1;
    }

  pen.pixel = term->colors[color_idx];
  gdk_gc_set_foreground (term->fg_gc, &pen);

  if ((ch & A_5250_BLINK) != 0 && !term->blink_state)
    {
      ch = (ch & ~0xff) | (guchar)' ';
      ch = ch & ~A_5250_REVERSE & ~A_5250_UNDERLINE;
    }
  
  if ((ch & A_5250_REVERSE) != 0)
    {
      bg = term->fg_gc;
      fg = term->bg_gc;
    }
  else
    {
      fg = term->fg_gc;
      bg = term->bg_gc;
    }

  gdk_draw_rectangle (term->store, bg, 1,
      x * term->font_w, y * (term->font_h + 4) + 4,
      term->font_w, term->font_h + 4);

  c = (ch & 0x00ff);
  gdk_draw_text (term->store, term->font, fg,
      x * term->font_w + 1, (y + 1) * (term->font_h + 4) + 1,
      &c, 1);

  if ((ch & A_5250_UNDERLINE) != 0)
    {
      gdk_draw_line (term->store, fg,
	  x * term->font_w, (y + 1) * (term->font_h + 4) + 3,
	  (x + 1) * term->font_w - 1, (y + 1) * (term->font_h + 4) + 3);
    }

  if ((ch & A_5250_VERTICAL) != 0)
    {
      gdk_draw_line (term->store, fg,
	  x * term->font_w, y * (term->font_h + 4) + 4,
	  x * term->font_w, (y + 1) * (term->font_h + 4) + 3);
    }
}

/*
 *  Destroy handler.
 */
static void gtk5250_terminal_destroy (GtkObject *object)
{
  Gtk5250Terminal *term;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (object));

  term = GTK5250_TERMINAL (object);

  gdk_font_unref (term->font);

  if(term->conn_tag != 0)
    gtk_input_remove(term->conn_tag);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/*
 *  This is the timeout handler which handles cursor/text blinking.
 */
static void gtk5250_terminal_blink_timeout (Gtk5250Terminal *term)
{
  term->blink_state = !term->blink_state;
  gtk_widget_queue_draw ((GtkWidget*) term);
}

/*
 *  Show the widget's windows.
 */
static void gtk5250_terminal_map (GtkWidget *widget)
{
  Gtk5250Terminal *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));

  term = GTK5250_TERMINAL (widget);

  if (!GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

      gdk_window_show (widget->window);
      gdk_window_show (term->client_window);

      /* FIXME: is this right? --JMP --JMF */
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
    }
}

/*
 *  Hide the widget's windows.
 */
static void gtk5250_terminal_unmap (GtkWidget *widget)
{
  Gtk5250Terminal *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));

  term = GTK5250_TERMINAL (widget);

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

      gdk_window_hide (widget->window);
      gdk_window_hide (term->client_window);
    }
}

/*
 *  Handle a key_press_event signal.
 */
static gboolean gtk5250_terminal_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  Gtk5250Terminal *term;

  g_return_val_if_fail(widget != NULL, FALSE);
  g_return_val_if_fail(GTK5250_IS_TERMINAL(widget), FALSE);

  term = GTK5250_TERMINAL(widget);

  if (GTK_WIDGET_CLASS (parent_class)->key_press_event)
    (* GTK_WIDGET_CLASS (parent_class)->key_press_event) (widget,event);

  term->pending |= TN5250_TERMINAL_EVENT_KEY; 
  term->next_keyval = event->keyval;
  if ((event->state & GDK_SHIFT_MASK) != 0)
    {
      /* We need to translate Shift+Fkey ourselves... */
      switch (term->next_keyval)
	{
	case GDK_F1:  term->next_keyval = GDK_F13; break;
	case GDK_F2:  term->next_keyval = GDK_F14; break;
	case GDK_F3:  term->next_keyval = GDK_F15; break;
	case GDK_F4:  term->next_keyval = GDK_F16; break;
	case GDK_F5:  term->next_keyval = GDK_F17; break;
	case GDK_F6:  term->next_keyval = GDK_F18; break;
	case GDK_F7:  term->next_keyval = GDK_F19; break;
	case GDK_F8:  term->next_keyval = GDK_F20; break;
	case GDK_F9:  term->next_keyval = GDK_F21; break;
	case GDK_F10: term->next_keyval = GDK_F22; break;
	case GDK_F11: term->next_keyval = GDK_F23; break;
	case GDK_F12: term->next_keyval = GDK_F24; break;
	}
    }

  gtk_main_quit();
  return TRUE;
}

/*
 *  This is a callback for when the socket becomes readable.  Basically, we just
 *  break out of the main loop so that gtkterm_waitevent will return.
 */
static void gtk5250_input_handler (gpointer data, gint source, GdkInputCondition cond)
{
  Gtk5250Terminal *This;

  g_return_if_fail(data != NULL);
  g_return_if_fail(GTK5250_IS_TERMINAL(data));

  This = GTK5250_TERMINAL(data);
  This->pending |= TN5250_TERMINAL_EVENT_DATA;

  gtk_main_quit();
}

static void gtkterm_init (Tn5250Terminal *tnThis)
{
  /* NOOP: We don't need to initialize anything. */
}

static void gtkterm_term (Tn5250Terminal *tnThis)
{
  tnThis->conn_fd = -1; /* Just in case. */
}

static void gtkterm_destroy (Tn5250Terminal *tnThis)
{
  /* Not much to do here: we get destroyed when the GTK+ widget does. */
  gtkterm_term(tnThis);
}

static int gtkterm_width (Tn5250Terminal *This)
{
  return 132; /* FIXME: Fudge this based on font size and screen width. */
}

static int gtkterm_height (Tn5250Terminal *This)
{
  return 27; /* FIXME: Fudge this based on font size and screen height. */
}

static int gtkterm_flags (Tn5250Terminal *This)
{
  return TN5250_TERMINAL_HAS_COLOR;
}

static void gtkterm_beep (Tn5250Terminal *This)
{
  gdk_beep();
}

static int gtkterm_config (Tn5250Terminal *This, Tn5250Config *config)
{
  /* FIXME: Load config info. */
  return 0;
}

static void gtkterm_update (Tn5250Terminal *tnThis, Tn5250Display *dsp)
{
  Gtk5250Terminal *This;
  gint y, x;
  guchar a = 0x20, c;
  guint attrs;
  gboolean have_char = FALSE;
  gboolean display_resized = FALSE;
  GdkRectangle area;

  g_return_if_fail(tnThis != NULL);
  g_return_if_fail(tnThis->data != NULL);
  g_return_if_fail(GTK5250_IS_TERMINAL(tnThis->data));
  g_return_if_fail(dsp != NULL);

  This = GTK5250_TERMINAL(tnThis->data);
  if (This->w != tn5250_display_width (dsp)
      || This->h != tn5250_display_height (dsp))
    display_resized = TRUE;
  This->w = tn5250_display_width(dsp);
  This->h = tn5250_display_height(dsp);

  if (display_resized)
    {
      for (y = 0; y < 28; y++)
	for (x = 0; x < 132; x++)
	  This->cells[y][x] = ' ' | A_5250_DIRTYFLAG;
      if (This->store)
	gdk_pixmap_unref (This->store);
      This->store = gdk_pixmap_new (This->client_window,
	  This->font_w * This->w, (This->font_h + 4) * (This->h + 1) + 10,
	  -1);
      gtk_widget_set_usize ((GtkWidget*) This,
	  This->font_w * This->w + BORDER_WIDTH*2,
	  (This->font_h + 4) * (This->h + 1) + 10 + BORDER_WIDTH*2);
    }

  for(y = 0; y < This->h; y++)
    {
      for(x = 0; x < This->w; x++)
	{
	  c = tn5250_display_char_at(dsp, y, x);
	  if ((c & 0xe0) == 0x20)
	    {
	      a = (c & 0xff);
	      if (This->cells[y][x] != ((guint)' ' | attribute_map[0]))
		This->cells[y][x] = A_5250_GREEN | A_5250_DIRTYFLAG | ' ';
	    }
	  else
	    {
	      attrs = attribute_map[a - 0x20];
	      if(attrs == 0) /* NONDISPLAY */
		{
		  if (This->cells[y][x] != (attrs | (guint)' '))
		    This->cells[y][x] = attrs | A_5250_DIRTYFLAG | (guint)' ';
		}
	      else
		{
		  /* UNPRINTABLE -- print block */
		  if ((c==0x1f) || (c==0x3f))
		    {
		      c = ' ';
		      attrs ^= A_5250_REVERSE;
		    }
		  /* UNPRINTABLE -- print blank */
		  else if ((c < 0x40 && c > 0x00) || c == 0xff)
		    c = ' ';
		  else
		    c = tn5250_char_map_to_local (tn5250_display_char_map (dsp), c);
		  if (This->cells[y][x] != (attrs | (guint)c))
		    This->cells[y][x] = attrs | A_5250_DIRTYFLAG | (guint)c;
		}
	    }
	}
    }

  if (tn5250_display_cursor_y (dsp) != This->cy ||
      tn5250_display_cursor_x (dsp) != This->cx)
    {
      This->cells[This->cy][This->cx] |= A_5250_DIRTYFLAG;
      This->cy = tn5250_display_cursor_y (dsp);
      This->cx = tn5250_display_cursor_x (dsp);
      This->cells[This->cy][This->cx] |= A_5250_DIRTYFLAG;
    }

  This->blink_state = 1;
  gtkterm_update_indicators (tnThis, dsp);

  if (display_resized)
    gtk_widget_queue_resize (gtk_widget_get_toplevel ((GtkWidget*) This));
}

static void gtkterm_update_indicators (Tn5250Terminal *This, Tn5250Display *dsp)
{
  /* FIXME: We should use the graphical indicators instead. */
  /* FIXME: We should at least character buffer this like we do the display
   * for redraw speed's sake. */
   Gtk5250Terminal *term = GTK5250_TERMINAL (This->data);
   int inds = tn5250_display_indicators(dsp);
   GdkColor pen;

   memset(term->ind_buf, ' ', sizeof(term->ind_buf));
   memcpy(term->ind_buf, "5250", 4);
   if ((inds & TN5250_DISPLAY_IND_MESSAGE_WAITING) != 0)
      memcpy(term->ind_buf + 23, "MW", 2);
   if ((inds & TN5250_DISPLAY_IND_INHIBIT) != 0)
      memcpy(term->ind_buf + 9, "X II", 4);
   else if ((inds & TN5250_DISPLAY_IND_X_CLOCK) != 0)
      memcpy(term->ind_buf + 9, "X CLOCK", 7);
   else if ((inds & TN5250_DISPLAY_IND_X_SYSTEM) != 0)
      memcpy(term->ind_buf + 9, "X SYSTEM", 8);
   if ((inds & TN5250_DISPLAY_IND_INSERT) != 0)
      memcpy(term->ind_buf + 30, "IM", 2);
   if ((inds & TN5250_DISPLAY_IND_FER) != 0)
      memcpy(term->ind_buf + 33, "FER", 3);

   sprintf(term->ind_buf+72,"%03.3d/%03.3d",tn5250_display_cursor_x(dsp)+1,
      tn5250_display_cursor_y(dsp)+1);
   *strchr(term->ind_buf, 0) = ' ';

   gtk_widget_draw ((GtkWidget*) term, NULL);
}

static int gtkterm_waitevent (Tn5250Terminal *tnThis)
{
  Gtk5250Terminal *This;
  gint pending;

  g_return_val_if_fail(tnThis != NULL,-1);
  g_return_val_if_fail(tnThis->data != NULL, -1);
  g_return_val_if_fail(GTK5250_IS_TERMINAL(tnThis->data), -1);

  This = GTK5250_TERMINAL(tnThis->data);

  /* Set up our handler to wait on the file descriptor. */
  if(This->tn5250_impl.conn_fd >= 0 && This->conn_tag == 0)
    {
      This->conn_tag = gtk_input_add_full (This->tn5250_impl.conn_fd, 
	  GDK_INPUT_READ, gtk5250_input_handler, NULL, (gpointer)This,
	  NULL);
    }
  else if(This->tn5250_impl.conn_fd < 0 && This->conn_tag != 0)
    {
      gtk_input_remove(This->conn_tag);
      This->conn_tag = 0;
    }

  gtk_main();

  pending = This->pending;
  This->pending = 0;
  return pending;
}

static int gtkterm_getkey (Tn5250Terminal *tnThis)
{
  Gtk5250Terminal *This;
  guint keyval;

  g_return_val_if_fail(tnThis != NULL,-1);
  g_return_val_if_fail(tnThis->data != NULL, -1);
  g_return_val_if_fail(GTK5250_IS_TERMINAL(tnThis->data), -1);

  This = GTK5250_TERMINAL(tnThis->data);

  keyval = This->next_keyval;
  This->next_keyval = 0;

  /* You can look up the symbols below in /usr/include/gdk/gdkkeysyms.h */
  switch (keyval)
    {
    case 0:		    return -1;	      /* Tell the key handling loop in
						 session.c that we have no
						 more keys. */

    case GDK_Left:	    return K_LEFT;
    case GDK_Up:	    return K_UP;
    case GDK_Right:	    return K_RIGHT;
    case GDK_Down:	    return K_DOWN;
    case GDK_Tab:	    return 9;
    case GDK_Return:	    return K_FIELDEXIT;
    case GDK_Home:	    return K_HOME;
    case GDK_Next:	    return K_ROLLUP;
    case GDK_Prior:	    return K_ROLLDN;
    case GDK_End:	    return K_END;
    case GDK_F1:	    return K_F1;
    case GDK_F2:	    return K_F2;
    case GDK_F3:	    return K_F3;
    case GDK_F4:	    return K_F4;
    case GDK_F5:	    return K_F5;
    case GDK_F6:	    return K_F6;
    case GDK_F7:	    return K_F7;
    case GDK_F8:	    return K_F8;
    case GDK_F9:	    return K_F9;
    case GDK_F10:	    return K_F10;
    case GDK_F11:	    return K_F11;
    case GDK_F12:	    return K_F12;
    case GDK_F13:	    return K_F13;
    case GDK_F14:	    return K_F14;
    case GDK_F15:	    return K_F15;
    case GDK_F16:	    return K_F16;
    case GDK_F17:	    return K_F17;
    case GDK_F18:	    return K_F18;
    case GDK_F19:	    return K_F19;
    case GDK_F20:	    return K_F20;
    case GDK_F21:	    return K_F21;
    case GDK_F22:	    return K_F22;
    case GDK_F23:	    return K_F23;
    case GDK_F24:	    return K_F24;
    case GDK_Control_L:	    return K_RESET;
    case GDK_Control_R:	    return K_ENTER;
    case GDK_BackSpace:	    return K_BACKSPACE;
    case GDK_Insert:	    return K_INSERT;
    case GDK_Delete:	    return K_DELETE;
    case GDK_Escape:	    return K_SYSREQ;
    case GDK_Pause:	    return K_HELP;

    /* Extra mappings that just happen to coincide */
    case GDK_Print:	    return K_PRINT;
    case GDK_Help:	    return K_HELP;
   
    /* Map the 3270 codes ;) */
    case GDK_3270_Duplicate:return K_DUPLICATE;
    case GDK_3270_BackTab:  return K_BACKTAB;
    case GDK_3270_Reset:    return K_RESET;
    case GDK_3270_Test:	    return K_TESTREQ;
    case GDK_3270_Attn:	    return K_ATTENTION;
    case GDK_3270_PrintScreen: return K_PRINT;
    case GDK_3270_Enter:    return K_ENTER;

    }

  if(keyval < 127)	  /* Return valid ASCII codes */
    return (int)keyval;

  g_warning("unhandled key 0x%04X (%s)", keyval, gdk_keyval_name(keyval));
  return -1;
}

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
