#ifndef __GTK5250_TERMINAL_H__
#define __GTK5250_TERMINAL_H__

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

  Tn5250Terminal    tn5250_impl;	/* Our implementation of the 5250 object */
  gint		    conn_tag;		/* Tag from gtk_input_add_full() */
  gint		    pending;		/* Pending events for gtkterm_waitevent to return */
  guint		    next_keyval;

  Tn5250Display*    char_buffer;	/* A display used as a character buffer. */

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
