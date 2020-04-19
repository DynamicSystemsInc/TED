/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

  The contents of this file are subject to the terms of the
  GNU General Public License version 2 (the "License")
  as published by the Free Software Foundation. You may not use
  this file except in compliance with the License.

  You should have received a copy of the License along with this
  file; see the file COPYING.  If not,you can obtain a copy
  at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html or by writing
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA. See the License for specific language
  governing permissions and limitations under the License.
*/

#include <config.h>
#include <gtk/gtk.h>
#undef  GTK_DISABLE_DEPRECATED
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/window.h>

#include <libgnometsol/label_builder.h>

#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include <auth_attr.h>
#include <secdb.h>
#include <stdio.h>
#include <string.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <zone.h>

#define DEFAULT_HEIGHT	400
#define DEFAULT_WIDTH	600

#define MAJOR_LEFT_JUSTIFY	5
#define MINOR_LEFT_JUSTIFY	15
#define YSPACING		5
#define UNDO_IMAGE_HEIGHT	20

enum {
	ADD_WINDOW,
	PROPS_WINDOW
};
int window_type;

enum {
	AUTHORIZED_USERS,
	NO_USERS,
	ALL_USERS,
	SAME_AS_TP
};

#if 0
/* Problems with my system configuration.  should be fixed shortly 
   but don't include until it is fixed. */
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <constraint-scaling.h>
#endif

#include "devmgr-props-dialog.h"

/* need to add the property global variables here */
struct _DevMgrPropsDialogDetails {
	GtkWidget 	*name;
	GtkWidget	*dtype;
	GtkEntry	*min;
	GtkImage	*min_img;
	GtkEntry	*max;
	GtkImage	*max_img;
	GtkEntry	*clean;
	GtkTextBuffer	*map;
	GtkWidget	*from;
	GtkWidget	*tpath;
	GtkWidget	*nontpath;
	GtkOptionMenu	*by;
	GtkWidget	*by_mitems[4];
	GtkExpander	*expander;
	GtkWidget	*exp_hbox;
	GtkEntry	*auth;
	GtkListStore	*required;
	GtkListStore	*not_required;
	GtkWidget	*desc;
	GtkWidget	*auth_add;
	GtkWidget	*auth_remove;
#ifdef ENABLE_DEALLOCATION
	GtkWidget	*boot;
	GtkWidget	*logout;
#endif /* ENABLE_DEALLOCATION */
};

static void devmgr_props_dialog_class_init    (DevMgrPropsDialogClass *klass);
static void devmgr_props_dialog_instance_init (DevMgrPropsDialog *object); 

static GtkDialogClass *parent_class = NULL;


GtkWidget *auth_add, *auth_remove;
GtkWidget *desc_label;
GtkTable *auth_table;
gboolean ugly_showing_lbuilder_global_which_sucks_fix_me = FALSE;
#ifdef CONSOLE
extern FILE *console;
#endif /* CONSOLE */

static void                                                             
devmgr_props_dialog_class_init_trampoline (gpointer klass, gpointer data)
{                                                                       
	parent_class = (GtkDialogClass *) g_type_class_ref (GTK_TYPE_DIALOG);
        devmgr_props_dialog_class_init ((DevMgrPropsDialogClass *)klass);     
}                                                               

GType
devmgr_props_dialog_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
		    sizeof (DevMgrPropsDialogClass),
	 	    NULL,               /* base_init */ 
		    NULL,               /* base_finalize */
		    devmgr_props_dialog_class_init_trampoline,
		    NULL,               /* class_finalize */
		    NULL,               /* class_data */
		    sizeof (DevMgrPropsDialog),
		    0,                  /* n_preallocs */
		    (GInstanceInitFunc) devmgr_props_dialog_instance_init
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "DevMgrPropsDialogType",
					       &info, 0);
        }
        return type;
}

