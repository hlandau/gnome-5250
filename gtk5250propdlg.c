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

static void	  gtk5250_prop_dlg_class_init	(Gtk5250PropDlgClass *klass);
static void	  gtk5250_prop_dlg_init		(Gtk5250PropDlg *obj);

static void	  gtk5250_prop_dlg_destroy	(GtkObject *obj);

static gboolean	  gtk5250_prop_dlg_validate	(Gtk5250PropDlg *This);
static void	  gtk5250_prop_dlg_apply	(Gtk5250PropDlg *This);
static void	  gtk5250_prop_dlg_close	(Gtk5250PropDlg *This);

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
  GtkWidget *menu;

  g_return_if_fail (GTK5250_IS_PROP_DLG (This));

  This->font_name_80 = NULL;
  This->font_name_132 = NULL;

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
  menu_item (menu, "telnet");
  menu_item (menu, "debug");
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
  menu_item (menu, "24x80 Display Font");
  menu_item (menu, "27x132 Display Font");
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lbox), menu);
  gtk_box_pack_start (GTK_BOX (page), lbox, TRUE, TRUE, 2);
  gtk_widget_show (lbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);
  gtk_widget_show (label);

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
 *  Destroy the prop_dlg object.
 */
static void gtk5250_prop_dlg_destroy (GtkObject *obj)
{
  Gtk5250PropDlg *dlg = (Gtk5250PropDlg*)obj;
 
  g_return_if_fail (!GTK5250_IS_PROP_DLG (obj));
  dlg = GTK5250_PROP_DLG (obj);

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


  /* FIXME: Trace Filename */
  /* FIXME: Trace Append Pid */
  /* FIXME: Font Name 80 */
  /* FIXME: Font Name 132 */

  /* FIXME: Colors */
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

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
