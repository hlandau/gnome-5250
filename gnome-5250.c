/*
 * gnome-5250 - Gnome 5250 Emulator.
 */

#include <gnome.h>
#include "gtk5250terminal.h"

#define VERSION "0.1"
#define PKGDATADIR "/usr/share/tn5250"

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

/*
 *    Program entry point.
 */
int main (int argc, char *argv[])
{
   GtkWidget *app;
   GtkWidget *term;

   gnome_init ("gnome-5250", VERSION, argc, argv);
   
   app = gnome_app_new ("gnome-5250", N_("Gnome 5250 Emulator"));
   gnome_app_create_menus (GNOME_APP (app), main_menu);

   term = gtk5250_terminal_new ();
   gnome_app_set_contents (GNOME_APP (app), term);
   gtk_widget_show (term);

   gtk_signal_connect (GTK_OBJECT (app), "delete_event",
	 GTK_SIGNAL_FUNC(file_exit_callback), NULL);

   gtk_widget_show (app);

   gtk_main ();
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
   gtk_main_quit ();
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

/* vi:set sts=3 sw=3: */
