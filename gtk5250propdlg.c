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
#include <string.h>
#include "gtk5250propdlg.h"

#define DEFAULT_FONT "-misc-fixed-*-*-*-*-*-*-*-*-c-*-iso8859-1"

static void	  gtk5250_prop_dlg_class_init	(Gtk5250PropDlgClass *klass);
static void	  gtk5250_prop_dlg_init		(Gtk5250PropDlg *obj);

static void	  gtk5250_prop_dlg_destroy	(GtkObject *obj);

static gboolean	  gtk5250_prop_dlg_validate	(Gtk5250PropDlg *This);
static void	  gtk5250_prop_dlg_apply	(Gtk5250PropDlg *This);
static void	  gtk5250_prop_dlg_close	(Gtk5250PropDlg *This);
static void	  gtk5250_prop_dlg_font_80_activate (Gtk5250PropDlg *This);
static void	  gtk5250_prop_dlg_font_132_activate (Gtk5250PropDlg *This);

static GtkWidget* uline_label			(const gchar *str);
static GtkWidget* uline_button			(const gchar *str);
static GtkWidget* menu_item			(GtkWidget *menu,
						 const gchar *str);
static GtkWidget* table_item			(GtkWidget *table,
						 const gchar *str,
						 gint row,
						 GtkWidget *item);
static gchar*	  get_option_menu_label		(GtkWidget *option_menu);

static GtkDialogClass *parent_class = NULL;

/*
 *  Return the Gtk5250PropDlg's type id, registering the type if necessary.
 */
GtkType gtk5250_prop_dlg_get_type ()
{
  static GtkType prop_dlg_type = 0;
  if (!prop_dlg_type)
    {
      GtkTypeInfo type_info =
	{
	  "Gtk5250PropDlg",
	  sizeof (Gtk5250PropDlg),
	  sizeof (Gtk5250PropDlgClass),
	  (GtkClassInitFunc) gtk5250_prop_dlg_class_init,
	  (GtkObjectInitFunc) gtk5250_prop_dlg_init,
	  (GtkArgSetFunc) NULL,
	  (GtkArgGetFunc) NULL
	};
      prop_dlg_type = gtk_type_unique (gtk_dialog_get_type (), &type_info);
    }
  return prop_dlg_type;
}

/*
 *  Initialize an instance of the class.
 */
static void gtk5250_prop_dlg_class_init (Gtk5250PropDlgClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  object_class->destroy = gtk5250_prop_dlg_destroy;

  parent_class = gtk_type_class (gtk_dialog_get_type ());
}

/*
 *  Our actual `constructor'-like doohickey.
 */
GtkWidget* gtk5250_prop_dlg_new (void)
{
  Gtk5250PropDlg *dlg =
    (Gtk5250PropDlg*)gtk_type_new (gtk5250_prop_dlg_get_type ());
  return GTK_WIDGET(dlg);
}

/*
 *  Initialize an instance of the object.
 */