static void
devmgr_props_dialog_finalize (GObject *object)
{
	DevMgrPropsDialog *devmgr_props_dialog = DEVMGR_PROPS_DIALOG (object);
	DevMgrPropsDialogDetails *details = devmgr_props_dialog->details;

	/* free all my vars then the details structure */
	g_free (details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

gint props_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gboolean ret = FALSE;
	
	gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
	
	return ( ret );
}

static gboolean
reset_expose (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	gboolean ret = FALSE;

	/* need to display the label and the undo stock image */
	
	return ( ret );
}

void update_image_widget(GtkDialog *dialog, GtkImage *img, char *devlabel)
{
	GdkGC *gc;
	GdkGCValues gc_values;
	int gc_mask = 0;
	GdkColor color;
	GdkColormap *colormap;
	GdkPixmap *pix;
	GdkScreen *screen;
	
	screen = gtk_window_get_screen(GTK_WINDOW(dialog));
	gc = gdk_gc_new(gdk_screen_get_root_window(screen));
	
	g_object_get(img, "pixmap", &pix, NULL);

	colormap = gtk_widget_get_colormap(GTK_WIDGET(img));
	if (gdk_color_parse("black", &color))
	  if (gdk_colormap_alloc_color(colormap, &color, TRUE, TRUE))
	    gdk_gc_set_foreground(gc, &color);
	gdk_draw_rectangle(GDK_DRAWABLE(pix), gc, TRUE, 0, 0, 16, 16);
	
	if (devlabel != NULL)
	{
	  m_label_t *mlabel = NULL;
	  char *color_str = NULL;
	  char *lookup_label;
	  int e;
	  
	  if (strcmp(devlabel, "admin_low") == 0)
	    lookup_label = GLOBAL_ZONENAME;
	  else if (strcmp(devlabel, "admin_high") == 0)
	    lookup_label = GLOBAL_ZONENAME;
	  else
	    lookup_label = devlabel;
	  str_to_label(lookup_label, &mlabel, MAC_LABEL, L_NO_CORRECTION, &e);
	  if (mlabel == NULL)
	    mlabel = getzonelabelbyname(lookup_label);
	  if (mlabel != NULL)
	  {
	    label_to_str(mlabel, &color_str, M_COLOR, DEF_NAMES);
	    label_to_str(mlabel, &color_str, M_COLOR, LONG_NAMES);
	  }
	  if (gdk_color_parse(color_str?color_str:"white", &color))
	    if (gdk_colormap_alloc_color(colormap, &color, TRUE, TRUE))
	      gdk_gc_set_foreground(gc, &color);
	  gdk_draw_rectangle(GDK_DRAWABLE(pix), gc, TRUE, 1, 1, 14, 14);
	}
	g_object_set(img, "pixmap", pix, NULL);
}

GtkEntry *centry;
GtkImage *cimage;
char *maxhex = NULL, *minhex = NULL, **hexlabel;

void
lbuilder_response_cb (GtkDialog *dialog, gint id, gpointer data)
{
  int error;
  bslabel_t *sl = NULL;
  char *label;
  GtkDialog *pdialog;
  DevMgrPropsDialogDetails *details;
  m_label_t *workspace_sl;
  zoneid_t zid;
  char zonename[ZONENAME_MAX];
  char *string;
  m_label_t *ll = NULL;
  char *tlabel;
  int e;
  /* 
   * Stops the GNOME_LABEL_BUILDER cast calling 
   * gnome_label_builder_get_type() directly
   */
#ifndef GNOME_TYPE_LABLE_BUILDER
#define GNOME_TYPE_LABEL_BUILDER (gnome_label_builder_get_type ())
#endif
  GnomeLabelBuilder *lbuilder = GNOME_LABEL_BUILDER (dialog);
  
  pdialog = GTK_DIALOG (data);
  details = DEVMGR_PROPS_DIALOG (pdialog)->details;
  switch (id) {
    case GTK_RESPONSE_OK:
         g_object_get (G_OBJECT (lbuilder), "sl", &sl, NULL);
         if (label_to_str (sl, &label, M_INTERNAL, LONG_NAMES) < 0)
         {
           gtk_widget_destroy (GTK_WIDGET (lbuilder));
           ugly_showing_lbuilder_global_which_sucks_fix_me = FALSE;
           return ;
         }
         *hexlabel = g_strdup(label);
#ifdef CONSOLE
	fprintf(console, "%s:%d:string = %s\n", __FILE__, __LINE__, string?string:"NULL");
#endif /* CONSOLE */
         if (hexlabel && *hexlabel) 
         {
             str_to_label(*hexlabel, &ll, MAC_LABEL, L_NO_CORRECTION, &e); 
             label_to_str(ll, &string, M_LABEL, LONG_NAMES);
         } else
             string = strdup("ADMIN_LOW");
         if (string && strcmp(string, "ADMIN_LOW") == 0)
         {
#ifdef CONSOLE
	fprintf(console, "%s:%d:string == ADMIN_LOW\n", __FILE__, __LINE__);
#endif /* CONSOLE */
           free(string);
           string = malloc(sizeof(char *) * strlen("admin_low")+1);
           if (centry == DEVMGR_PROPS_DIALOG (pdialog)->details->min)
             strcpy(string, "admin_low"); 
           else
             strcpy(string, "admin_high");
         }
         /* FIXME - remember to free up stuff */
	 if (label != NULL) {
	   /* Update the Min-label and image */
	   str_to_label(label, &workspace_sl, MAC_LABEL, L_NO_CORRECTION, &error);
           if ((zid = getzoneidbylabel(workspace_sl)) > 0) {

             if (getzonenamebyid(zid, zonename, sizeof (zonename)) == -1) {
               strcpy(zonename, label);
             }
           } else {
               strcpy(zonename, label);
           }

	   strcpy(zonename, label);
	   g_object_set(centry, "text", string, NULL);
	   update_image_widget(pdialog, cimage, zonename);
 	  }
          gtk_widget_destroy (GTK_WIDGET (lbuilder));
 	  ugly_showing_lbuilder_global_which_sucks_fix_me = FALSE;
 	  free(string);
          break;
    case GTK_RESPONSE_HELP:
         /* show help and return control */
 	 break;
    case GTK_RESPONSE_CANCEL:
         /* We dont want to change the workspace label so bye-bye */
         gtk_widget_destroy (GTK_WIDGET (lbuilder));
         ugly_showing_lbuilder_global_which_sucks_fix_me = FALSE;
         break;
    default:
         /* We shouldn't really have got here */
         break;	
  }

  return;
}

extern char *wnck_workspace_get_label (WnckWorkspace *);
extern int wnck_workspace_get_label_range(WnckWorkspace *space,
					  char **min_label,
					  char **max_label);

static void
min_cb(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET(data);
	gint result;
	gboolean value;
	int i;
	int error = 0;
	char *cur_ws_label = NULL;
	char *lower_bound = NULL;
	char *upper_bound = NULL;

	m_label_t *ws_sl = NULL;
	m_label_t *lower_sl = NULL;
	m_label_t *upper_clear = NULL;
	GtkWidget *lbuilder = NULL;
	WnckWorkspace *wspace = NULL;
	GdkScreen *screen = gtk_widget_get_screen ( GTK_WIDGET(dialog) );;  
	WnckScreen *wscreen = wnck_screen_get ( gdk_screen_get_number (screen) );
  
	lower_bound = NULL;
	upper_bound = NULL;

	wspace = wnck_screen_get_active_workspace (wscreen);
	error = wnck_workspace_get_label_range (wspace, &lower_bound, &upper_bound);
	if (error != 0)
	    return;

	/* Convert the lower and upper bounds to internal binary labels */
	if (str_to_label (lower_bound, &lower_sl, MAC_LABEL, L_DEFAULT,
	    	&error) < 0) {
        	g_warning ("Workspace has invalid label range minimum label");
		g_free (lower_bound);
		g_free (upper_bound);
		return;
	}
	g_free (lower_bound);

	if (str_to_label (upper_bound, &upper_clear, USER_CLEAR, L_DEFAULT, 
	    	&error) < 0) {
		g_warning ("Workspace has invalid label range");
		g_free (upper_bound);
		m_label_free (lower_sl);
		return;
	}
	g_free (upper_bound);

	/* Get the current workspace label. */
	cur_ws_label = wnck_workspace_get_label (wspace);
	if (cur_ws_label != NULL) {
		/* Convert the workspace's current label to binary type */
    		if (str_to_label (cur_ws_label, &ws_sl, MAC_LABEL, L_DEFAULT,
		    	&error) < 0) {
			g_warning ("Workspace has an invalid label");
			g_free (cur_ws_label);
			return;
		}
	} else {
		g_warning ("No workspace label - defaulting to lowest in range");
		m_label_dup (&ws_sl, lower_sl);
	}

	lbuilder = gnome_label_builder_new(_("Device Allocation: Set Min Label"),
  				   upper_clear, lower_sl,
  				   LBUILD_MODE_SL);

  				   
	centry = DEVMGR_PROPS_DIALOG (dialog)->details->min;
	cimage = DEVMGR_PROPS_DIALOG (dialog)->details->min_img;
	hexlabel = &minhex;
	g_signal_connect (G_OBJECT (lbuilder), "response",
  	    G_CALLBACK (lbuilder_response_cb), (gpointer) dialog);
  	    
	gtk_widget_show_all (lbuilder);
	ugly_showing_lbuilder_global_which_sucks_fix_me = TRUE;

	/* GAH, why do I have to do this after the show? */
	g_object_set (G_OBJECT (lbuilder), "sl", &ws_sl, NULL);

/*      devmgr core dumps if these are freed although they
 *	need to be freed.
 */
/*
	m_label_free (ws_sl);
	m_label_free (lower_sl);
	m_label_free (upper_clear);
	g_free (cur_ws_label);
*/
}

static void
max_cb(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog = GTK_WIDGET(data);
	gint result;
	gboolean value;
	int i;
	int error = 0;
	char *cur_ws_label = NULL;
	char *lower_bound = NULL;
	char *upper_bound = NULL;

	m_label_t *ws_sl = NULL;
	m_label_t *lower_sl = NULL;
	m_label_t *upper_clear = NULL;
	GtkWidget *lbuilder = NULL;
	WnckWorkspace *wspace = NULL;
	GdkScreen *screen = gtk_widget_get_screen ( GTK_WIDGET(dialog) );;  
	WnckScreen *wscreen = wnck_screen_get ( gdk_screen_get_number (screen) );
  
	lower_bound = NULL;
	upper_bound = NULL;

	wspace = wnck_screen_get_active_workspace (wscreen);
	error = wnck_workspace_get_label_range (wspace, &lower_bound, &upper_bound);
	if (error != 0)
	    return;

	/* Convert the lower and upper bounds to internal binary labels */
	if (str_to_label (lower_bound, &lower_sl, MAC_LABEL, L_DEFAULT,
	    	&error) < 0) {
        	g_warning ("Workspace has invalid label range minimum label");
		g_free (lower_bound);
		g_free (upper_bound);
		return;
	}
	g_free (lower_bound);

	if (str_to_label (upper_bound, &upper_clear, USER_CLEAR, L_DEFAULT, 
	    	&error) < 0) {
		g_warning ("Workspace has invalid label range");
		g_free (upper_bound);
		m_label_free (lower_sl);
		return;
	}
	g_free (upper_bound);

	/* Get the current workspace label. */
	cur_ws_label = wnck_workspace_get_label (wspace);
	if (cur_ws_label != NULL) {
		/* Convert the workspace's current label to binary type */
    		if (str_to_label (cur_ws_label, &ws_sl, MAC_LABEL, L_DEFAULT,
		    	&error) < 0) {
			g_warning ("Workspace has an invalid label");
			g_free (cur_ws_label);
			return;
		}
	} else {
		g_warning ("No workspace label - defaulting to lowest in range");
		m_label_dup (&ws_sl, lower_sl);
	}

	lbuilder = gnome_label_builder_new(_("Device Allocation: Set Max Label"),
  				   upper_clear, lower_sl,
  				   LBUILD_MODE_SL);
		   
	centry = DEVMGR_PROPS_DIALOG (dialog)->details->max;
	cimage = DEVMGR_PROPS_DIALOG (dialog)->details->max_img;
	hexlabel = &maxhex;
	g_signal_connect (G_OBJECT (lbuilder), "response",
  	    G_CALLBACK (lbuilder_response_cb), (gpointer) dialog);
  	    
	gtk_widget_show_all (lbuilder);
	ugly_showing_lbuilder_global_which_sucks_fix_me = TRUE;

	/* GAH, why do I have to do this after the show? */
	g_object_set (G_OBJECT (lbuilder), "sl", &ws_sl, NULL);
	
/*      devmgr core dumps if these are freed although they
 *	need to be freed.
 */
/*
	m_label_free (ws_sl);
	m_label_free (lower_sl);
	m_label_free (upper_clear);
	g_free (cur_ws_label);
*/
}

char *clean_program_str;
GtkEntry *clean_entry;

static void
file_ok_sel(GtkWidget *w, GtkFileSelection *fs)
{
	if (clean_program_str != NULL)
		g_free(clean_program_str);
		
	clean_program_str = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));
	
	if (clean_program_str != NULL)
		gtk_entry_set_text(clean_entry, clean_program_str);
}

