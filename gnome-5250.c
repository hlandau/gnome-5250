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

#include <gnome.h>
#include "gtk5250terminal.h"
#include "gtk5250propdlg.h"

static void file_connect_callback (void);
static void file_disconnect_callback (void);
static void file_preferences_callback (void);
static void file_exit_callback (void);
static void help_about_callback (void);

static GnomeUIInfo file_menu[] = {
   GNOMEUIINFO_ITEM_STOCK ( _("_Connect..."), _("Connect to the remote system."),
	 file_connect_callback, GNOME_STOCK_PIXMAP_REFRESH),
   GNOMEUIINFO_ITEM_STOCK ( _("_Disconnect"), _("Disconnect from the remote sysstem."),
	 file_disconnect_callback, GNOME_STOCK_PIXMAP_STOP),
   GNOMEUIINFO_SEPARATOR,
   GNOMEUIINFO_MENU_PREFERENCES_ITEM (file_preferences_callback, NULL),
   GNOMEUIINFO_SEPARATOR,
   GNOMEUIINFO_MENU_EXIT_ITEM (file_exit_callback, NULL),
   GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
   GNOMEUIINFO_MENU_ABOUT_ITEM (help_about_callback, NULL),
   GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
   GNOMEUIINFO_MENU_FILE_TREE (file_menu),
   GNOMEUIINFO_MENU_HELP_TREE (help_menu),
   GNOMEUIINFO_END
};

Tn5250Session *sess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Terminal *term = NULL;
Tn5250Display *display = NULL;
Tn5250Config *config = NULL;
const char *tracefile = NULL;

static void bomb_out (const gchar *msg)
{
  GtkWidget *mb;
  mb = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_ERROR, _("Ok"), NULL);
  gtk_widget_show (mb);
  gtk_main ();
  exit (1); /* FIXME: Implement. */
}

static void syntax (void)
{
  bomb_out ("Syntax error.");
}

/*
 *    Program entry point.
 */
int main (int argc, char *argv[])
{
   GtkWidget *app;
   GtkWidget *term_window;

   gnome_init ("gnome-5250", VERSION, argc, argv);

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
	 g_print ("gnome-5250 %s\n", VERSION);
	 exit (0);
      }

   app = gnome_app_new ("gnome-5250", N_("Gnome 5250 Emulator"));
   gnome_app_create_menus (GNOME_APP (app), main_menu);

   term_window = gtk5250_terminal_new ();
   gnome_app_set_contents (GNOME_APP (app), term_window);
   gtk_widget_show (term_window);

   gtk_signal_connect (GTK_OBJECT (app), "delete_event",
	 GTK_SIGNAL_FUNC(file_exit_callback), NULL);

#ifndef NDEBUG
   tracefile = tn5250_config_get (config, "trace");
   if (tracefile != NULL)
      tn5250_log_open (tracefile);
#endif

   if (tn5250_config_get (config, "host"))
     {
       stream = tn5250_stream_open (tn5250_config_get (config, "host"), config);
       if (stream == NULL)
	  bomb_out ( _("Could not connect to host.") );
       if (tn5250_stream_config (stream, config) == -1)
	  syntax ();
     }
   else
     stream = NULL;

   display = tn5250_display_new ();
   if (display == NULL)
     bomb_out ( _("Could not create display.") );
   if (tn5250_display_config (display, config) == -1)
     syntax ();

   term = gtk5250_terminal_get_impl(GTK5250_TERMINAL(term_window));
   if (term == NULL)
     bomb_out ( _("Could not create terminal.") );
   if (tn5250_terminal_config (term, config) == -1)
     syntax ();
#ifndef NDEBUG
   /* Shrink-wrap the terminal with the debug terminal, if appropriate. */
   if (stream != NULL)
     {
	const char *remotehost = tn5250_config_get (config, "host");
	if (strlen (remotehost) >= 6
	      && !memcmp (remotehost, "debug:", 6)) {
	   Tn5250Terminal *dbgterm = tn5250_debug_terminal_new (term, stream);
	   if (dbgterm == NULL) {
	      tn5250_terminal_destroy (term);
	      bomb_out ( _("Could not create debug terminal.") );
	   }
	   term = dbgterm;
	   if (tn5250_terminal_config (term, config) == -1)
	     syntax ();
	}
     }
#endif
   tn5250_terminal_init (term);
   tn5250_display_set_terminal (display, term);

   sess = tn5250_session_new();
   if (sess == NULL)
     bomb_out ( _("Could not create session.") );
   tn5250_display_set_session (display, sess);
   if (stream != NULL)
     {
       tn5250_session_set_stream (sess, stream);
       term->conn_fd = tn5250_stream_socket_handle (stream);
      }
   if (tn5250_session_config (sess, config) == -1)
     syntax ();

   gtk_widget_show (app);
   tn5250_session_main_loop (sess);
   return 0;
}

/*
 *    Connect to the remote host.
 */
static void file_connect_callback ()
{
}

/*
 *    Disconnect from the remote host.
 */
static void file_disconnect_callback ()
{
}

/*
 *    Edit emulator preferences.
 */
static void file_preferences_callback ()
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
static void file_exit_callback ()
{
   gtk_exit(0);
}

/*
 *    Show the Help | About dialog.
 */
static void help_about_callback ()
{
   GtkWidget *dlg;
   const gchar *authors[] = {
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

   dlg = gnome_about_new ( _("Gnome 5250 Emulator"), VERSION,
	 _("Copyright (C) 1999 Michael Madore"),
	 authors,
	 _("A GNU General Public License 5250 Emulator for Gnome systems."),
	 PKGDATADIR "/tn5250-logo.xpm");

   gtk_widget_show (dlg);
}

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