static void gtk5250_prop_dlg_init (Gtk5250PropDlg *This)
{
  GtkWidget *btnbox;
  GtkWidget *btn;
  GtkWidget *notebook, *page, *label, *lbox;
  GtkWidget *menu, *item;
  gchar *font_spacings[] = { "c", "m", NULL };

  g_return_if_fail (GTK5250_IS_PROP_DLG (This));

  This->config = NULL;
  This->font_name_80 = NULL;
  This->font_name_132 = NULL;
  This->font_132_active = FALSE;
  This->font_80 = NULL;

  /* This will be updated from the session object later. */
  gtk_window_set_title (GTK_WINDOW (This), "Session Properties");

  /* Add the `Apply' and `Close' buttons. */
  btnbox = gtk_hbutton_box_new ();
  gtk_container_add (
      GTK_CONTAINER (GTK_DIALOG (This)->action_area),
      btnbox);

  btn = uline_button ("_Apply");
  GTK_WIDGET_SET_FLAGS (btn, GTK_CAN_DEFAULT | GTK_HAS_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (btn), "clicked",
      GTK_SIGNAL_FUNC (gtk5250_prop_dlg_apply),
      GTK_OBJECT (This));
  gtk_container_add (GTK_CONTAINER (btnbox), btn);
  gtk_widget_show (btn);

  btn = uline_button ("Close");
  GTK_WIDGET_SET_FLAGS (btn, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (btn), "clicked",
      GTK_SIGNAL_FUNC (gtk5250_prop_dlg_close),
      GTK_OBJECT (This));
  gtk_container_add (GTK_CONTAINER (btnbox), btn);
  gtk_widget_show (btn);
  gtk_widget_show (btnbox);

  /* Add the notebook. */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (This)->vbox),
      notebook, TRUE, TRUE, 2);

  /* Add the 'Connection' page to the notebook. */
  label = uline_label ("_Connection");
  page = gtk_table_new (5, 2, FALSE);
  This->name = table_item (page, "_Name", 0, gtk_entry_new ());
  lbox = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  This->cn_method_telnet = menu_item (menu, "telnet");
  This->cn_method_debug = menu_item (menu, "debug");
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lbox), menu);
  This->cn_method = table_item (page, "Connection _Method", 1, lbox);
  This->address = table_item (page, "_Host Address", 2,
      gtk_entry_new ());
  This->port = table_item (page, "Host _Port", 3, gtk_entry_new ());
  This->session_name = table_item (page, "_Session Name", 4, gtk_entry_new ());
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);
  gtk_widget_show (label);

  /* Add the 'Emulation' page to the notebook. */
  label = uline_label ("_Emulation");
  page = gtk_table_new (5, 2, FALSE);
  lbox = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  menu_item (menu, "IBM-3477-FC (27x132 color)");
  menu_item (menu, "IBM-3477-FG (27x132 monochrome)");
  menu_item (menu, "IBM-3180-2 (27x132 monochrome)");
  menu_item (menu, "IBM-3179-2 (24x80 color)");
  menu_item (menu, "IBM-3196-A1 (24x80 monochrome)");
  menu_item (menu, "IBM-5292-2 (24x80 color)");
  menu_item (menu, "IBM-5291-1 (24x80 monochrome)");
  menu_item (menu, "IBM-5251-11 (24x80 monochrome)");
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lbox), menu);
  This->emulation_type = table_item (page, "_Emulation Type", 0, lbox);
  table_item (page, NULL, 1, 
      gtk_check_button_new_with_label ("Supress Beeps"));
  lbox = gtk_option_menu_new ();
  menu = gtk_menu_new ();
    {
      Tn5250CharMap *map = tn5250_transmaps;
      while (map->name != NULL)
	{
	  menu_item (menu, map->name);
	  map++;
	}
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lbox), menu);
  This->translation_map = table_item (page, "Translation _Map", 2, lbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);
  gtk_widget_show (label);

  /* Add the 'Fonts' page to the notebook. */
  label = uline_label ("_Fonts");
  page = gtk_vbox_new (FALSE,0);
  This->font_selector = gtk_font_selection_new ();
  gtk_box_pack_end (GTK_BOX (page), This->font_selector, FALSE, TRUE, 2);
  gtk_widget_show (This->font_selector);
  lbox = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  This->font_80 = item = menu_item (menu, "24x80 Display Font");
  gtk_signal_connect_object (GTK_OBJECT (item), "activate",
      GTK_SIGNAL_FUNC (gtk5250_prop_dlg_font_80_activate), GTK_OBJECT (This));
  item = menu_item (menu, "27x132 Display Font");
  gtk_signal_connect_object (GTK_OBJECT (item), "activate",
      GTK_SIGNAL_FUNC (gtk5250_prop_dlg_font_132_activate), GTK_OBJECT (This));
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lbox), menu);
  gtk_box_pack_start (GTK_BOX (page), lbox, TRUE, TRUE, 2);
  gtk_widget_show (lbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);
  gtk_widget_show (label);
  gtk_font_selection_set_filter (GTK_FONT_SELECTION (This->font_selector),
      GTK_FONT_FILTER_BASE, GTK_FONT_ALL, 
      NULL, NULL, NULL, NULL, font_spacings, NULL);

  /* Add the 'Colors' page to the notebook. */
  label = uline_label ("C_olors");
  page = gtk_vbox_new (FALSE, 0);
  lbox = gtk_color_selection_new ();
  gtk_box_pack_end (GTK_BOX (page), lbox, TRUE, TRUE, 2);
  gtk_widget_show (lbox);
  lbox = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  menu_item (menu, "Background Color");
  menu_item (menu, "White");
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lbox), menu);
  gtk_box_pack_start (GTK_BOX (page), lbox, FALSE, FALSE, 2);
  gtk_widget_show (lbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);
  gtk_widget_show (label);

  gtk_widget_show (notebook);
}

