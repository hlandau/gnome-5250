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

#include <gtk/gtk.h>
#include "gtk5250terminal.h"
#include "gtk5250propdlg.h"

static void file_connect_callback (void);
static void file_disconnect_callback (void);
static void file_preferences_callback (void);
static void file_exit_callback (void);
static void help_about_callback (void);

Tn5250Session *sess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Terminal *term = NULL;
Tn5250Display *display = NULL;
Tn5250Config *config = NULL;
const char *tracefile = NULL;

static GtkItemFactoryEntry menu_items[] =
{
  {"/_File", NULL, NULL, 0, "<Branch>"},
/*
  {"/File/_Connect...", NULL, file_connect_callback, 0, NULL},
  {"/File/_Disconnect", NULL, file_disconnect_callback, 0, NULL},
  {"/File/sep1", NULL, NULL, 0, "<Separator>"},
  {"/File/_Preferences...", NULL, file_preferences_callback, 0, NULL},
  */
  {"/File/sep2", NULL, NULL, 0, "<Separator>"},
  {"/File/E_xit   Ctrl-Q", NULL, file_exit_callback, 0, NULL},
  {"/_Help", NULL, NULL, 0, "<Branch>"},
  {"/Help/_About", NULL, help_about_callback, 0, NULL},
};

void 
get_main_menu (GtkWidget * window,
	       GtkWidget ** menubar)
{
  GtkItemFactory *item_factory;
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  accel_group = gtk_accel_group_new ();

  /* This function initializes the item factory.
   * Param 1: The type of menu - can be GTK_TYPE_MENU_BAR, GTK_TYPE_MENU,
   * or GTK_TYPE_OPTION_MENU.
   * Param 2: The path of the menu.
   * Param 3: A pointer to a gtk_accel_group.  The item factory sets up
   * the accelerator table while generating menus. */

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
				       accel_group);

  /* This function generates the menu items. Pass the item factory,
   * the number of items in the array, the array itself, and any
   * callback data for the the menu items. */
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  /* Attach the new accelerator group to the window. */
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  if (menubar)
    /* Finally, return the actual menu bar created by the item factory. */
    *menubar = gtk_item_factory_get_widget (item_factory, "<main>");

}

static void 
syntax (void)
{
  exit (1);
}

/*
 *    Program entry point.
 */
int 
main (int argc, char *argv[])
{
  GtkWidget *app;
  GtkWidget *term_window;
  GtkWidget *vbox;
  GtkWidget *menu;
  GtkSettings *settings;

  gtk_init (&argc, &argv);

  config = tn5250_config_new ();
  if (tn5250_config_load_default (config) == -1)
    {
      tn5250_config_unref (config);
      exit (1);
    }
  if (tn5250_config_parse_argv (config, argc, argv) == -1)
    {
      tn5250_config_unref (config);
      syntax ();
    }
  if (tn5250_config_get (config, "help"))
    syntax ();
  else if (tn5250_config_get (config, "version"))
    {
      g_print ("gtk-5250 %s\n", VERSION);
      exit (0);
    }

  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (app), vbox);
  
  get_main_menu (app, &menu);
  if (menu != NULL)
    {
      gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, TRUE, 0);
      gtk_widget_show (menu);
    }

  settings = gtk_widget_get_settings(app);
  gtk_settings_set_string_property(settings, "gtk-menu-bar-accel", 
        "VoidSymbol", "gtk-5250");


  term_window = gtk5250_terminal_new ();
  gtk_box_pack_end (GTK_BOX (vbox), term_window, TRUE, TRUE, 0);
  gtk_widget_show (term_window);
  gtk_widget_show (vbox);

  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
		      GTK_SIGNAL_FUNC (file_exit_callback), NULL);

#ifndef NDEBUG
  tracefile = tn5250_config_get (config, "trace");
  if (tracefile != NULL)
    tn5250_log_open (tracefile);