void filew_quit(GtkWidget *widget, gpointer data)
{
	gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
}

/* popup file selection widget for open button... */
void find_clean_program(GtkWidget *widget, gpointer data)
{
	GtkWidget *filew = gtk_file_selection_new("Clean Program");
	g_signal_connect(G_OBJECT(filew), "destroy",
		G_CALLBACK(filew_quit), NULL);

	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filew)->ok_button),
		"clicked", G_CALLBACK(file_ok_sel), (gpointer)filew);
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button),
		"clicked", G_CALLBACK(gtk_widget_destroy), G_OBJECT(filew));
	
	gtk_widget_show(GTK_WIDGET(filew));
	switch (gtk_dialog_run(GTK_DIALOG(filew)))
	{
	    case GTK_RESPONSE_HELP:
	        /* Popup help application */
		break;
 	    case GTK_RESPONSE_CANCEL:
                break;
            case GTK_RESPONSE_OK:
                if (clean_program_str != NULL)
                {
                	gtk_entry_set_text(clean_entry, clean_program_str);
                	g_free(clean_program_str);
                	clean_program_str = NULL;
                }
                break;
            default:
                break;
	}
	gtk_widget_hide_all(GTK_WIDGET(filew));
	gtk_widget_destroy(GTK_WIDGET(filew));
}

enum
{
	ROLE_COL,
	ROLE_DESC_COL,
	NUM_ROLE_COLS
};	

GtkTreeView *req_view, *not_req_view;
static void remove_required_from_not_required(GtkListStore *, char *);
void fill_not_required_list(GtkListStore *);

void auth_add_callback(GtkWidget *widget, gpointer data)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(not_req_view);
    GtkTreeModel *model = gtk_tree_view_get_model(not_req_view);
    GtkTreeIter iter;
    DevMgrPropsDialogDetails *details = data;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	GValue v = {0,};
	char *authname;
	char *authdesc;
	GtkListStore *reqlist;
	GtkListStore *notreq;
	GtkTreeIter niter;
	char *auth_value;
	char *new_value;
	int len;
	
	gtk_tree_model_get_value(model, &iter, ROLE_COL, &v);
	authname = g_strdup(g_value_get_string(&v));
	g_value_unset(&v);
	gtk_tree_model_get_value(model, &iter, ROLE_DESC_COL, &v);
	authdesc = g_strdup(g_value_get_string(&v));
	g_value_unset(&v);
	
 	g_object_get(req_view, "model", &reqlist, NULL);
	gtk_list_store_append(reqlist, &niter);
	gtk_list_store_set(reqlist, &niter,
		ROLE_COL,	authname,
		ROLE_DESC_COL,	authdesc,
		-1);
	
	g_object_get(details->auth, "text", &auth_value, NULL);
	if (auth_value != NULL && strcmp(auth_value, "") != 0)
	{
		len = strlen(auth_value)+strlen(authname)+strlen(",")+1;
		new_value = (char *)malloc(len);
		sprintf(new_value, "%s,%s", auth_value, authname);
	} else
		new_value = authname;
	g_object_set(details->auth, "text", new_value, NULL);
	
	g_object_get(not_req_view, "model", &notreq, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
	gtk_list_store_clear(notreq);
	fill_not_required_list(notreq);
	remove_required_from_not_required(GTK_LIST_STORE(notreq), new_value);
	
	if (new_value != authname)
	  free(new_value);
	g_free(authname);
	g_free(authdesc);
	gtk_widget_set_sensitive(GTK_WIDGET(auth_add), FALSE);
   }
}


void auth_remove_callback(GtkWidget *widget, gpointer data)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(req_view);
    GtkTreeModel *model = gtk_tree_view_get_model(req_view);
    GtkTreeIter iter;
    DevMgrPropsDialogDetails *details = data;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	GValue v1 = {0,};
	GValue v2 = {0,};
	char *authname;
	char *authdesc;
	GtkListStore *nreqlist;
	GtkListStore *reqlist;
	GtkTreeIter niter;
	char *auth_value;
	char *new_value;
	char *t;
	
	gtk_tree_model_get_value(model, &iter, ROLE_COL, &v1);
	gtk_tree_model_get_value(model, &iter, ROLE_DESC_COL, &v2);
	authname = g_strdup(g_value_get_string(&v1));
	authdesc = g_strdup(g_value_get_string(&v2));

 	g_object_get(not_req_view, "model", &nreqlist, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
	gtk_list_store_clear(nreqlist);
	fill_not_required_list(nreqlist);
	
	g_object_get(req_view, "model", &reqlist, NULL);
	gtk_list_store_remove(reqlist, &iter);
	
	g_object_get(details->auth, "text", &auth_value, NULL);
	/* find the authorization that was removed */
	if ((t = strstr(auth_value, authname)) != NULL)
	{
	    char *p1, *p2;
	    int len = 0;
	    
	    /* was that the first one in the list */
	    if (t == auth_value)
	    {
		p1 = t+strlen(authname);
		/* are there more in the list */
		if (*p1 == ',')
		    p1++;
		len = strlen(p1);
		p2 = NULL;
	    } 
	    else
	    {
	    	/* not the first one */
	        p1 = auth_value;
	        /* step over the authname */
	        p2 = t+strlen(authname);
	        /* step back over the "," characters */
	        t--;
	        /* end the first string p1 */
	        *t = '\0';
	        len = strlen(p1)+strlen(p2);
	    }
	    new_value = (char *)malloc(len+1);
	    sprintf(new_value, "%s%s", p1, p2?p2:"");
	    g_object_set(details->auth, "text", new_value, NULL);
	    remove_required_from_not_required(GTK_LIST_STORE(nreqlist), new_value);
	    free(new_value);
	}
	else	
	    /* not found, really impossible */
	    g_object_set(details->auth, "text", "", NULL);

	g_free(authname);
	g_free(authdesc);
	gtk_widget_set_sensitive(GTK_WIDGET(auth_remove), FALSE);
   }
}

gboolean props_remove_selection_callback(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
    GtkWidget *label = GTK_WIDGET(data);
    GtkTreeIter iter;
    GtkTreeModel *lm = model;
    GtkLabel *desc;
    char *s;
    GValue value = {0,};
    char str[256];
    char *tmp;
    char *new_str = NULL;
    char *part;
    int count = 0;

    gtk_widget_set_sensitive(GTK_WIDGET(auth_remove), TRUE);
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
    	gtk_tree_model_get_value(model, &iter, ROLE_DESC_COL, &value);
	s = g_strdup(g_value_get_string(&value));
	tmp = s;
	while ((part = strstr(tmp, "&")) != NULL)
	{
	    ++count;
	    tmp = ++part;
	}
	if (count == 0)
	    new_str = s;
	else
	{
	    new_str = malloc(strlen(s)+(4*count)+1);
	    bzero(new_str, strlen(s)+(4*count)+1);
	    tmp = s;
	    while ((part = strstr(tmp, "&")) != NULL)
	    {
	        strncat(new_str, tmp, part-tmp);
	        strncat(new_str, "&amp;", strlen("&amp;"));
	        tmp = ++part;
	    }
	    strncat(new_str, tmp, part-tmp);
	}
	snprintf(str, 256, _("<i><b>Description:</b></i> %s"), new_str);
	if (count != 0)
	    free(new_str);
	g_free(s);
	g_object_set(desc_label, 
		"label", 		str, 
		"wrap",			TRUE,
		NULL);
    }
}

