#ifndef __GTK5250_PROP_DLG_H__
#define __GTK5250_PROP_DLG_H__

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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <tn5250.h>

#define GTK5250_PROP_DLG_TYPE \
    (gtk5250_prop_dlg_get_type ())
#define GTK5250_PROP_DLG(obj) \
    GTK_CHECK_CAST (obj, GTK5250_PROP_DLG_TYPE, Gtk5250PropDlg)
#define GTK5250_PROP_DLG_CLASS(klass) \
    GTK_CHECK_CLASS_CAST (klass, GTK5250_PROP_DLG_TYPE, Gtk5250PropDlgClass)
#define GTK5250_IS_PROP_DLG(obj) \
    GTK_CHECK_TYPE (obj, GTK5250_PROP_DLG_TYPE)

typedef struct _Gtk5250PropDlg Gtk5250PropDlg;
typedef struct _Gtk5250PropDlgClass Gtk5250PropDlgClass;

struct _Gtk5250PropDlg
{
  GtkDialog	    object;

  Tn5250Config *    config;

  GtkWidget *	    name;
  GtkWidget *	    address;
  GtkWidget *	    port;
  GtkWidget *	    session_name;

  GtkWidget *	    cn_method;
  GtkWidget *	    cn_method_telnet;
  GtkWidget *	    cn_method_debug;

  GtkWidget *	    emulation_type;
  GtkWidget *	    translation_map;

  GtkWidget *	    trace_filename;

  GtkWidget *	    font_selector;
  gchar *	    font_name_80;
  gchar *	    font_name_132;
  GtkWidget *	    font_80;
  gboolean	    font_132_active;

  /* FIXME: Colors */
};

struct _Gtk5250PropDlgClass
{
  GtkDialogClass    parent_class;
};

GtkType		gtk5250_prop_dlg_get_type	  (void);
GtkWidget*	gtk5250_prop_dlg_new		  (void);

void		gtk5250_prop_dlg_set_config	  (Gtk5250PropDlg *This,
						   Tn5250Config *config);
Tn5250Config *	gtk5250_prop_dlg_get_config	  (Gtk5250PropDlg *This);
void		gtk5250_prop_dlg_update_dialog	  (Gtk5250PropDlg *This);
void		gtk5250_prop_dlg_update_config	  (Gtk5250PropDlg *This);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK5250_PROP_DLG_H__ */

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