#endif

  if (tn5250_config_get (config, "host"))
    {
      stream = tn5250_stream_open (tn5250_config_get (config, "host"), config);
      if (stream == NULL)
	return 255; /* FIXME: Error message */
      if (tn5250_stream_config (stream, config) == -1)
	return 255; /* FIXME: Error message */
    }
  else
    stream = NULL;

  display = tn5250_display_new ();
  if (display == NULL)
    return 255;			/* FIXME: Error Message. */
  if (tn5250_display_config (display, config) == -1)
    return 255;			/* FIXME: An error message would be nice. */

  term = gtk5250_terminal_get_impl (GTK5250_TERMINAL (term_window));
  if (term == NULL)
    return 255;			/* FIXME: An error message would be nice. */
  if (tn5250_terminal_config (term, config) == -1)
    return 255;			/* FIXME: An error message would be nice. */
#ifndef NDEBUG
  /* Shrink-wrap the terminal with the debug terminal, if appropriate. */
  if (stream != NULL)
    {
      const char *remotehost = tn5250_config_get (config, "host");
      if (strlen (remotehost) >= 6
	  && !memcmp (remotehost, "debug:", 6))
	{
	  Tn5250Terminal *dbgterm = tn5250_debug_terminal_new (term, stream);
	  if (dbgterm == NULL)
	    {
	      tn5250_terminal_destroy (term);
	      return 255;		/* FIXME: Error message. */
	    }
	  term = dbgterm;
	  if (tn5250_terminal_config (term, config) == -1)
	    return 255;		/* FIXME: Error message. */
	}
    }
#endif
  tn5250_terminal_init (term);
  tn5250_display_set_terminal (display, term);

  sess = tn5250_session_new ();
  if (sess == NULL)
    return 255;			/* FIXME: Error message. */
  tn5250_display_set_session (display, sess);
  if ( stream != NULL )
    {
      tn5250_session_set_stream (sess, stream);
      term->conn_fd = tn5250_stream_socket_handle (stream);
    }
  if (tn5250_session_config (sess, config) == -1)
    return 255;			/* FIXME: An error message would be nice. */

  gtk_widget_show (app);
  tn5250_session_main_loop (sess);
  return 0;
}

/*
 *    Connect to the remote host.
 */
static void 
file_connect_callback ()
{
}

/*
 *    Disconnect from the remote host.
 */
static void 
file_disconnect_callback ()
{
}

/*
 *    Edit emulator preferences.
 */
static void 
file_preferences_callback ()
{
  static GtkWidget *dlg = NULL;

  if (dlg == NULL)
    {
      dlg = gtk5250_prop_dlg_new ();
      gtk5250_prop_dlg_set_config (GTK5250_PROP_DLG (dlg), config);
    }
  else
    gtk5250_prop_dlg_update_dialog (GTK5250_PROP_DLG (dlg));

  gtk_widget_show (dlg);
  /* FIXME: Raise window. */
}

/*
 *    Quit the application.
 */
static void 
file_exit_callback ()
{
  gtk_exit (0);
}

/*
 *    Show the Help | About dialog.
 */
static void 
help_about_callback ()
{
  GtkWidget *dlg;
  const gchar *authors[] =
  {
    "Roger Bowler",
    "Ron Colcernian",
    "Carey Evans",
    "Jay 'Eraserhead' Felice <jasonf@nacs.net>",
    "Colin McCormack",
    "Michael Madore <mmadore@blarg.net>",
    "Peter Schlaile",
    "William J. Suetholz",
    NULL
  };

  /* FIXME:
     dlg = gnome_about_new ( _("Gnome 5250 Emulator"), VERSION,
     _("Copyright (C) 1999 Michael Madore"),
     authors,
     _("A GNU General Public License 5250 Emulator for Gnome systems."),
     PKGDATADIR "/tn5250-logo.xpm");

     gtk_widget_show (dlg); */
}

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