gboolean props_add_selection_callback(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
    GtkWidget *label = GTK_WIDGET(data);
    GtkTreeIter iter;
    GtkTreeModel *lm = model;
    GtkLabel *desc;
    char *s;
    GValue value = {0,};
    char str[256];
    char *tmp;
    char *new_str = NULL;
    char *part;
    int count = 0;
		
    gtk_widget_set_sensitive(GTK_WIDGET(auth_add), TRUE);
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
    	gtk_tree_model_get_value(model, &iter, ROLE_DESC_COL, &value);
	s = g_strdup(g_value_get_string(&value));
	tmp = s;
	while ((part = strstr(tmp, "&")) != NULL)
	{
	    ++count;
	    tmp = ++part;
	}
	if (count == 0)
	    new_str = s;
	else
	{
	    new_str = malloc(strlen(s)+(4*count)+1);
	    bzero(new_str, strlen(s)+(4*count)+1);
	    tmp = s;
	    while ((part = strstr(tmp, "&")) != NULL)
	    {
	        strncat(new_str, tmp, part-tmp);
	        strncat(new_str, "&amp;", strlen("&amp;"));
	        tmp = ++part;
	    }
	    strncat(new_str, tmp, part-tmp);
	}
	snprintf(str, 256, _("<i><b>Description:</b></i> %s"), new_str);
	if (count != 0)
	    free(new_str);
	g_free(s);
	g_object_set(desc_label, 
		"label", 		str, 
		"wrap",			TRUE,
		NULL);
    }
}

GtkWidget *
create_required_authorization_list()
{
	GtkWidget *ret = NULL;
	GtkListStore *list;
	GtkTreeView *view;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeIter iter;
	authattr_t *authp;
	
	list = gtk_list_store_new(NUM_ROLE_COLS,
			G_TYPE_STRING, G_TYPE_STRING);
	
	req_view = view = (GtkTreeView *)gtk_tree_view_new ();
	
	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Required"),
			cell,
			"text",		ROLE_COL,
			NULL);
	gtk_tree_view_append_column(view, col);
	gtk_tree_view_set_model (view, (GtkTreeModel *)list);
	
	ret = (GtkWidget *)view;
	return ( ret );
}

static void
remove_required_from_not_required(GtkListStore *not_required, char *authlist)
{
	char *tmp;
	char *tauth = g_strdup(authlist);
	GtkTreeIter iter;
	GValue value = {0,};
	char *s;
	GtkTreeModel *model;
	char *next_auth;
	
	model = GTK_TREE_MODEL(not_required);
	next_auth = strtok(tauth, ",");
	while (next_auth)
	{

		gtk_tree_model_get_iter_first(model, &iter);
		do {
			gtk_tree_model_get_value(model, &iter, ROLE_COL, &value);
			s = g_strdup(g_value_get_string(&value));
			g_value_unset(&value);
			if (strcmp(s, next_auth) == 0) 
				gtk_list_store_remove(not_required, &iter);
			g_free(s);
		} while (gtk_tree_model_iter_next(model, &iter));
		next_auth = strtok(NULL, ",");
	}
}

void update_required_list(GtkListStore *list, char *authlist)
{
	char *tmp;
	char *tauth = g_strdup(authlist);
	GtkTreeIter iter;
	authattr_t *authp;	
	char *next_auth;
	
	setauthattr();
	next_auth = strtok(tauth, ",");
	while (next_auth)
	{
		authp = getauthnam(next_auth);
		if (authp != NULL)
		{
		    gtk_list_store_append(list, &iter);
		    gtk_list_store_set(list, &iter,
				ROLE_COL,	authp->name, 
				ROLE_DESC_COL,	authp->short_desc,
				-1);
		    free_authattr(authp);
		}
		next_auth = strtok(NULL, ",");
	}
	endauthattr();
}

void fill_not_required_list(GtkListStore *list)
{
	GtkTreeIter iter;
	authattr_t *authp;
	
	setauthattr();
	while ((authp = getauthattr()) != NULL) {
		if ((authp->name[strlen(authp->name) - 1] == '.') ||
		    (strcmp(authp->name, "") == 0))
		{
			free_authattr(authp);
			continue;
		}
		
		gtk_list_store_append(list, &iter);
		gtk_list_store_set(list, &iter,
			ROLE_COL,	authp->name, 
			ROLE_DESC_COL,	authp->short_desc,
			-1);
		
		free_authattr(authp);
	}
	endauthattr();
}

GtkWidget *
create_not_required_authorization_list()
{
	GtkWidget *ret = NULL;
	GtkListStore *list;
	GtkTreeView *view;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeIter iter;
	authattr_t *authp;
		
	list = gtk_list_store_new(NUM_ROLE_COLS,
			G_TYPE_STRING, G_TYPE_STRING);

	fill_not_required_list(list);
	
	not_req_view = view = (GtkTreeView *)gtk_tree_view_new ();
	
	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Not Required"),
			cell,
			"text",		ROLE_COL,
			NULL);
	gtk_tree_view_append_column(view, col);
	gtk_tree_view_set_model (view, (GtkTreeModel *)list);
	
	ret = (GtkWidget *)view;
		
	return ( ret );
}

char *tpath_auths = NULL;
char *ntpath_auths = NULL;
static void update_authorizations(DevMgrPropsDialogDetails *, char *);

