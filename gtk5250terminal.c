
#include <gtk/gtk.h>
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
static void gtk5250_terminal_draw_char (Gtk5250Terminal *term, gint y, gint x, gint ch);
static void gtk5250_terminal_blink_timeout (Gtk5250Terminal *term);
static void gtk5250_terminal_map (GtkWidget *widget);
static void gtk5250_terminal_unmap (GtkWidget *widget);

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

  for (n = 0; n < 8; n++) {
    term->red[n] = default_red[n];
    term->green[n] = default_green[n];
    term->blue[n] = default_blue[n];
  }
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

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
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
      (term->font_h + 4) * 25, -1);

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
  gint h, w;
  Gtk5250Terminal *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));
  g_return_if_fail (requisition != NULL);

  term = GTK5250_TERMINAL (widget);

  /* FIXME: Take into account whether we are 80x24 or 132x27 */
  requisition->width = term->font_w * 80 + (2 * BORDER_WIDTH);
  requisition->height = (term->font_h + 4) * 25 + (2 * BORDER_WIDTH);
}

/*
 *  Set our size based on what GTK+ has computed.
 */
static void gtk5250_terminal_size_allocate (GtkWidget *widget,
    GtkAllocation *allocation)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK5250_IS_TERMINAL (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
	  allocation->x, allocation->y,
	  allocation->width, allocation->height);
    }
} 

/*
 *  Draw the display.
 */
static gint gtk5250_terminal_expose (GtkWidget *widget, GdkEventExpose *event)
{
  Gtk5250Terminal *term;
  GdkColor pen;
  gint i, x, h;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK5250_IS_TERMINAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;

  term = GTK5250_TERMINAL (widget); 

  gdk_window_clear (widget->window);

  h = gdk_char_height (term->font, 'M'); 

  /* FIXME: This has to take into account the current height. */
  pen.pixel = term->colors[(A_5250_TURQ >> 8) - 1];
  gdk_gc_set_foreground (term->fg_gc, &pen);
  gdk_draw_line (term->store,
      term->fg_gc, 0, (h + 4) * 24, 
      widget->allocation.width - (2 * BORDER_WIDTH), (h + 4) * 24);

  for (i = 0; i < sizeof(attribute_map)/sizeof (int); i++)
    {
      static char *test_str = "** This is a test string **";
    
      for (x = 0; x < strlen(test_str); x++)
	gtk5250_terminal_draw_char (term, i % 22, x + ((i / 22) * 40),
	    test_str[x] | attribute_map[i]);
    }

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      gdk_draw_pixmap (term->client_window, term->fg_gc,
	  term->store, 0, 0, 0, 0,
	  term->font_w * 80,
	  (term->font_h + 4) * 25);
    }

  return FALSE;
}

static void gtk5250_terminal_draw_char (Gtk5250Terminal *term, gint y, gint x, gint ch)
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

  c = (ch & 0xff);
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
	  x * term->font_w, y * (term->font_h + 4) + 3,
	  x * term->font_w, (y + 1) * (term->font_h + 4) + 2);
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

/* vi:set sts=2 sw=2: */