/*
 *  Set the property dialog's config object.
 */
void gtk5250_prop_dlg_set_config (Gtk5250PropDlg *This, Tn5250Config *config)
{
  Tn5250Config *old_config = This->config;
  This->config = config;
  if (This->config != NULL)
    tn5250_config_ref (This->config);
  if (old_config != NULL)
    tn5250_config_unref (old_config);
  if (This->config != NULL)
    gtk5250_prop_dlg_update_dialog (This);
}

/*
 *  Retreive the property dialog's config object.
 */
Tn5250Config *gtk5250_prop_dlg_get_config (Gtk5250PropDlg *This)
{
  return This->config;
}

/*
 *  Update the dialog from the information in the configuration object.
 */
void gtk5250_prop_dlg_update_dialog (Gtk5250PropDlg *This)
{
  static struct stream_type
    {
      char *prefix;
      char *type;
    }
  stream_types[] =
    {
	{ "debug:", "debug" },
	{ "telnet:", "telnet" },
	{ "tn5250:", "tn5250" },
	{ NULL, NULL }
    }, *stream_type_iter;
  const gchar *s;
  g_return_if_fail (This->config != NULL);

  /* Parse the host into type, address, and port. */
  s = tn5250_config_get (This->config, "host");
  if (s != NULL && strcmp (s, ""))
    {
      gchar *cn_method = NULL, *address = NULL, *port = NULL;
      stream_type_iter = stream_types;
      while (stream_type_iter->prefix != NULL)
	{
	  if (strlen (s) >= strlen (stream_type_iter->prefix)
	      && !memcmp (s, stream_type_iter->prefix,
		strlen (stream_type_iter->prefix)))
	    {
	      cn_method = stream_type_iter->type;
	      break;
	    }
	  stream_type_iter++;
	}
      
      if (cn_method == NULL)
	cn_method = "telnet";
      else
	s += strlen (stream_type_iter->prefix);
      
      address = g_strdup (s);
      if (!strcmp (cn_method, "telnet") && strrchr (s, ':'))
	{
	  port = g_strdup (strrchr (s, ':')+1);
	  *strrchr (address, ':') = '\0';
	}
      else
	port = g_strdup ("");
  
      gtk_entry_set_text (GTK_ENTRY (This->address), address);
      gtk_entry_set_text (GTK_ENTRY (This->port), port);

      if (!strcmp (cn_method, "telnet"))
	gtk_menu_item_activate (GTK_MENU_ITEM (This->cn_method_telnet));
      else if (!strcmp (cn_method, "debug"))
	gtk_menu_item_activate (GTK_MENU_ITEM (This->cn_method_debug));
      else
	g_warning ("Bad cn_method (%s)", cn_method);

      if (address != NULL)
	g_free (address);
      if (port != NULL)
	g_free (port);
    }

  /* Session name */
  gtk_menu_item_activate (GTK_MENU_ITEM (This->font_80));
  s = tn5250_config_get (This->config, "env.DEVNAME");
  if (s == NULL)
    s = "";
  gtk_entry_set_text (GTK_ENTRY (This->session_name), s);

  /* Font names (for 80 and 132 column modes)... */
  if (This->font_name_80 != NULL)
    g_free (This->font_name_80);
  s = tn5250_config_get (This->config, "font_80");
  if (s == NULL || !strcmp (s, ""))
    This->font_name_80 = g_strdup (DEFAULT_FONT);
  else
    This->font_name_80 = g_strdup (s);

  if (This->font_name_132 != NULL)
    g_free (This->font_name_132);
  s = tn5250_config_get (This->config, "font_132");
  if (s == NULL || !strcmp (s, ""))
    This->font_name_132 = g_strdup (This->font_name_80);
  else
    This->font_name_132 = g_strdup (s);

  /* Update selected font. */
  gtk_font_selection_set_font_name (GTK_FONT_SELECTION (This->font_selector),
      This->font_name_80);
  This->font_132_active = FALSE;
}

/*
 *  Update the configuration object from the dialog (e.g. apply).
 */
void gtk5250_prop_dlg_update_config (Gtk5250PropDlg *This)
{
}

/*
 *  Destroy the prop_dlg object.
 */