void 
tpath_callback(GtkWidget *w, gpointer data)
{
	DevMgrPropsDialogDetails *details = data;
	char *tmp;
	gboolean act;
	
	switch (gtk_option_menu_get_history(GTK_OPTION_MENU(details->by)))
	{
	    case AUTHORIZED_USERS:
	    	g_object_get(details->auth, "text", &tmp, NULL);
	    	break;
	    case NO_USERS:
	    	tmp = g_strdup("*");
	    	break;
	    case ALL_USERS:
	    	tmp = g_strdup("@");
	    	break;
	    case SAME_AS_TP:
	        tmp = g_strdup("all");
	    	break;
	    default: /* ERROR */
	  	break;
	}
	
	g_object_get(w, "active", &act, NULL);
	if (w == details->nontpath)
	{
	  if (act)
	  {
	    gtk_widget_set_sensitive(details->by_mitems[SAME_AS_TP], TRUE);	
	    if (ntpath_auths && strcmp(ntpath_auths, "*") == 0)
	      gtk_option_menu_set_history(details->by, NO_USERS);
	    else if (ntpath_auths && strcmp(ntpath_auths, "@") == 0)
	      gtk_option_menu_set_history(details->by, ALL_USERS);
	    else if (ntpath_auths && strcmp(ntpath_auths, "all") == 0)
	    {
	      gtk_option_menu_set_history(details->by, SAME_AS_TP);
	      gtk_list_store_clear(details->required);
	      gtk_list_store_clear(details->not_required);
	      fill_not_required_list(details->not_required);
	    }
	    else if (ntpath_auths) {
	      g_object_set(details->auth, "text", ntpath_auths, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
	      gtk_list_store_clear(details->required);
	      update_required_list(details->required, ntpath_auths);
	      fill_not_required_list(details->not_required);
	    } else {
	      g_object_set(details->auth, "text", "", NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
	      gtk_list_store_clear(details->required);
	      fill_not_required_list(details->not_required);
	      gtk_option_menu_set_history(details->by, SAME_AS_TP);
	    }
	  } else {
	  	if (ntpath_auths)
	  	  g_free(ntpath_auths);
	  	ntpath_auths = g_strdup(tmp);
	        gtk_list_store_clear(details->required);
	        fill_not_required_list(details->not_required);
	  }
	}
	else 
	{
	  if (act)
	  {
	    if (tpath_auths && strcmp(tpath_auths, "*") == 0)
	      gtk_option_menu_set_history(details->by, NO_USERS);
	    else if (tpath_auths && strcmp(tpath_auths, "@") == 0)
	      gtk_option_menu_set_history(details->by, ALL_USERS);
	    else if (tpath_auths) {
	      g_object_set(details->auth, "text", tpath_auths, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
	      gtk_list_store_clear(details->required);
	      update_required_list(details->required, tpath_auths);
	      fill_not_required_list(details->not_required);
	      remove_required_from_not_required(details->not_required, tpath_auths);	      
	      gtk_option_menu_set_history(details->by, AUTHORIZED_USERS);
	    }
	    gtk_widget_set_sensitive(details->by_mitems[SAME_AS_TP], FALSE);
	  } else {
	  	if (tpath_auths)
	  	  g_free(tpath_auths);
	  	tpath_auths = g_strdup(tmp);
	  }
	}
}

void
option_changed(GtkWidget *w, gpointer data)
{
	DevMgrPropsDialogDetails *details = data;
	int oindex;

	oindex = gtk_option_menu_get_history(GTK_OPTION_MENU(w));
	switch (oindex) {
		case AUTHORIZED_USERS:
		  gtk_widget_set_sensitive(GTK_WIDGET(details->expander), TRUE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->auth), TRUE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), TRUE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->desc), TRUE);
		  break;
		case NO_USERS:
		  gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
		  /* Clear the auth list */
		  gtk_list_store_clear(details->required);
		  fill_not_required_list(details->not_required);
		  g_object_set(details->auth, "text", "", NULL);
		  break;
		case ALL_USERS:
		  gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
		  /* Clear the auth list */
		  gtk_list_store_clear(details->required);
		  fill_not_required_list(details->not_required);
		  g_object_set(details->auth, "text", "", NULL);
		  break;
		case SAME_AS_TP:
		  gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
		  gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
		  /* Clear the auth list */
		  gtk_list_store_clear(details->required);
		  fill_not_required_list(details->not_required);
		  g_object_set(details->auth, "text", "", NULL);
		  break;
		default: /* Really an Error */
			break;
	}
}

GtkTable *
create_authorization_table(GtkListStore **dreq, GtkListStore **dnotreq, GtkWidget **desc, DevMgrPropsDialogDetails *details)
{
	GtkVBox *avbox;
	GtkTable *auth_table;
	GtkHBox *hbox;
	GtkWidget *button;
	GtkWidget *not_req, *required;
	GtkWidget *scroller;
	GtkRequisition req;
	GtkTreeSelection *selection;
	
	auth_table = g_object_new(GTK_TYPE_TABLE,
			"n-rows",	2,
			"n-columns",	3,
			NULL);
			
	/* Not Required Authorization list */
	not_req = create_not_required_authorization_list();
	g_object_get(not_req, "model", dnotreq, NULL);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(not_req));
	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW, NULL);
	gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(not_req));
	gtk_widget_size_request (GTK_WIDGET(not_req), &req);
	gtk_widget_set_size_request(GTK_WIDGET(scroller), req.width + 20, 200);
	gtk_table_attach(auth_table, GTK_WIDGET(scroller), 0, 1, 0, 1,
			GTK_FILL, GTK_SHRINK, 0, 0);
	*desc = desc_label = g_object_new(GTK_TYPE_LABEL, 
			"use-markup",	TRUE,
			"wrap",		TRUE,
			"label",	_("<i><b>Description:</b></i>"),	
			NULL);
	gtk_misc_set_alignment (GTK_MISC (desc_label), 0.0, 0.0);	
	gtk_table_attach(auth_table, GTK_WIDGET(desc_label), 0, 1, 1, 2,
			GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)props_add_selection_callback, GTK_WIDGET(desc_label), NULL);
	
	/* Add and Remove buttons */
	avbox = (GtkVBox *)gtk_vbox_new(FALSE, 0);
	details->auth_add = auth_add = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("_Add >>"),
			"use-underline",	TRUE,
			"sensitive",		FALSE, /* No device selected yet */
			NULL);

	details->auth_remove = auth_remove = g_object_new(GTK_TYPE_BUTTON,
			"label",	_("<< _Remove"),
			"use-underline",	TRUE,
			"sensitive",		FALSE, /* No device selected yet */
			NULL);
	gtk_box_pack_start(GTK_BOX(avbox), GTK_WIDGET(auth_add), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(avbox), GTK_WIDGET(auth_remove), FALSE, FALSE, 10);

	gtk_table_attach(auth_table, GTK_WIDGET(avbox), 1, 2, 0, 1,
			GTK_SHRINK, GTK_SHRINK, 5, 0);
			
	/* Required Authorization list */
	required = create_required_authorization_list();
	g_object_get(required, "model", dreq, NULL);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(required));
	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW, NULL);
	gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(required));
	gtk_widget_set_size_request(GTK_WIDGET(scroller), req.width + 20, 200);
	gtk_table_attach(auth_table, GTK_WIDGET(scroller), 2, 3, 0, 1,
			GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)props_remove_selection_callback, GTK_WIDGET(desc_label), NULL);
	g_signal_connect(auth_add, "clicked", G_CALLBACK(auth_add_callback), details);	
	g_signal_connect(auth_remove, "clicked", G_CALLBACK(auth_remove_callback), details);
	/* Additional buttons */
	hbox = (GtkHBox *)gtk_hbox_new(FALSE, 0);
	button = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("_Select All"),
			"use-underline",	TRUE,
			NULL);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(button), TRUE, TRUE, 3);
	button = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("_Clear All"),
			"use-underline",	TRUE,
			NULL);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(button), TRUE, TRUE, 3);
	gtk_table_attach(auth_table, GTK_WIDGET(hbox), 2, 3, 1, 2,
		GTK_SHRINK, GTK_SHRINK, 0, 5);
	
	return ( auth_table );
}

void props_expand_callback(GtkWidget *widget, gpointer data)
{
	GtkTable *table = auth_table;
	GtkDialog *dialog = GTK_DIALOG(data);
	gboolean shown;
	gint height, width;
	GtkRequisition req;

	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height); 
	gtk_widget_size_request (GTK_WIDGET(table), &req);
		
	g_object_get(G_OBJECT(table), "visible", &shown, NULL);
	if (shown)
	{
		/* expander_table height + 10 for padding */
		height -= req.height + YSPACING;
		gtk_widget_hide(GTK_WIDGET(table));
	}
	else
	{
		/* Same as above. */
		height += req.height + YSPACING;
		gtk_widget_show(GTK_WIDGET(table));
	}
	gtk_window_resize(GTK_WINDOW(dialog), width, height);
}

