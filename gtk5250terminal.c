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
#include <gtk/gtkselection.h>
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
#define A_5250_RULER_COLOR      0x900
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
static gboolean gtk5250_terminal_blink_timeout (Gtk5250Terminal *term);
static void gtk5250_terminal_map (GtkWidget *widget);
static void gtk5250_terminal_unmap (GtkWidget *widget);
static gboolean gtk5250_terminal_key_press_event (GtkWidget *widget, GdkEventKey *event);
static void gtk5250_input_handler (gpointer data, gint source, GdkInputCondition cond);
static void gtk5250_terminal_update_from_config (Gtk5250Terminal *This);
static gint gtk5250_terminal_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gint gtk5250_terminal_button_release_event (GtkWidget *widget, GdkEventButton *event);
static gint gtk5250_terminal_motion_notify_event (GtkWidget *widget, GdkEventMotion *event);
static void gtk5250_draw_selection (Gtk5250Terminal *term, gboolean fill);
static gint gtk5250_clear_selection (GtkWidget *widget, GdkEventSelection *event);
static void gtk5250_copy_selection (GtkWidget *widget, GtkSelectionData *selection_data, guint info, guint timestamp);
static void gtk5250_selection_received (GtkWidget *widget, GtkSelectionData *selection_data, guint time);
static void gtk5250_terminal_queuekey(Gtk5250Terminal *term, gint key);



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

struct _Gtk5250_color_map {
    gchar *name;
    gchar *spec;
};
typedef struct _Gtk5250_color_map Gtk5250_color_map;

static Gtk5250_color_map colorlist[] =
{
  { "white",       "rgb:ff/ff/ff" },
  { "red",         "rgb:ff/00/00" },
  { "turquoise",   "rgb:00/ff/ff" },
  { "yellow",      "rgb:ff/ff/00" },
  { "pink",        "rgb:ff/80/80" },
  { "blue",        "rgb:00/80/ff" },
  { "black",       "rgb:00/00/00" },
  { "green",       "rgb:00/ff/00" },
  { "ruler_color", "rgb:c0/00/00" },
  { NULL, NULL }
};

static gushort default_red[8] = 
  { 0xffff, 0xffff, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000 };
static gushort default_green[8] =
  { 0xffff, 0x0000, 0xffff, 0xffff, 0x8080, 0xc8c8, 0x0000, 0xffff };
static gushort default_blue[8] =
  { 0xffff, 0x0000, 0xffff, 0x0000, 0x8080, 0xffff, 0x0000, 0x0000 };

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
  widget_class->button_press_event = gtk5250_terminal_button_press_event;
  widget_class->button_release_event = gtk5250_terminal_button_release_event;
  widget_class->motion_notify_event = gtk5250_terminal_motion_notify_event;
  widget_class->selection_get = gtk5250_copy_selection;
  widget_class->selection_clear_event = gtk5250_clear_selection;
  widget_class->selection_received = gtk5250_selection_received;
}

/*
 *  Initialize the Gtk5250Terminal structure.
 */
