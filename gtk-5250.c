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

static void file_connect_callback (void);
static void file_disconnect_callback (void);
static void file_preferences_callback (void);
static void file_exit_callback (void);
static void help_about_callback (void);

Tn5250Session *tnsess;
Tn5250Terminal *tnterm;
Tn5250Stream *tnstream;
gchar *remotehost;

/*
 *    Program entry point.
 */
int main (int argc, char *argv[])
{
   GtkWidget *app;
   GtkWidget *term;

   /* This is _really_ ugly, but I'm working on other things right now ... */
   if(argc != 2)
      {
	 fprintf (stderr,"Ack! THPTHT!  Usage: gtk-5250 [telnet:]remotehost[:port]\n");
	 return 255;
      }
   remotehost = argv[1];

   gtk_init (&argc, &argv);

   app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   term = gtk5250_terminal_new ();
   gtk_container_add(GTK_CONTAINER (app), term);
   gtk_widget_show (term);

   gtk_signal_connect (GTK_OBJECT (app), "delete_event",
	 GTK_SIGNAL_FUNC(file_exit_callback), NULL);

   gtk_widget_show (app);

   tn5250_settransmap("en"); /* FIXME: */

   tnstream = tn5250_stream_open (remotehost);
   if(tnstream == NULL)
      return 255; /* FIXME: An error message would be nice. */

   tnterm = gtk5250_terminal_get_impl(GTK5250_TERMINAL(term));

   tnsess = tn5250_session_new();
   tn5250_session_set_terminal(tnsess, tnterm);
   tn5250_stream_setenv(tnstream, "TERM", "IBM-3477-FC");

   /* FIXME: Set DEVNAME */

   tnterm->conn_fd = tn5250_stream_socket_handle(tnstream);
   tn5250_session_set_stream(tnsess, tnstream);

   tn5250_session_main_loop(tnsess);
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

   /* FIXME:
   dlg = gnome_about_new ( _("Gnome 5250 Emulator"), VERSION,
	 _("Copyright (C) 1999 Michael Madore"),
	 authors,
	 _("A GNU General Public License 5250 Emulator for Gnome systems."),
	 PKGDATADIR "/tn5250-logo.xpm");

   gtk_widget_show (dlg); */
}

/* vi:set sts=3 sw=3: */