static void
devmgr_props_dialog_instance_init (DevMgrPropsDialog *devmgr_props_dialog) 
{
	GdkPixbuf *pixbuf;
	GtkWidget *hbox, *label, *image, *vbox1, *vbox2;
	GtkWidget *scroller, *text_view, *combo;
	DevMgrPropsDialogDetails *details;
	GtkWidget *reset_button;
	char message[256];
	GtkDialog *dialog = GTK_DIALOG (devmgr_props_dialog);
	GtkVBox *dvbox;
	GtkTable *props_details_table;
	GtkEntry *min_entry, *max_entry, *auth_entry;
	GtkTextBuffer *device_buffer;
	GtkTextView *device_text;
	GtkTextTagTable *device_tags;
	GtkWidget *vbox, *bbox;
	GtkWidget *boot, *logout;
	GtkTable *alloc_table;
	GtkExpander *expander;
	GtkWidget *tpath, *nontpath;
	GtkButton *button;
	GtkMenu *menu;
	GtkOptionMenu *omenu;
	GdkScreen *screen;
	int depth;
	GdkPixmap *pix;
	GtkImage *img;
	GdkPixbuf *tmp;
	int scaled_w, image_height;
	GdkPixbuf *ws_scaled;
	int i;

	devmgr_props_dialog->details = g_new0(DevMgrPropsDialogDetails, 1);
	details = devmgr_props_dialog->details;
	
	if (window_type == PROPS_WINDOW)
		sprintf(message, _("Device Properties: %s"), "cdrom");
	else
		sprintf(message, _("Add Device"));
	gtk_window_set_title (GTK_WINDOW (dialog), message);
	
	dvbox = GTK_VBOX(dialog->vbox);
	
	gtk_dialog_add_buttons (dialog, 
			GTK_STOCK_HELP, 	GTK_RESPONSE_HELP,
			GTK_STOCK_CANCEL, 	GTK_RESPONSE_CANCEL,
			NULL);

	reset_button = g_object_new(GTK_TYPE_BUTTON, NULL);
	label = g_object_new(GTK_TYPE_LABEL,
				"label",		_("_Reset"),
				"use-underline",	TRUE,
				NULL);

	image = gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_BUTTON);

	bbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(bbox), 2);
	gtk_box_pack_start(GTK_BOX(bbox), image, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(bbox), label, FALSE, FALSE, 3);
	gtk_container_add(GTK_CONTAINER(reset_button), bbox);
	g_signal_connect(G_OBJECT(reset_button), "expose_event", G_CALLBACK(reset_expose), NULL);
	gtk_widget_show(reset_button);
	gtk_dialog_add_action_widget(dialog, reset_button, DEVMGR_PROPS_RESET);
	gtk_dialog_add_buttons(dialog,
			GTK_STOCK_OK,		GTK_RESPONSE_OK,
			NULL);
	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);

	g_signal_connect(dialog, "delete-event", G_CALLBACK(props_delete_event), NULL);
	
	/* Details */
	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new (NULL);
	sprintf(message, _("<b>Details</b>"));
	gtk_label_set_markup (GTK_LABEL (label), message);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), TRUE, TRUE, MAJOR_LEFT_JUSTIFY);
	gtk_box_pack_start(GTK_BOX(dvbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);
	
	props_details_table = g_object_new(GTK_TYPE_TABLE,
			"n-rows",	6,
			"n-columns",	3,
			NULL);

	label = g_object_new(GTK_TYPE_LABEL,
			"label",	_("Name:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach(props_details_table, GTK_WIDGET(label), 0, 1, 1, 2,
		GTK_FILL, GTK_FILL, MINOR_LEFT_JUSTIFY, 0);
	
	if (window_type == PROPS_WINDOW) 
	{
	  details->name = label = g_object_new(GTK_TYPE_LABEL, NULL);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	  gtk_table_attach(props_details_table, GTK_WIDGET(label), 1, 3, 1, 2,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	}
	else
	{
	  details->name = label = g_object_new(GTK_TYPE_ENTRY, NULL);  
	  gtk_table_attach(props_details_table, GTK_WIDGET(label), 1, 2, 1, 2,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, YSPACING);
	}
	
	label = g_object_new(GTK_TYPE_LABEL,
			"label",	_("Type:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach(props_details_table, GTK_WIDGET(label), 0, 1, 2, 3,
		GTK_FILL, GTK_FILL, MINOR_LEFT_JUSTIFY, 0);
	
	if (window_type == PROPS_WINDOW)
	{
	  details->dtype = label = g_object_new(GTK_TYPE_LABEL, NULL);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	  gtk_table_attach(props_details_table, GTK_WIDGET(label), 1, 3, 2, 3,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	}
	else
	{
	  details->dtype = label = g_object_new(GTK_TYPE_ENTRY, NULL);
	  gtk_table_attach(props_details_table, GTK_WIDGET(label), 1, 2, 2, 3,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, YSPACING);
	}
	
	button = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("Mi_n Label..."),
			"use-underline",	TRUE,
			NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(button), 0, 1, 3, 4,
		GTK_FILL, GTK_FILL | GTK_SHRINK, MINOR_LEFT_JUSTIFY, YSPACING);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(min_cb), dialog);
	
	details->min = min_entry = g_object_new(GTK_TYPE_ENTRY, 
			"editable",	FALSE,
			NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(min_entry), 1, 2, 3, 4,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	g_object_set(details->min, "text", "admin_low", NULL);

	screen = gtk_window_get_screen(GTK_WINDOW(dialog));
	depth = gdk_drawable_get_depth(gdk_screen_get_root_window(screen));
	pix = gdk_pixmap_new(gdk_screen_get_root_window(screen), 16, 16, depth);
	details->min_img = img = (GtkImage *)gtk_image_new_from_pixmap(pix, NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(img), 2, 3, 3, 4,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	update_image_widget(GTK_DIALOG(dialog), details->min_img, "admin_low");

	button = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("Ma_x Label..."),
			"use-underline",	TRUE,
			NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(button), 0, 1, 4, 5,
		GTK_FILL, GTK_FILL | GTK_SHRINK, MINOR_LEFT_JUSTIFY, YSPACING);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(max_cb), dialog);
	
	details->max = max_entry = g_object_new(GTK_TYPE_ENTRY, 
			"editable",	FALSE,
			NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(max_entry), 1, 2, 4, 5,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	g_object_set(details->max, "text", "admin_high", NULL);
		
	pix = gdk_pixmap_new(gdk_screen_get_root_window(screen), 16, 16, depth);
	details->max_img = img = (GtkImage *)gtk_image_new_from_pixmap(pix, NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(img), 2, 3, 4, 5,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	update_image_widget(GTK_DIALOG(dialog), details->max_img, "admin_high");

	label = g_object_new(GTK_TYPE_LABEL,
			"label",	_("Clean Program:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach(props_details_table, GTK_WIDGET(label), 0, 1, 5, 6,
		GTK_FILL, GTK_FILL | GTK_SHRINK, MINOR_LEFT_JUSTIFY, 0);
	details->clean = clean_entry = g_object_new(GTK_TYPE_ENTRY, NULL);
        g_object_set(details->clean, "text", "/bin/true", NULL);
	gtk_table_attach(props_details_table, GTK_WIDGET(clean_entry), 1, 2, 5, 6,
		GTK_EXPAND | GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
	button = g_object_new(GTK_TYPE_BUTTON, NULL);
	bbox = gtk_hbox_new(FALSE, 0);
	image = gtk_image_new_from_stock(GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(bbox), image, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button), bbox);
	g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(find_clean_program), (gpointer)clean_entry);
	gtk_table_attach(props_details_table, GTK_WIDGET(button), 2, 3, 5, 6,
		GTK_FILL, GTK_FILL, MINOR_LEFT_JUSTIFY, 0);
		
	label = g_object_new(GTK_TYPE_LABEL,
			"label",	_("Device Map:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach(props_details_table, GTK_WIDGET(label), 0, 1, 6, 7,
		GTK_FILL, GTK_FILL | GTK_SHRINK, MINOR_LEFT_JUSTIFY, 10);
	
	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
			"hscrollbar_policy",	GTK_POLICY_NEVER,
			"vscrollbar_policy",	GTK_POLICY_NEVER,
			"shadow_type",		GTK_SHADOW_IN,
			NULL);
	gtk_widget_set_usize(GTK_WIDGET(scroller), 100, 100);
	device_tags = gtk_text_tag_table_new();
	details->map = device_buffer = gtk_text_buffer_new(device_tags);
	device_text = GTK_TEXT_VIEW(gtk_text_view_new_with_buffer(device_buffer));
	g_object_set(device_text, "wrap-mode", GTK_WRAP_WORD, NULL);

	gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(device_text));

	gtk_table_attach(props_details_table, GTK_WIDGET(scroller), 1, 2, 6, 7,
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 10);

	gtk_box_pack_start (GTK_BOX (dvbox), GTK_WIDGET(props_details_table), TRUE, TRUE, 0);
	
	/* Allocation Properties */
	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new (NULL);
	sprintf(message, _("<b>Allocation</b>"));
	gtk_label_set_markup (GTK_LABEL (label), message);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), TRUE, TRUE, MAJOR_LEFT_JUSTIFY);
	gtk_box_pack_start (GTK_BOX (dvbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	alloc_table = g_object_new(GTK_TYPE_TABLE,
			"n-rows",	3,
			"n-columns",	2,
			NULL);
	
	label = g_object_new(GTK_TYPE_LABEL,
			"label",		_("Allocation from:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach(alloc_table, GTK_WIDGET(label), 0, 1, 0, 1,
		GTK_FILL, GTK_FILL | GTK_SHRINK, MINOR_LEFT_JUSTIFY, YSPACING);
	
	details->tpath = tpath = g_object_new(GTK_TYPE_RADIO_BUTTON,
			"use-underline",	TRUE,
			"label",		_("_Trusted Path"),
			NULL);
	details->nontpath = nontpath = g_object_new(GTK_TYPE_RADIO_BUTTON,
			"use-underline",	TRUE,
			"label",		_("Non-T_rusted Path"),
			"group",		tpath,
			NULL);
	g_signal_connect(G_OBJECT(tpath), "clicked", G_CALLBACK(tpath_callback), details);
	g_signal_connect(G_OBJECT(nontpath), "clicked", G_CALLBACK(tpath_callback), details);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tpath), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(nontpath), TRUE, TRUE, 0);
	gtk_table_attach(alloc_table, GTK_WIDGET(hbox), 1, 2, 0, 1, 
		GTK_FILL, GTK_FILL | GTK_SHRINK, 0, 0);
		
	label = g_object_new(GTK_TYPE_LABEL,
			"use-underline",	TRUE,
			"label",		_("Allocatable b_y:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach(alloc_table, GTK_WIDGET(label), 0, 1, 1, 2,
		GTK_FILL, GTK_FILL, MINOR_LEFT_JUSTIFY, YSPACING);

#ifdef USE_COMBOBOX
	details->by = combobox = (GtkComboBox *)gtk_combo_box_new_text ();
	gtk_combo_box_append_text (combobox, _("Authorized Users"));
	gtk_combo_box_append_text (combobox, _("No Users"));
	gtk_combo_box_append_text (combobox, _("All Users"));
	gtk_combo_box_append_text (combobox, _("Same As Trusted Path"));
	gtk_combo_box_set_active(combobox, 0);
	gtk_table_attach(alloc_table, GTK_WIDGET(combobox), 1, 2, 1, 2,
		GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
	g_object_set(label, "mnemonic-widget", combobox, NULL);
#else
	menu = g_object_new(GTK_TYPE_MENU, NULL);
	details->by_mitems[0] = gtk_menu_item_new_with_label(_("Authorized Users"));
	details->by_mitems[1] = gtk_menu_item_new_with_label(_("No Users"));
	details->by_mitems[2] = gtk_menu_item_new_with_label(_("All Users"));
	details->by_mitems[3] = gtk_menu_item_new_with_label(_("Same As Trusted Path"));
	for (i = 0; i < 4; i++)
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), details->by_mitems[i]);
	details->by = omenu = g_object_new(GTK_TYPE_OPTION_MENU, "menu", menu, NULL);
        gtk_option_menu_set_history(details->by, 1);
	gtk_widget_set_sensitive(details->by_mitems[SAME_AS_TP], FALSE);
	g_signal_connect(G_OBJECT(omenu), "changed", G_CALLBACK(option_changed), details);
	gtk_table_attach(alloc_table, GTK_WIDGET(omenu), 1, 2, 1, 2,
		GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
#endif

	details->expander = expander = g_object_new(GTK_TYPE_EXPANDER,
			"label",		_("Authori_zations:"),
			"use-underline",	TRUE,
			"expanded",		TRUE,
			NULL);

	gtk_table_attach(alloc_table, GTK_WIDGET(expander), 0, 1, 2, 3,
		GTK_FILL, GTK_FILL | GTK_SHRINK, MINOR_LEFT_JUSTIFY, YSPACING);
	details->auth = auth_entry = g_object_new(GTK_TYPE_ENTRY, NULL);
	gtk_table_attach(alloc_table, GTK_WIDGET(auth_entry), 1, 2, 2, 3,
		GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_box_pack_start (GTK_BOX (dvbox), GTK_WIDGET(alloc_table), TRUE, TRUE, 0);		

	details->exp_hbox = hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX (dvbox), GTK_WIDGET(hbox), TRUE, TRUE, MAJOR_LEFT_JUSTIFY);
	
	auth_table = create_authorization_table(&details->required, &details->not_required, &details->desc, details);

	gtk_box_pack_start (GTK_BOX(hbox), GTK_WIDGET(auth_table), TRUE, TRUE, MINOR_LEFT_JUSTIFY);

	g_signal_connect(G_OBJECT(expander), "activate", G_CALLBACK(props_expand_callback), devmgr_props_dialog);
	
/* I should really attach the authorization table to to the expander,
 * however, I end up with a wierd layout change.  Once I figure out
 * why then I will attach it to the expander.  Until then I am commenting
 * out the container add function below.
 */
/*
	gtk_container_add(GTK_CONTAINER(expander), hbox);
*/

#ifdef ENABLE_DEALLOCATION
	/* Deallocation properties */
	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new (NULL);
	sprintf(message, _("<b>Deallocation</b>"));
	gtk_label_set_markup (GTK_LABEL (label), message);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), TRUE, TRUE, MAJOR_LEFT_JUSTIFY);
	gtk_box_pack_start (GTK_BOX (dvbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	details->boot = boot = g_object_new(GTK_TYPE_CHECK_BUTTON,
			"use-underline",	TRUE,
			"label",		_("Deallocate on _boot"),
			NULL);
	details->logout = logout = g_object_new(GTK_TYPE_CHECK_BUTTON,
			"use-underline",	TRUE,
			"label",		_("Deallocate on _logout"),
			NULL);
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(boot), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(logout), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox), GTK_WIDGET(vbox), TRUE, TRUE, MINOR_LEFT_JUSTIFY);
	gtk_box_pack_start (GTK_BOX(dvbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);
#endif /* ENABLE_DEALLOCATION */

        gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
}

static void
devmgr_props_dialog_class_init (DevMgrPropsDialogClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	
	gobject_class->finalize = devmgr_props_dialog_finalize;
}

char *
dev_mgr_get_authorizations(DevMgrPropsDialogDetails *details)
{
	char *tmp;
	int oindex;
	
	oindex = gtk_option_menu_get_history(details->by);
	switch (oindex) {
		case AUTHORIZED_USERS:
			g_object_get(details->auth, "text", &tmp, NULL);
			break;
		case NO_USERS:
			tmp = g_strdup("*");
			break;
		case ALL_USERS:
			tmp = g_strdup("@");
			break;
		case SAME_AS_TP:
			tmp = g_strdup("all");
			break;
		default:
			break;
	}
	
	return ( tmp );
}

void
dev_mgr_props_add_reset(GtkWidget *dialog)
{
	DevMgrPropsDialogDetails *details = DEVMGR_PROPS_DIALOG (dialog)->details;
	GtkMenu *menu;
	
	g_object_set(details->name, "text", "", NULL);
	g_object_set(details->dtype, "text", "", NULL);
	g_object_set(details->min, "text", "admin_low", NULL);
	update_image_widget(GTK_DIALOG(dialog), details->min_img, "admin_low");
	g_object_set(details->max, "text", "admin_high", NULL);
	update_image_widget(GTK_DIALOG(dialog), details->max_img, "admin_high");
	g_object_set(details->clean, "text", "/bin/true", NULL);
	gtk_text_buffer_set_text(details->map, "", 0);
	g_object_set(details->tpath, "active", TRUE, NULL);
	gtk_option_menu_set_history(details->by, 1);
	g_object_set(details->auth, "text", "", NULL);

	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
	gtk_list_store_clear(details->required);
	gtk_list_store_clear(details->not_required);
	fill_not_required_list(details->not_required);
	g_object_set(details->desc, "label", _("<i><b>Description:</b></i>"), NULL);
#ifdef ENABLE_DEALLOCATION
	g_object_set(details->boot, "active", FALSE, NULL);
	g_object_set(details->logout, "active", FALSE, NULL);
#endif /* ENABLE_DEALLOCATION */
        gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
}

void 
dev_mgr_props_get_values(GtkWidget *dialog, char **ndevname, char **ndevtype, char **ndevfile, char **ndevmin, char **ndevmax, char **ndevauth, char **ndeveall, char **ndevowner, char **ndevlabel, char **ndevclean)
{
	DevMgrPropsDialogDetails *details = DEVMGR_PROPS_DIALOG (dialog)->details;
	gint by;
	GtkTextIter start, end;
	char *new;
	int index;
	char tmp[5120];
	
	/* Work on getting the HEX name for the min/max text entries */
	if (minhex == NULL)
		g_object_get(details->min, "text", ndevmin, NULL);
	else
		*ndevmin = strdup(minhex);
	if (maxhex == NULL)
		g_object_get(details->max, "text", ndevmax, NULL);
	else
		*ndevmax = strdup(maxhex);
	gtk_text_buffer_get_bounds(details->map, &start, &end);
	new = gtk_text_buffer_get_text(details->map, &start, &end, FALSE);
	*ndevfile = g_strdup(new);
	g_free(new);
	g_object_get(details->auth, "text", ndevauth, NULL);
	g_object_get(details->clean, "text", ndevclean, NULL);
	if (window_type == PROPS_WINDOW)
	{
		g_object_get(details->dtype, "label", ndevtype, NULL);
		g_object_get(details->name, "label", ndevname, NULL);
	} else {
		g_object_get(details->dtype, "text", ndevtype, NULL);
		g_object_get(details->name, "text", ndevname, NULL);
	}

	*ndevauth = dev_mgr_get_authorizations(details);

#ifdef ENABLE_DEALLOCATION
	g_object_set(details->boot, "active", FALSE, NULL);
	g_object_set(details->logout, "active", FALSE, NULL);
#endif /* ENABLE_DEALLOCATION */
}

/* 
 * Authorizations are:
 *
 *	trusted-path:non-trusted-path
 *
 *	    path can be:
 *
 *		* ----> No users
 *		@ ----> All users
 *		all --> All authorizations required
 *	        list -> List of authorizations
 */
static void update_authorizations(DevMgrPropsDialogDetails *details, char *devauth)
{
	char *tdevauth = g_strdup(devauth);
	char *tpauth;
	char *ntpauth;
	char *next_auth;
	
	tpauth = strtok(tdevauth, ":");
	ntpauth = strtok(NULL, ":");
	
	/* no users */
	if (strcmp(tpauth, "*") == 0) {
		gtk_option_menu_set_history(details->by, 1);
		gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
	/* all users */
	} else if (strcmp(tpauth, "@") == 0) {
		gtk_option_menu_set_history(details->by, 2);
		gtk_widget_set_sensitive(GTK_WIDGET(details->expander), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->auth), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->exp_hbox), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->desc), FALSE);
	/* all authorizations */
	} else if (strcmp(tpauth, "all") == 0) {
		gtk_option_menu_set_history(details->by, 3);
	} else {
		gtk_option_menu_set_history(details->by, 0);
		g_object_set(details->auth, "text", tpauth, NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);
		gtk_list_store_clear(details->required);
		update_required_list(details->required, tpauth);
		gtk_list_store_clear(details->not_required);
		fill_not_required_list(details->not_required);
		remove_required_from_not_required(details->not_required, tpauth);
	}
	
	/* non-trusted path is either all users, no users, or an 
	   authorization list */
	if (ntpauth)
	{
	  if (strcmp(ntpauth, "*") == 0) {
	
	  } else if (strcmp(ntpauth, "@") == 0) {
	
	  } else {
	
	  }
	}
}