static void gtk5250_prop_dlg_destroy (GtkObject *obj)
{
  Gtk5250PropDlg *dlg = (Gtk5250PropDlg*)obj;
 
  g_return_if_fail (!GTK5250_IS_PROP_DLG (obj));
  dlg = GTK5250_PROP_DLG (obj);

  if (dlg->config != NULL)
    {
      tn5250_config_unref (dlg->config);
      dlg->config = NULL;
    }
  if (dlg->font_name_80 != NULL)
    {
      g_free (dlg->font_name_80);
      dlg->font_name_80 = NULL;
    }
  if (dlg->font_name_132 != NULL)
    {
      g_free (dlg->font_name_132);
      dlg->font_name_132 = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (obj);
}

/*
 *  Validate the values entered into the property dialog.
 */
static gboolean gtk5250_prop_dlg_validate (Gtk5250PropDlg *This)
{
  /* FIXME: Implement. */
  return TRUE;
}

/*
 *  Signal handler for the `Apply' button on the property dialog.
 */
static void gtk5250_prop_dlg_apply (Gtk5250PropDlg *This)
{
  gchar *str;

  g_return_if_fail (This != NULL);
  g_return_if_fail (GTK5250_IS_PROP_DLG (This));

  if (!gtk5250_prop_dlg_validate (This))
    return;

  gtk5250_prop_dlg_update_config (This);
}

/*
 *  Signal handler for the `Close' button on the property dialog.
 */
static void gtk5250_prop_dlg_close (Gtk5250PropDlg *This)
{
  g_return_if_fail (GTK5250_IS_PROP_DLG (This));
  g_return_if_fail (This != NULL);

  gtk_widget_hide (GTK_WIDGET (This));
  /* FIXME: Trigger a signal. */
}

/*
 *  Convenience function to create a label with an accelerator.  Sort of.
 */
static GtkWidget *uline_label (const gchar *str)
{
  GtkWidget *label = gtk_label_new ("");
  gtk_label_parse_uline (GTK_LABEL (label), str);
  return label;
}

static GtkWidget *uline_button (const gchar *str)
{
  GtkWidget *btn = gtk_button_new (), *lbl;
  gtk_container_add (GTK_CONTAINER (btn), lbl = uline_label (str));
  gtk_widget_show (lbl);
  return btn;
}

static GtkWidget *menu_item (GtkWidget *menu, const gchar *str)
{
  GtkWidget *item = gtk_menu_item_new_with_label (str);
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  return item;
}

static GtkWidget *table_item (GtkWidget *table, const gchar *str, gint row,
    GtkWidget *item)
{
  if (str != NULL)
    {
      GtkWidget *label = uline_label (str);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, row+0, row+1, 0, 0,
	  3, 3);
      gtk_widget_show (label);
    }
  gtk_table_attach (GTK_TABLE (table), item, 1, 2, row+0, row+1, 
      GTK_EXPAND | GTK_FILL, 0, 3, 3);
  gtk_widget_show (item);
  return item;
}

static gchar *get_option_menu_label (GtkWidget *option_menu)
{
  GtkArg arg;

  arg.type = 0;
  arg.name = "label";
  gtk_object_arg_get (GTK_OBJECT (option_menu), &arg, NULL);
  return GTK_VALUE_STRING(arg);
}

/*
 *  Happens when we choose '24x80' from the font option menu.
 */
static void gtk5250_prop_dlg_font_80_activate (Gtk5250PropDlg *This)
{
  if (This->font_132_active)
    {
      if (This->font_name_132 != NULL)
	g_free (This->font_name_132);
      This->font_name_132 = g_strdup (gtk_font_selection_get_font_name (
	  GTK_FONT_SELECTION (This->font_selector)
	  ));
      gtk_font_selection_set_font_name (
	  GTK_FONT_SELECTION (This->font_selector), This->font_name_80
	  );
      This->font_132_active = FALSE;
    }
}

/*
 *  Happens when we choose '27x132' from the font option menu.
 */
static void gtk5250_prop_dlg_font_132_activate (Gtk5250PropDlg *This)
{
  if (!This->font_132_active)
    {
      if (This->font_name_80 != NULL)
	g_free (This->font_name_80);
      This->font_name_80 = g_strdup (gtk_font_selection_get_font_name (
	  GTK_FONT_SELECTION (This->font_selector)
	  ));
      gtk_font_selection_set_font_name (
	  GTK_FONT_SELECTION (This->font_selector), This->font_name_132
	  );
      This->font_132_active = TRUE;
    }
}

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