static void gtk5250_terminal_init (Gtk5250Terminal *term)
{
  gint n;
  GdkColor clr;

  /* FIXME: Do we have to load our fonts here? */
  term->font_80 = gdk_font_load (DEFAULT_FONT);
  gdk_font_ref (term->font_80);
  term->font_80_w = gdk_char_width (term->font_80, 'M') + 1;
  term->font_80_h = gdk_char_height (term->font_80, 'M');

  term->font_132 = gdk_font_load (DEFAULT_FONT);
  gdk_font_ref (term->font_132);
  term->font_132_w = gdk_char_width (term->font_132, 'M') + 1;
  term->font_132_h = gdk_char_height (term->font_132, 'M');

  term->config = NULL;
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
  term->sel_gc = NULL;
  term->ssx = term->ssy = term->sex = term->sey = -1;
  term->sel_visible = FALSE;
  term->copybuf = NULL;
  term->copybufsize = 0;
  term->ruler = 0;
  term->rx = -1;
  term->ry = -1;

  n = 0;
  while (colorlist[n].name != NULL) {
      gdk_color_parse(colorlist[n].spec, &clr);
      term->red[n]   = clr.red;
      term->green[n] = clr.green;
      term->blue[n]  = clr.blue;
      n++;
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

  gtk_selection_add_target (GTK_WIDGET(term), GDK_SELECTION_PRIMARY,
                              GDK_SELECTION_TYPE_STRING, 1);
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
  GdkColor pen;
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
    | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK 
    | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK 
    | GDK_BUTTON1_MOTION_MASK ;
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

  term->sel_gc = gdk_gc_new (term->client_window);
  gdk_gc_set_function(term->sel_gc, GDK_INVERT);
  term->ssx = term->ssy = term->sex = term->sey = -1;
  term->sel_visible = FALSE;
  term->copybuf = NULL;
  term->copybufsize = 0;

  memset (term->colors, 0, sizeof (term->colors));
  nallocated = 0;
  gdk_color_context_get_pixels (term->color_ctx,
      term->red, term->green, term->blue, 9,
      term->colors, &nallocated);

  term->store = gdk_pixmap_new (term->client_window, term->font_80_w * 80,
      (term->font_80_h + 4) * 26, -1);

  /*  intialize entire background color to black */

  pen.pixel = term->colors[(A_5250_BLACK >> 8) - 1];
  gdk_gc_set_foreground (term->bg_gc, &pen);
  gdk_draw_rectangle(term->store, term->bg_gc, TRUE, 0, 0,
      widget->allocation.width+1, widget->allocation.height+1);

  term->timeout_id = g_timeout_add (500,
      (GSourceFunc)gtk5250_terminal_blink_timeout, term);
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

  g_source_remove (term->timeout_id);
  term->blink_timeout = 0;

  gdk_color_context_free (term->color_ctx);
  term->color_ctx = NULL;

  gdk_gc_destroy (term->bg_gc);
  term->bg_gc = NULL;

  gdk_gc_destroy (term->fg_gc);
  term->fg_gc = NULL;

  gdk_gc_destroy (term->sel_gc);
  term->sel_gc = NULL;

  g_free(term->copybuf);
  term->copybuf = NULL;
  term->copybufsize = 0;

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

  if (term->w != 132)
    {
      requisition->width = term->font_80_w * term->w + (2 * BORDER_WIDTH);
      requisition->height = (term->font_80_h + 4) * (term->h + 2) +
	(2 * BORDER_WIDTH);
    }
  else
    {
      requisition->width = term->font_132_w * term->w + (2 * BORDER_WIDTH);
      requisition->height = (term->font_132_h + 4) * (term->h + 2) +
	(2 * BORDER_WIDTH);
    }
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
  GdkFont *font;
  gint font_w, font_h;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK5250_IS_TERMINAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;

  term = GTK5250_TERMINAL (widget); 

  if (term->w != 132)
    {
      font_w = term->font_80_w;
      font_h = term->font_80_h;
      font = term->font_80;
    }
  else
    {
      font_w = term->font_132_w;
      font_h = term->font_132_h;
      font = term->font_132;
    }

  /* gdk_window_clear (widget->window); */

  /* erase previous ruler */

  if (term->ruler && term->rx!=-1 && term->ry!=-1) {
       for (x=0; x<term->w; x++) 
	      term->cells[term->ry][x] |= (guint)A_5250_DIRTYFLAG;
       for (y=0; y<term->h; y++) 
	      term->cells[y][term->rx] |= (guint)A_5250_DIRTYFLAG;
  }

  pen.pixel = term->colors[(A_5250_TURQ >> 8) - 1];
  gdk_gc_set_foreground (term->fg_gc, &pen);
  gdk_draw_line (term->store,
      term->fg_gc,
      0, (font_h + 4) * term->h + 4, 
      widget->allocation.width - (2 * BORDER_WIDTH),
      (font_h + 4) * term->h + 4 );

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

  /* draw new ruler */

  if (term->ruler) {
       term->rx = term->cx;
       term->ry = term->cy;
       x = term->cx * font_w;
       y = (term->cy * (font_h + 4)) + 4;
       pen.pixel = term->colors[(A_5250_RULER_COLOR >> 8) - 1];
       gdk_gc_set_foreground (term->fg_gc, &pen);
       gdk_draw_line (term->store, term->fg_gc,
            0, y, widget->allocation.width - (2*BORDER_WIDTH), y);
       gdk_draw_line (term->store, term->fg_gc,
            x, 4, x, term->h * (font_h+4) + 3);
  }
       

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      /* Draw indicators. */
       gdk_draw_rectangle (term->store, term->bg_gc, 1,
	  0, term->h * (font_h + 4) + 5,
	  (term->w * font_w), font_h + 4);

       pen.pixel = term->colors[(A_5250_WHITE >> 8) - 1];
       gdk_gc_set_foreground (term->fg_gc, &pen);
       gdk_draw_text (term->store, font, term->fg_gc,
	   1, (term->h + 1) * (font_h + 4) + 4,
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
  GdkFont *font;
  gint font_w, font_h;

  if (term->w != 132)
    {
      font_w = term->font_80_w;
      font_h = term->font_80_h;
      font = term->font_80;
    }
  else
    {
      font_w = term->font_132_w;
      font_h = term->font_132_h;
      font = term->font_132;
    }
  
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
      TN5250_LOG(("BLINK: state = %d\n", term->blink_state ? 1 : 0));
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
      x * font_w, y * (font_h + 4) + 4,
      font_w, font_h + 4);

  c = (ch & 0x00ff);
  gdk_draw_text (term->store, font, fg,
      x * font_w + 1, (y + 1) * (font_h + 4) + 1,
      &c, 1);

  if ((ch & A_5250_UNDERLINE) != 0)
    {
      gdk_draw_line (term->store, fg,
	  x * font_w, (y + 1) * (font_h + 4) + 3,
	  (x + 1) * font_w - 1, (y + 1) * (font_h + 4) + 3);
    }

  if ((ch & A_5250_VERTICAL) != 0)
    {
      gdk_draw_line (term->store, fg,
	  x * font_w, y * (font_h + 4) + 4,
	  x * font_w, (y + 1) * (font_h + 4) + 3);
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

  if (term->font_80 != NULL)
    gdk_font_unref (term->font_80);
  if (term->font_132 != NULL)
    gdk_font_unref (term->font_132);

  if(term->conn_tag != 0)
    gtk_input_remove(term->conn_tag);

  if(term->config != NULL)
    tn5250_config_unref (term->config);

  if (term->copybuf != NULL)
    g_free(term->copybuf);
  term->copybuf = NULL;
  term->copybufsize = 0;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/*
 *  This is the timeout handler which handles cursor/text blinking.
 */
static gboolean gtk5250_terminal_blink_timeout (Gtk5250Terminal *term)
{
  term->blink_state = !term->blink_state;
  TN5250_LOG(("BLINK: state changed to %d\n", term->blink_state ? 1: 0));
  gtk_widget_queue_draw ((GtkWidget*) term);
  return TRUE;
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

  /* We need to translate Shift+Fkey ourselves... */
  /* You can look up the symbols below in /usr/include/gdk/gdkkeysyms.h */
  switch (term->next_keyval)
    {
    case GDK_KP_Left:
    case GDK_Left:	
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_PREVWORD : K_LEFT;
      break;
    case GDK_KP_Up:
    case GDK_Up:	    term->next_keyval = K_UP; break;
    case GDK_KP_Right:
    case GDK_Right:	  
      term->next_keyval= (event->state & GDK_SHIFT_MASK) ? K_NEXTWORD : K_RIGHT;
      break;
    case GDK_KP_Down:
    case GDK_Down:	    term->next_keyval = K_DOWN; break;

    case GDK_Tab:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_BACKTAB : 9;
      break;
    case GDK_ISO_Left_Tab:
      term->next_keyval = K_BACKTAB;
      break;

    case GDK_KP_0: term->next_keyval = '0'; break;
    case GDK_KP_1: term->next_keyval = '1'; break;
    case GDK_KP_2: term->next_keyval = '2'; break;
    case GDK_KP_3: term->next_keyval = '3'; break;
    case GDK_KP_4: term->next_keyval = '4'; break;
    case GDK_KP_5: term->next_keyval = '5'; break;
    case GDK_KP_6: term->next_keyval = '6'; break;
    case GDK_KP_7: term->next_keyval = '7'; break;
    case GDK_KP_8: term->next_keyval = '8'; break;
    case GDK_KP_9: term->next_keyval = '9'; break;

    case GDK_KP_Add: term->next_keyval = K_FIELDPLUS; break;
    case GDK_KP_Subtract: term->next_keyval = K_FIELDMINUS; break;

    case GDK_KP_Enter:
    case GDK_Return:
      if (tn5250_config_get_bool (term->config, "bassackwards"))
	term->next_keyval = K_FIELDEXIT;
      else
	term->next_keyval = K_ENTER;
      break;

    case GDK_KP_Home:
    case GDK_Home:	 
      term->next_keyval= (event->state & GDK_SHIFT_MASK) ? K_FIELDHOME : K_HOME;
      break;
    case GDK_KP_Next:
    case GDK_Next:	    term->next_keyval = K_ROLLUP; break;
    case GDK_KP_Prior:
    case GDK_Prior:	    term->next_keyval = K_ROLLDN; break;
    case GDK_KP_End:
    case GDK_End:	    term->next_keyval = K_END; break;

    case GDK_F1: case GDK_KP_F1:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F13 : K_F1;
      break;
    case GDK_F2: case GDK_KP_F2:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F14 : K_F2;
      break;
    case GDK_F3: case GDK_KP_F3:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F15 : K_F3;
      break;
    case GDK_F4: case GDK_KP_F4:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F16 : K_F4;
      break;
    case GDK_F5:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F17 : K_F5;
      break;
    case GDK_F6:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F18 : K_F6;
      break;
    case GDK_F7:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F19 : K_F7;
      break;
    case GDK_F8:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F20 : K_F8;
      break;
    case GDK_F9:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F21 : K_F9;
      break;
    case GDK_F10:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F22 : K_F10;
      break;
    case GDK_F11:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F23 : K_F11;
      break;
    case GDK_F12:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_F24 : K_F12;
      break;

    case GDK_Escape:
      term->next_keyval = (event->state & GDK_SHIFT_MASK) ? K_SYSREQ :
	K_ATTENTION;
      break;

    case GDK_KP_Divide: term->next_keyval = '/'; break;
    case GDK_KP_Multiply: term->next_keyval = '*'; break;

    case GDK_Control_L:	    term->next_keyval = K_RESET; break;
    case GDK_Control_R:
      if (tn5250_config_get_bool (term->config, "bassackwards"))
	term->next_keyval = K_ENTER;
      else
	term->next_keyval = K_FIELDEXIT;
      break;
    case GDK_BackSpace:	    term->next_keyval = K_BACKSPACE; break;
    case GDK_Insert:	    term->next_keyval = K_INSERT; break;
    case GDK_KP_Delete:
    case GDK_Delete:	    term->next_keyval = K_DELETE; break;
    case GDK_Pause:	    term->next_keyval = K_HELP; break;

    /* Extra mappings that just happen to coincide */
    case GDK_Sys_Req:
    case GDK_Print:	    term->next_keyval = K_PRINT; break;
    case GDK_Help:	    term->next_keyval = K_HELP; break;
   
    /* Map the 3270 codes ;) */
    case GDK_3270_Duplicate:term->next_keyval = K_DUPLICATE; break;
    case GDK_3270_BackTab:  term->next_keyval = K_BACKTAB; break;
    case GDK_3270_Reset:    term->next_keyval = K_RESET; break;
    case GDK_3270_Test:	    term->next_keyval = K_TESTREQ; break;
    case GDK_3270_Attn:	    term->next_keyval = K_ATTENTION; break;
    case GDK_3270_PrintScreen: term->next_keyval = K_PRINT; break;
    case GDK_3270_Enter:    term->next_keyval = K_ENTER; break;

    case 'Q':
    case 'q': 
      if (event->state & GDK_CONTROL_MASK) gtk_exit(0);
      break; 

   case 't':
   case 'T':
      if (event->state & GDK_CONTROL_MASK) term->next_keyval = K_TESTREQ;
      break; 

    default:
      if (term->next_keyval >= 127)
	g_warning("unhandled key 0x%04X (%s)", term->next_keyval,
	    gdk_keyval_name(term->next_keyval));
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
  Gtk5250Terminal *term;
  Tn5250Config *old_config;

  term = GTK5250_TERMINAL (This->data);

  old_config = term->config;
  term->config = config;
  if (term->config != NULL)
    tn5250_config_ref (term->config);
  if (old_config != NULL)
    tn5250_config_unref (old_config);
  gtk5250_terminal_update_from_config (term);
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
  gint font_w, font_h;

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
  This->display = dsp;

  gtk5250_clear_selection(GTK_WIDGET(This), NULL);

  if (display_resized)
    {
      for (y = 0; y < 28; y++)
	for (x = 0; x < 132; x++)
	  This->cells[y][x] = ' ' | A_5250_DIRTYFLAG;
      if (This->w != 132)
	{
	  font_w = This->font_80_w;
	  font_h = This->font_80_h;
	}
      else
	{
	  font_w = This->font_132_w;
	  font_h = This->font_132_h;
	}
      if (This->store)
	gdk_pixmap_unref (This->store);
      This->store = gdk_pixmap_new (This->client_window,
	  font_w * This->w, (font_h + 4) * (This->h + 1) + 10,
	  -1);
      gtk_widget_set_usize ((GtkWidget*) This,
	  font_w * This->w + BORDER_WIDTH*2,
	  (font_h + 4) * (This->h + 1) + 10 + BORDER_WIDTH*2);
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
 gint j;

  g_return_val_if_fail(tnThis != NULL,-1);
  g_return_val_if_fail(tnThis->data != NULL, -1);
  g_return_val_if_fail(GTK5250_IS_TERMINAL(tnThis->data), -1);

  This = GTK5250_TERMINAL(tnThis->data);

  if (This->next_keyval == 0) {
    if (This->k_buf_len == 0)
        return -1;
    This->next_keyval = This->k_buf[0];
    This->k_buf_len --;
    for (j=0; j<This->k_buf_len; j++)
       This->k_buf[j] = This->k_buf[j+1];
  }

  keyval = This->next_keyval;
  This->next_keyval = 0;
  return keyval;
}

static void gtk5250_terminal_update_from_config (Gtk5250Terminal *This)
{
  const gchar *s;
  gint n, r, g, b;
  const gchar *spec;
  GdkColor clr;
  GdkColor pen;
  gchar temp[15];
  
  /* Set the font, width and height, etc. */
  if (This->font_80 != NULL)
    gdk_font_unref (This->font_80);
  s = tn5250_config_get (This->config, "font_80");
  if (s != NULL)
    {
      This->font_80 = gdk_font_load (s);
      if (This->font_80 == NULL)
	{
	  g_warning ("Couldn't load font %s", s);
	  This->font_80 = gdk_font_load (DEFAULT_FONT);
	}
    }
  else
    This->font_80 = gdk_font_load (DEFAULT_FONT);
  gdk_font_ref (This->font_80);

  /* FIXME: Get some real font metrics. */
  This->font_80_w = gdk_char_width (This->font_80, 'M') + 1;
  This->font_80_h = gdk_char_height (This->font_80, 'M');

  if (This->font_132 != NULL)
    gdk_font_unref (This->font_132);
  s = tn5250_config_get (This->config, "font_132");
  if (s != NULL)
    {
      This->font_132 = gdk_font_load (s);
      if (This->font_132 == NULL)
	{
	  g_warning ("Couldn't load font %s", s);
	  This->font_132 = gdk_font_load (DEFAULT_FONT);
	}
    }
  else
    This->font_132 = gdk_font_load (DEFAULT_FONT);

  gdk_font_ref (This->font_132);
  /* FIXME: Get some real font metrics. */
  This->font_132_w = gdk_char_width (This->font_132, 'M') + 1;
  This->font_132_h = gdk_char_height (This->font_132, 'M');

  if (tn5250_config_get (This->config, "ruler")) 
      This->ruler = tn5250_config_get_bool (This->config, "ruler");

  if (tn5250_config_get_bool(This->config, "black_on_white")) {
      gdk_color_parse("rgb:00/00/00", &clr);
      for (n=0; n<(A_5250_GREEN >> 8); n++) {
           This->red[n]   = clr.red;
           This->green[n] = clr.green;
           This->blue[n]  = clr.blue;
      }
      gdk_color_parse("rgb:ff/ff/ff", &clr);
      n = (A_5250_BLACK >> 8) - 1;
      This->red[n]   = clr.red;
      This->green[n] = clr.green;
      This->blue[n]  = clr.blue;
  }

  if (tn5250_config_get_bool(This->config, "white_on_black")) {
      gdk_color_parse("rgb:ff/ff/ff", &clr);
      for (n=0; n<(A_5250_GREEN >> 8); n++) {
           This->red[n]   = clr.red;
           This->green[n] = clr.green;
           This->blue[n]  = clr.blue;
      }
      gdk_color_parse("rgb:00/00/00", &clr);
      n = (A_5250_BLACK >> 8) - 1;
      This->red[n]   = clr.red;
      This->green[n] = clr.green;
      This->blue[n]  = clr.blue;
  }

  n = 0;
  while (colorlist[n].name != NULL) {
      if ((spec=tn5250_config_get(This->config, colorlist[n].name)) != NULL)  {
          g_print("colorname = %s, config val = %s\n", colorlist[n].name, spec);
          if (gdk_color_parse(spec, &clr)) {
               This->red[n]   = clr.red;
               This->green[n] = clr.green;
               This->blue[n]  = clr.blue;
          }
          else if (tn5250_parse_color(This->config, colorlist[n].name,
                                 &r, &g, &b) != -1)  {
               sprintf(temp, "rgb:%02x/%02x/%02x", r&0xff, g&0xff, b&0xff);
               if (gdk_color_parse(temp, &clr)) {
                    This->red[n]   = clr.red;
                    This->green[n] = clr.green;
                    This->blue[n]  = clr.blue;
               }
          }
      }
      n++;
  }


  /*  intialize entire background to "black" color */

  if (This->store != NULL && This->bg_gc!=NULL) {
     pen.pixel = This->colors[(A_5250_BLACK >> 8) - 1];
     gdk_gc_set_foreground (This->bg_gc, &pen);
     gdk_draw_rectangle(This->store, This->bg_gc, TRUE, 0, 0,
         GTK_WIDGET(This)->allocation.width+1, 
         GTK_WIDGET(This)->allocation.height+1);
  }

}

static gint
gtk5250_terminal_button_press_event (GtkWidget *widget,
				     GdkEventButton *event) {
    Gtk5250Terminal *term;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK5250_IS_TERMINAL(widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    term = GTK5250_TERMINAL(widget);

    switch (event->button) {

      case 1:  /* select text */
        gtk5250_clear_selection(widget, NULL);
        term->ssx = term->sex = event->x;
        term->ssy = term->sey = event->y;
	/* draw new box */
        gtk5250_draw_selection (term, FALSE);
        gtk_widget_draw (widget, NULL);
        return FALSE;

      case 2:  /* paste text */
	gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, 
	                               GDK_SELECTION_TYPE_STRING, 
				       GDK_CURRENT_TIME);
	return FALSE;
    }

    return FALSE;
}

static gint
gtk5250_terminal_button_release_event (GtkWidget *widget,
				       GdkEventButton *event) {
    Gtk5250Terminal *term;
    gint font_w, font_h;
    gint cx, cy;
    gint movex, movey;
    gint sx, sy, ex, ey;
    guchar c;
    gint bp, x, y;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK5250_IS_TERMINAL(widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    term = GTK5250_TERMINAL(widget);

    if (event->button != 1)
         return FALSE;
    if (!term->sel_visible)
         return FALSE;

    /* erase box */
    gtk5250_draw_selection (term, FALSE);

    term->sex = event->x;
    term->sey = event->y;

    if (term->sex<0) term->sex = 0;
    if (term->sey<0) term->sey = 0;

/* get font height/width */

    if (term->w != 132) {
      font_w = term->font_80_w;
      font_h = term->font_80_h;
    } else {
      font_w = term->font_132_w;
      font_h = term->font_132_h;
    }

/* if flip start/end coordinates if one is higher than the other */

#define TN5250_FLIPEM(a, b)  if (a>b) { x = a; a = b; b = x; }
          TN5250_FLIPEM(term->ssx, term->sex)
          TN5250_FLIPEM(term->ssy, term->sey)
#undef TN5250_FLIPEM
    
/* save the amount that the pointer has moved */
    movex = term->sex - term->ssx;
    movey = term->sey - term->ssy;

/* constrain coordinates to the window's client area */

    if (term->sex < 0) term->sex = 0;
    if (term->sex > widget->allocation.width) 
             term->sex = widget->allocation.width;
    if (term->sey < 0) term->sey = 0;
    if (term->sey > widget->allocation.height)
             term->sey = widget->allocation.height;

    if (term->ssx < 0) term->ssx = 0;
    if (term->ssx > widget->allocation.width) 
             term->ssx = widget->allocation.width;
    if (term->ssy < 0) term->ssy = 0;
    if (term->ssy > widget->allocation.height)
             term->ssy = widget->allocation.height;

/* move start position to a character boundary */

    cx = term->ssx / font_w;
    term->ssx = cx * font_w;
    cy = (term->ssy - 4) / (font_h + 4);
    term->ssy = (cy * (font_h + 4)) + 4;

/* move end position to a character boundary */

    cx = term->sex / font_w;
    if (term->sex % font_w) cx++;
    term->sex = cx * font_w - 1;
    if (term->sex < 0) term->sex = 0;
    cy = (term->sey - 4) / (font_h + 4);
    if ((term->sey - 4) % (font_h + 4)) cy++;
    term->sey = cy * (font_h + 4) + 3;

/* draw new (solid) box */
    gtk5250_draw_selection (term, TRUE);

/* if the start/end positions haven't changed much, the user probably
    wasn't trying to select anything...  */

     if (movex<2 && movey<2)  {
           gtk5250_clear_selection(widget, NULL);
           return FALSE;
     }

/* remove previous contents of copy buffer */

/*
     if (gdk_selection_owner_get (GDK_SELECTION_PRIMARY) == widget->window) {
         gtk_selection_owner_set (NULL, GDK_SELECTION_PRIMARY, 
	                          GDK_CURRENT_TIME);
     }
*/

     if (term->copybuf != NULL) 
          g_free(term->copybuf);
     term->copybuf = NULL;
     term->copybufsize = 0;

/* calculate start/end character positions of selected region */

     sx = term->ssx / font_w;
     ex = (term->sex+1) / font_w;
     sy = (term->ssy - 4) / (font_h + 4);
     ey = (term->sey - 3) / (font_h + 4);

/* copy the character cells into the copy buffer.  Add a newline char
   whenever we move to a different line on the screen */

     term->copybufsize = ((ex-sx)+1) * ((ey-sy)+1);
     term->copybuf = g_malloc((term->copybufsize+1) * sizeof(guchar));
     memset(term->copybuf, 0, term->copybufsize+1);
     bp = -1;

     for (y=sy; y<ey; y++) {
 	for (x=sx; x<ex; x++) {
 	    c = (guchar)(term->cells[y][x] & 0xff);
 	    bp++;
 	    if (bp==term->copybufsize) break;
 	    term->copybuf[bp] = c;
 	}
 	if (y != (ey-1)) {
	    bp++;
	    if (bp==term->copybufsize) break;
	    term->copybuf[bp] = '\n';
        }
    }

/* claim ownership of the "primary selection" (main clipboard).  If
    it fails, we'll unselect the text, since we won't be able to 
    transfer it to clipboard... */

    if (!gtk_selection_owner_set (widget, 
	                         GDK_SELECTION_PRIMARY, 
				 GDK_CURRENT_TIME       )) {
           TN5250_LOG (("GTK: gtk_selection_owner_set failed!!\n"));
           gtk5250_clear_selection (widget, NULL);
    }

    gtk_widget_draw (GTK_WIDGET(term), NULL);
    return FALSE;
}

static gint
gtk5250_terminal_motion_notify_event (GtkWidget *widget,
				      GdkEventMotion *event) {
    Gtk5250Terminal *term;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK5250_IS_TERMINAL(widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    term = GTK5250_TERMINAL(widget);

    /* erase old box */
    gtk5250_draw_selection (term, FALSE);

    term->sex = event->x;
    term->sey = event->y;

    if (term->sex < 0) term->sex = 0;
    if (term->sey < 0) term->sey = 0;


    /* draw new box */
    gtk5250_draw_selection (term, FALSE);

    gtk_widget_draw (GTK_WIDGET(term), NULL);
    return FALSE;
}


static void
gtk5250_draw_selection (Gtk5250Terminal *term, gboolean fill) {

    int sx, sy;
    int w, h;

    if (term->ssx <= term->sex) {
	 sx = term->ssx;
	 w = (term->sex - term->ssx) + 1;
    }
    else {
	 sx = term->sex;
	 w = (term->ssx - term->sex) + 1;
    }

    if (term->ssy <= term->sey) {
	 sy = term->ssy;
	 h = (term->sey - term->ssy) + 1;
    }
    else {
	 sy = term->sey;
	 h = (term->ssy - term->sey) + 1;
    }

    if (fill) { w++; h++; };

    if (term->sel_visible)
         term->sel_visible = FALSE;
    else
         term->sel_visible = TRUE;

    gdk_draw_rectangle( term->store, term->sel_gc, fill, sx, sy, w, h);

}

static void
gtk5250_copy_selection (GtkWidget *widget,
                        GtkSelectionData *selection_data,
		        guint info, guint timestamp) {

    g_return_if_fail (widget != NULL);

    gtk_selection_data_set (selection_data, GDK_SELECTION_TYPE_STRING,
          8*sizeof(guchar), GTK5250_TERMINAL(widget)->copybuf, 
                            GTK5250_TERMINAL(widget)->copybufsize);

}

static gint
gtk5250_clear_selection (GtkWidget *widget,
                         GdkEventSelection *event) {

    g_return_val_if_fail (widget != NULL, FALSE);

    /* since we are overriding the default handler for this event,
       we need to call the GTK handler manually (for system events)
       Otherwise, GTK will lose track of it's internal state...    */
    if (event != NULL) 
       if (!gtk_selection_clear (widget, event))
           return FALSE;

    if (GTK5250_TERMINAL(widget)->sel_visible) {
        gtk5250_draw_selection (GTK5250_TERMINAL(widget), TRUE);
        GTK5250_TERMINAL(widget)->sel_visible = FALSE;
        gtk_widget_draw (widget, NULL);
    }

    return FALSE;
}

static void
gtk5250_selection_received (GtkWidget *widget,
                            GtkSelectionData *selection_data,
			    guint            time) {

     guchar *buf, *p;
     Gtk5250Terminal *term;
     gint thisrow;

     g_return_if_fail (widget != NULL);
     g_return_if_fail (selection_data->type == GDK_TARGET_STRING);
     g_return_if_fail (selection_data->length > 0);

     term = GTK5250_TERMINAL(widget);
     g_return_if_fail (term->display !=NULL);

     buf = g_malloc((selection_data->length+1) * sizeof(guchar));
     memcpy(buf, selection_data->data, selection_data->length);
     buf[selection_data->length] = '\0';
     p = buf;

     thisrow = 0;
     while (*p) {
        switch (*p) {
          case '\r':
            break;
          case '\n':
            while (thisrow > 0) {
               gtk5250_terminal_queuekey(term, K_LEFT);
               thisrow --;
               if (term->k_buf_len == GTK5250_MAX_K_BUF) {
                      tn5250_display_do_keys(term->display);
               }
            }
            gtk5250_terminal_queuekey(term, K_DOWN);
            break;
          default:
            thisrow++;
            gtk5250_terminal_queuekey(term, (*p));
            break;
        }
        if (term->k_buf_len == GTK5250_MAX_K_BUF) {
             tn5250_display_do_keys(term->display);
        }
        p++;
     }

     g_free(buf);
     tn5250_display_do_keys(term->display);

}

static void
gtk5250_terminal_queuekey(Gtk5250Terminal *term, gint key) {
     if (term->k_buf_len < GTK5250_MAX_K_BUF) {
           term->k_buf[term->k_buf_len] = key;
           term->k_buf_len ++;
     }
}