void
dev_mgr_props_reset(GtkWidget *dialog, char *devfile, char *devmin, char *devmax, char *devauth, char *devdeall, char *devowner, char *devlabel, char *devclean)
{
	DevMgrPropsDialogDetails *details = DEVMGR_PROPS_DIALOG (dialog)->details;
	g_object_set(details->min, "text", devmin, NULL);
	update_image_widget(GTK_DIALOG(dialog), details->min_img, devmin);
	g_object_set(details->max, "text", devmax, NULL);
	update_image_widget(GTK_DIALOG(dialog), details->max_img, devmax);
	g_object_set(details->clean, "text", devclean, NULL);
	gtk_text_buffer_set_text(details->map, devfile, -1);
	g_object_set(details->tpath, "active", TRUE, NULL);
	update_authorizations(details, devauth);
	if (tpath_auths)
	  g_free(tpath_auths);
	tpath_auths = NULL;
	if (ntpath_auths)
	  g_free(ntpath_auths);
	ntpath_auths = NULL;
#ifdef DOH
#ifdef USE_COMBOBOX
	gtk_combo_box_set_active(details->by, 0);
#else
	/* not quite sure how to reset an option menu */
	g_object_get(details->by, "menu", &menu, NULL);
	gtk_menu_set_active(GTK_MENU(menu), 0);
#endif
	g_object_set(details->auth, "text", devauth, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(details->auth_remove), FALSE);

	gtk_list_store_clear(details->required);
	update_required_list(details->required, devauth);
	gtk_list_store_clear(details->not_required);
	fill_not_required_list(details->not_required);
	remove_required_from_not_required(details->not_required, devauth);
	g_object_set(details->desc, "label", _("<i><b>Description:</b></i>"), NULL);
#endif /* DOH */
#ifdef ENABLE_DEALLOCATION
	g_object_set(details->boot, "active", FALSE, NULL);
	g_object_set(details->logout, "active", FALSE, NULL);
#endif /* ENABLE_DEALLOCATION */
}

GtkWidget *
dev_mgr_props_add_dialog_new()
{
	GtkWidget *dialog;
	DevMgrPropsDialogDetails *details;

	window_type = ADD_WINDOW;
	dialog = g_object_new(DEVMGR_PROPS_TYPE_DIALOG, 
			"default-width",	DEFAULT_WIDTH,
			"default-height",	DEFAULT_HEIGHT,
			NULL);
	details = DEVMGR_PROPS_DIALOG (dialog)->details;
	
	return dialog;
}

GtkWidget *
dev_mgr_props_dialog_new (char *devname, char *devtype, char *devfile, char *devmin, char *devmax, char *devauth, char *devdeall, char *devowner, char *devlabel, char *devclean)
{
	GtkWidget *dialog;
	DevMgrPropsDialogDetails *details;
	char message[256];
	authattr_t *authp;
	GtkTreeIter iter;
	
	window_type = PROPS_WINDOW;
	dialog = g_object_new(DEVMGR_PROPS_TYPE_DIALOG, 
			"default-width",	DEFAULT_WIDTH,
			"default-height",	DEFAULT_HEIGHT,
			NULL);
	details = DEVMGR_PROPS_DIALOG (dialog)->details;
		
	snprintf(message, 256, _("Device Properties: %s"), devname);
	gtk_window_set_title(GTK_WINDOW(dialog), message);
	g_object_set(details->name, "label", devname, NULL);
	g_object_set(details->dtype, "label", devtype, NULL);
	g_object_set(details->min, "text", devmin, NULL);
	update_image_widget(GTK_DIALOG(dialog), details->min_img, devmin);
	g_object_set(details->max, "text", devmax, NULL);
	update_image_widget(GTK_DIALOG(dialog), details->max_img, devmax);	
	g_object_set(details->clean, "text", devclean, NULL);
	
	/* Deal with Text Buffer */
	gtk_text_buffer_set_text(details->map, devfile, -1);
	
	/* Deal with authorizations */
	update_authorizations(details, devauth);
	
	return dialog;
}

