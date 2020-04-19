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

#define HAVE_XTSOL

#include <config.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <zone.h>
#include <user_attr.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include <deflt.h>
#include <dev_alloc.h>
#include <auth_attr.h>
#include <secdb.h>

#include <libwnck/screen.h>
#include <libwnck/workspace.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/window.h>
#include <gdk/gdkx.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>

#include <pwd.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <priv.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "devmgr-dialog.h"
#include "devmgr-props-dialog.h"
#include "devmgr.h"

#define POLICY_CONF		"/etc/security/policy.conf"
#define DEFAULT_HEIGHT		400
#define DEFAULT_WIDTH		600
#define INTERVAL_TIMEOUT	5000	/* 5 seconds */
#define SYS_ACCRED_SET_AUTH     "solaris.label.range"
#define DEVICE_CONFIG_AUTH	"solaris.device.config"
#define DEVICE_ALLOCATE_AUTH	"solaris.device.allocate"
#define DEVICE_CDRW_AUTH	"solaris.device.cdrw"
#define DEVICE_GRANT_AUTH	"solaris.device.grant"
#define DEVICE_REVOKE_AUTH	"solaris.device.revoke"

/* Update with appropriate global variable for the popup */
struct _DevMgrDialogDetails {
	guint timerid;
	int timercount;

	GtkWidget *message;
	GtkWidget *source_label;
	GtkWidget *source_type;
	GtkWidget *source_owner;
	GtkWidget *dest_label;
	GtkWidget *dest_type;
	GtkWidget *dest_owner;
	GtkTextBuffer *buffer;
	GtkWidget *progress;
};

enum {
	AVAILABLE_LIST,
	ALLOCATED_LIST,
	NUM_LISTS
};

#define MAX_USERS       10      /* number of assumable roles in session */
#define PW_BUF_LEN      256     /* buffer area for password context */
#define MAX_LABEL_SIZE  2048    /* Max size of label string/hex */

typedef struct _WmUser {
        int             do_accred_check;
        char            pw_buffer[PW_BUF_LEN];
        struct passwd   p;
        /*
         * char    *pw_name;
         * char    *pw_passwd;
         * uid_t   pw_uid;
         * gid_t
         * pw_gid;
         * char    *pw_age;
         * char    *pw_comment;
         * char    *pw_gecos;
         * char    *pw_dir;
         * char    *pw_shell;
         */
        userattr_t      *u;
        /*
         * char *name;
         * char *lock;
         * char *gen;
         * char *profiles;
         * char *roles;
         * char *idletime;
         * char *idlecmd;
         * char *labelview;
         * char *labeltrans;
         * char *labelmin;
         * char *labelmax;
         * char *usertype;
         * char *res1;
         * char *res2;
         * char *res3;
         */
} WmUser;

static char    *logname;
static WmUser   User[MAX_USERS];
static int      UserCount = 0;
static int      user_index = 0;
static char    *role;
static int      current_user = 0;

#ifdef CONSOLE
extern FILE *console;
#endif /* CONSOLE */

/* these should be moved to the structure above. */
GtkListStore *list[NUM_LISTS];
GtkTreeView *view[NUM_LISTS];
GtkButton *device_add, *device_remove;
GtkWidget *admin_add, *admin_remove, *admin_reclaim, *admin_revoke, *admin_properties;
GtkWidget *admin_device, *admin_owner, *admin_label;
GtkLabel *user_label;
static GtkDialog *global_dialog;
static GtkDialog *global_props = NULL;
static gulong handler_id = 0;
static WnckWorkspace *gworkspace = NULL;
gboolean expanded_view;
GtkLabel *alloc_label;
GtkExpander *expander;
extern GtkTable *expander_table;

static void devmgr_dialog_class_init    (DevMgrDialogClass *);
static void devmgr_dialog_instance_init (DevMgrDialog *); 
extern const char *wnck_workspace_get_role(WnckWorkspace *);
char *getusrattrval(userattr_t *uattr, char *keywd);
extern const char *wnck_workspace_get_label(WnckWorkspace *);
extern void gnome_tsol_render_coloured_label_for_zone(GtkWidget *, char *);

static GtkDialogClass *parent_class = NULL;
/*
 * Structure that contains a list of devices. The list is dynamic, i.e. it
 * grows according to number of devices present on the system. count keeps
 * track of the number of devices. The structure could have used label field
 * for each device, but that would have made it more complicated. The device
 * labels could be easily obtained by calling getlabel(file) and stored
 * in the list maintained by motif, hence not nessary to maintain it twice.
 */

#ifdef DEV_T
typedef struct _DevItem_t {
        char      *devname;
	char      *devtype;
        bslabel_t label;
	char      *icon;
} DevItem_t;


typedef struct _DevInfo {
	Widget		w;
	int             count;
	DevItem_t      *pdevItem;
        XmStringTable   namestr;
} DevInfo;
#endif

enum {
  DEVNAME = 0,
  DEVTYPE,
  DEVFILE,
  DEVMINSL,
  DEVMAXSL,
  DEVAUTH,
  DEVCLEAN,
  DEVDEALL,
  DEVOWNER,
  DEVLABEL,
  DEVXDPY,
  KEYS
} ;

#define SEP2   ";:"
#define SEP3   "="
#define AUTO_DEALLOC_DEFAULT "none"
#define AUTO_DEALLOC_BOOT    "boot"
#define AUTO_DEALLOC_LOGOUT  "logout"
#define DATA_BUFSIZ    8096

typedef struct {
     char *key_value[KEYS];
} deviceData_t;

char *devKeyTab[KEYS] = { "device", "type", "files", "minlabel", "maxlabel", 
                          "auths", "clean", "auto_deallocate", "owner", 
                          "zone", "xdpy" };

char *zonename;
char *hilighted_device_zone = NULL;
char *time_stamp_file;
static time_t	last_mtime = 0;

enum
{
  AVAIL_COL_IMAGE = 0,
  AVAIL_COL_DEVNAME,
  AVAIL_COL_DEVTYPE,
  AVAIL_COL_DEVFILE,
  AVAIL_COL_DEVMINSL,
  AVAIL_COL_DEVMAXSL,
  AVAIL_COL_DEVAUTH,
  AVAIL_COL_DEVCLEAN,
  AVAIL_COL_DEVDEALL,
  AVAIL_COL_DEVOWNER,
  AVAIL_COL_DEVLABEL,
  AVAIL_NUM_COLS
} ;

enum
{
  ALLOC_COL_IMAGE = 0,
  ALLOC_COL_DEVNAME,
  ALLOC_COL_IMAGE_LABEL,
  ALLOC_COL_LABEL,
  ALLOC_COL_ZONE,
  ALLOC_COL_DEVTYPE,
  ALLOC_COL_DEVFILE,
  ALLOC_COL_DEVMINSL,
  ALLOC_COL_DEVMAXSL,
  ALLOC_COL_DEVAUTH,
  ALLOC_COL_DEVCLEAN,
  ALLOC_COL_DEVDEALL,
  ALLOC_COL_DEVOWNER,
  ALLOC_NUM_COLS
} ;

enum
{
  FLOPPY,
  CDROM,
  AUDIO,
  RMDISK,
  PRINTER,
  UNKNOWN,
  TYPE_OF_DEVICES
} ;

char *device_names[] = {
	"floppy",
	"cdrom",
	"audio",
	"rmdisk",
	"lp",
	"unknown"
};

char *fname[] = {
	"gnome-dev-floppy",
	"gnome-dev-cdrom",
	"audio-card",
	"gnome-dev-removable",
	"gnome-dev-printer",
	"gnome-dev-removable"
};

#define LIST_HEIGHT_IMAGE	16

GdkPixbuf *pdevices[TYPE_OF_DEVICES];
struct passwd pwd;
static char     displayNumber[8];

static time_t
get_latest_mtime(char *filename)
{
	time_t latest_time;
	struct stat statbuf;

	latest_time = 0;
        if (stat(filename, &statbuf) == 0) {
             latest_time = statbuf.st_mtime;
        }
     
	return (latest_time);
}

void load_device_pixbufs(void)
{
  int  count;
  static gint initialized = FALSE;
  
  if ( ! initialized )
  {
    for ( count = 0 ; count < TYPE_OF_DEVICES ; count++ )
    {
      pdevices[count] = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      fname[count], 16, 0, NULL);
    }
  }
  initialized = TRUE;
}

char *get_labelname(gchar *zonename)
{
  char          *label;
  m_label_t     *workspace_sl = NULL;
  m_label_t     *tmp_sl = NULL;
  zoneid_t       zid;
  int            error;

  workspace_sl = getzonelabelbyname(zonename);
  if (workspace_sl != NULL) 
    label_to_str(workspace_sl, &label, M_LABEL, LONG_NAMES);
  if (strcmp(label, "ADMIN_LOW") == 0 || strcmp(label, "global") == 0)
    label = strdup("ADMIN_HIGH");

  return ( strdup( label ) );
}

char *get_zonename(WnckScreen *screen)
{
  WnckWorkspace *workspace;
  gchar 	*label;
  m_label_t	*workspace_sl = NULL;
  m_label_t	*tmp_sl = NULL;
  zoneid_t	 zid;
  char 		 zonename[ZONENAME_MAX];
  int		 error;
  
  if ( screen == NULL )
    return ( strdup ( GLOBAL_ZONENAME ) );
  
  wnck_screen_force_update( screen );
  workspace = wnck_screen_get_active_workspace( screen );
  /* Get the Hex label for the workspace */
  label = (gchar *)wnck_workspace_get_label ( workspace );

  if ((label == NULL) || (strcmp(label, "ADMIN_LOW") == 0))
    return ( strdup ( GLOBAL_ZONENAME ) );
    
  str_to_label(label, &workspace_sl, MAC_LABEL, L_NO_CORRECTION, &error);

  if ((zid = getzoneidbylabel(workspace_sl)) > 0) {
    if (getzonenamebyid(zid, zonename, sizeof (zonename)) == -1) {
      strcpy(zonename, GLOBAL_ZONENAME);
    }
  } else {
     strcpy(zonename, GLOBAL_ZONENAME);
  }

  return ( strdup( zonename ) );
}

int run_command(char *cmd[])
{
	int 		 pid;
	priv_set_t	*pset;
	int		 w;
	int		 status;
	struct passwd	*p = getpwuid(User[current_user].p.pw_uid);
#ifdef CONSOLE
	int 		 i = 0;

	fprintf(console, "%s:%d:run_command\n", __FILE__, __LINE__);
	while (cmd[i] != NULL)
	{
		fprintf(console, "\tcmd[%d] = %s\n", i, cmd[i]);
		i++;
	}
	fprintf(console, "%s:%d:run_command\n", __FILE__, __LINE__);
#endif /* CONSOLE */

	pid = fork();
	switch ( pid )
	{
		case 0:	/* Child */
		    setgid(p->pw_gid);
		    setuid(p->pw_uid);
		    pset = priv_allocset();
		    getppriv(PRIV_PERMITTED, pset);
		    setppriv(PRIV_SET, PRIV_EFFECTIVE, pset);
		    chdir(p->pw_dir);
		    execv(cmd[0], cmd);
		    _exit(4); /* Should never reach here */
		    break;
		case -1:	/* Error */
		    fprintf(stderr, _("tsoljdsdevmgr:%s failed to start\n"), cmd[0]);
		    _exit(127);
		    break ; /* Not really needed as we'll never reach here */
		default:	/* Parent */
		    w = waitpid(pid, &status, 0);
		    return ( WEXITSTATUS(status) );
		    break;
	}
}

gboolean has_config_authorization()
{
	if (chkauthattr(DEVICE_CONFIG_AUTH, User[current_user].p.pw_name) == 1) 
		return ( TRUE );
	return ( FALSE );
}

gboolean has_allocate_authorization()
{
	if (chkauthattr(DEVICE_ALLOCATE_AUTH, User[current_user].p.pw_name) == 1) 
		return ( TRUE );
	return ( FALSE );
}

gboolean has_cdrw_authorization()
{
	if (chkauthattr(DEVICE_CDRW_AUTH, User[current_user].p.pw_name) == 1) 
		return ( TRUE );
	return ( FALSE );
}

gboolean has_grant_authorization()
{
	if (chkauthattr(DEVICE_GRANT_AUTH, User[current_user].p.pw_name) == 1) 
		return ( TRUE );
	return ( FALSE );
}

gboolean has_revoke_authorization()
{
	if (chkauthattr(DEVICE_REVOKE_AUTH, User[current_user].p.pw_name) == 1) 
		return ( TRUE );
	return ( FALSE );
}

void
reset_buttons(gboolean sensitivity)
{
  gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(admin_add), has_config_authorization() && sensitivity);
  gtk_widget_set_sensitive(GTK_WIDGET(admin_remove), has_config_authorization() && sensitivity);
  gtk_widget_set_sensitive(GTK_WIDGET(admin_reclaim), FALSE); /* unless device owner == /ERROR */
  gtk_widget_set_sensitive(GTK_WIDGET(admin_revoke), has_revoke_authorization() && sensitivity);
  gtk_widget_set_sensitive(GTK_WIDGET(admin_properties), has_config_authorization() && sensitivity);
}

int 
parseDeviceData(char *buf, deviceData_t *pdevData)
{
    char *next_token;
    char *value;
    char *str2;
    int  i;

#ifdef DEBUG
    perror("--- parseDeviceData() begins ---");
    fprintf(stderr, "buf = %s\n", buf);
#endif /* DEBUG */

    for (i = 0; i < KEYS; i++) {
       pdevData->key_value[i]=NULL;
    }

    /* remove trailing newline */
    i = strlen(buf);
    if ( i > 0 && buf[i-1] == '\n') {
           buf[i-1]= '\0';
    }

    next_token=strtok(buf, SEP2);
    while (next_token) {
         if ((value=strpbrk(next_token, SEP3))!= NULL) {
             value++; /* get rid of "=" */
         } else {
#ifdef DEBUG
             fprintf(stderr,"devmgr/parseDeviceData: Error in parsing token (%s)\n", next_token);
#endif /* DEBUG */
             return 1;
         }            
         for (i = 0; i < KEYS; i++) {
#ifdef DEBUG2
		fprintf(stderr, "next_token = %s\n", next_token?next_token:"NULL");
#endif /* DEBUG2 */
            if (strncmp(next_token, devKeyTab[i], strlen(devKeyTab[i])) == 0) {
               pdevData->key_value[i] = strdup(value);
#ifdef DEBUG2
               fprintf(stderr, "found: i = %d devKeyTab[i] = %s value = %s\n", i, devKeyTab[i], pdevData->key_value[i]);
#endif /* DEBUG2 */
               break;
            };
         };

	 if ( i >= KEYS) {
#ifdef DEBUG
            fprintf(stderr,"Could not find the key %s in the devKeyTab.\n", next_token);
#endif /* DEBUG */
         }
         next_token=strtok(NULL, SEP2);
    }   

    return 0;

}

static int
load_devices(GtkListStore *store, char *zonename, gint ttype)
{
  char  	 cmd[1024];
  char  	 buf[DATA_BUFSIZ];
  FILE 		*ptr;
  int   	 ret = 0;
  GtkTreeIter	 iter;
  deviceData_t 	 devData;
  GdkPixbuf 	*pix;
  int		 count;
  int		 i;
  char          *fullname = NULL;
  m_label_t 	*workspace_sl = NULL;

  workspace_sl = getzonelabelbyname(zonename);
  if (workspace_sl != NULL) 
    label_to_str(workspace_sl, &fullname, M_LABEL, LONG_NAMES);
  if (strcmp(fullname, "ADMIN_LOW") == 0 || strcmp(fullname, "global") == 0)
    fullname = strdup("ADMIN_HIGH");

  if (ttype == ALLOCATED_LIST)
  {
    if ( has_revoke_authorization() && expanded_view) 
      snprintf(cmd, sizeof(cmd), "/usr/sbin/list_devices -alw");
/*
      snprintf(cmd, sizeof(cmd), "/usr/sbin/list_devices -alw -z %s", zonename);
*/
    else
      snprintf(cmd, sizeof(cmd), "/usr/sbin/list_devices -auwU %d -z %s", User[current_user].p.pw_uid, zonename);
  }
  else /* AVAILABLE_LIST */
  {
    if ( has_revoke_authorization() && expanded_view)
      snprintf(cmd, sizeof(cmd), "/usr/sbin/list_devices -alw");
    else
      snprintf(cmd, sizeof(cmd), "/usr/sbin/list_devices -anwU %d -z %s", User[current_user].p.pw_uid, zonename);     
  }

  if ((ptr = popen(cmd, "r")) != NULL) 
  {
    while (fgets(buf, DATA_BUFSIZ, ptr) != NULL) 
    {
      if ( parseDeviceData(buf, &devData) != 0) 
      {
        perror ("Unable to parse data\n");
        return ( ret ) ;
      }
      else
      {
          char *devxdpy = devData.key_value[DEVXDPY];

	  if (devxdpy == NULL) {
		devxdpy = "0"; /*Console display */
	  }

	  /* Filter out devices based on X display */
	  if (strcmp (devxdpy, displayNumber) != 0) {
             /*
              * free devData space allocated
              * in parseDeviceData()
              */
                     for (i = 0; i < KEYS; i++) {
                         free(devData.key_value[i]);
                     }
             continue;
          }

          pix = NULL;
          for (count = FLOPPY; count < TYPE_OF_DEVICES ; count++)
            if (strncmp(device_names[count], devData.key_value[DEVNAME], strlen(device_names[count])) == 0)
              pix = pdevices[count];
              if (pix == NULL)
                  pix = pdevices[UNKNOWN];
          if (ttype == AVAILABLE_LIST)
          {
	    if ((devData.key_value[DEVOWNER] && 
                strcmp(devData.key_value[DEVOWNER], "/FREE") == 0) ||
		(devData.key_value[DEVAUTH] && 
		 strcmp(devData.key_value[DEVAUTH], "*") == 0 ))
                 {
                   gtk_list_store_append (store, &iter);
	           gtk_list_store_set (store, &iter,
  		      AVAIL_COL_IMAGE, pix,
                      AVAIL_COL_DEVNAME, devData.key_value[DEVNAME],
                      AVAIL_COL_DEVTYPE, devData.key_value[DEVTYPE],
                      AVAIL_COL_DEVFILE, devData.key_value[DEVFILE],
                      AVAIL_COL_DEVMINSL, devData.key_value[DEVMINSL],
                      AVAIL_COL_DEVMAXSL, devData.key_value[DEVMAXSL],
                      AVAIL_COL_DEVAUTH, devData.key_value[DEVAUTH],
                      AVAIL_COL_DEVCLEAN, devData.key_value[DEVCLEAN],
                      AVAIL_COL_DEVDEALL, devData.key_value[DEVDEALL],
                      AVAIL_COL_DEVOWNER, devData.key_value[DEVOWNER],
                      AVAIL_COL_DEVLABEL, devData.key_value[DEVLABEL],
                      -1);
		 }
          }
          else
          {
                 if (devData.key_value[DEVOWNER] && 
                     strcmp(devData.key_value[DEVOWNER], "/FREE") != 0 &&
       	      devData.key_value[DEVAUTH] && 
		      strcmp(devData.key_value[DEVAUTH], "*") != 0 )
                 {
                   fullname = get_labelname(devData.key_value[DEVLABEL]);
  
                   gtk_list_store_append (store, &iter);
          	   gtk_list_store_set (store, &iter,
          		ALLOC_COL_IMAGE, pix,
          	 	ALLOC_COL_DEVNAME, devData.key_value[DEVNAME],
          	 	ALLOC_COL_LABEL, fullname,
			ALLOC_COL_ZONE, devData.key_value[DEVLABEL],
                        ALLOC_COL_DEVTYPE, devData.key_value[DEVTYPE],
                        ALLOC_COL_DEVFILE, devData.key_value[DEVFILE],
                        ALLOC_COL_DEVMINSL, devData.key_value[DEVMINSL],
                        ALLOC_COL_DEVMAXSL, devData.key_value[DEVMAXSL],
                        ALLOC_COL_DEVAUTH, devData.key_value[DEVAUTH],
                        ALLOC_COL_DEVCLEAN, devData.key_value[DEVCLEAN],
                        ALLOC_COL_DEVDEALL, devData.key_value[DEVDEALL],
                        ALLOC_COL_DEVOWNER, devData.key_value[DEVOWNER],
                        -1);
                 }
}
/*
          g_object_unref ( G_OBJECT ( pix ) );
*/
      }
    }
    pclose(ptr);
  } 
  if (workspace_sl != NULL)
    blabel_free(workspace_sl);

  return ( ret );
}

GtkTreeModel *create_and_fill_model(GtkTreeView *view, char *zonename, gint ttype)
{
  GtkListStore	*store;
  GtkTreeIter    iter;
  int		 count;
  int		 data_count = 0;

  if (ttype == AVAILABLE_LIST)
    store = gtk_list_store_new (AVAIL_NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  else
    store = gtk_list_store_new (ALLOC_NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING,  GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  load_device_pixbufs();

  load_devices((GtkListStore *)store, zonename, ttype);
  
  return GTK_TREE_MODEL (store);
}

static void                                                             
devmgr_dialog_class_init_trampoline (gpointer klass, gpointer data)
{                                                                       
	parent_class = (GtkDialogClass *) g_type_class_ref (GTK_TYPE_DIALOG);
        devmgr_dialog_class_init ((DevMgrDialogClass *)klass);     
}                                                               

GType
devmgr_dialog_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
		    sizeof (DevMgrDialogClass),
	 	    NULL,               /* base_init */ 
		    NULL,               /* base_finalize */
		    devmgr_dialog_class_init_trampoline,
		    NULL,               /* class_finalize */
		    NULL,               /* class_data */
		    sizeof (DevMgrDialog),
		    0,                  /* n_preallocs */
		    (GInstanceInitFunc) devmgr_dialog_instance_init
		};
		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "DevMgrDialogType",
					       &info, 0);
        }
        return type;
}

static void
devmgr_dialog_finalize (GObject *object)
{
	DevMgrDialog *devmgr_dialog = DEVMGR_DIALOG (object);
	DevMgrDialogDetails *details = devmgr_dialog->details;

	/* free all my vars then the details structure */
	g_free (details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Get the Label color for the selected device's zonename */
static gboolean
label_expose (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	gboolean ret = FALSE;
	
	if (hilighted_device_zone != NULL)
	{
	  gnome_tsol_render_coloured_label_for_zone (widget, hilighted_device_zone);
	  
	  ret = TRUE;
	} 
	return ( ret );
}

gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gboolean ret = FALSE;
	gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
	
	return ( ret );
}

void quit_callback(GtkWidget *widget, gpointer data)
{
	gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
}

void expand_callback(GtkWidget *widget, gpointer data)
{
	GtkTable *table = expander_table;
	GtkDialog *dialog = GTK_DIALOG(data);
	gboolean shown;
	gint height, width;
	GtkRequisition req;
	GdkScreen *screen = NULL;
	WnckScreen *wnckscreen = NULL;
	char *zonename;
        GtkTreeView *lvv1 = view[AVAILABLE_LIST];
        GtkTreeView *lvv2 = view[ALLOCATED_LIST];
        GtkTreeModel *lva1 = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv1));
        GtkTreeModel *lva2 = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv2));
	char message[256];

        gtk_list_store_clear((GtkListStore *)lva1);
        gtk_list_store_clear((GtkListStore *)lva2);

	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height); 
	gtk_widget_size_request (GTK_WIDGET(table), &req);

	/* get zone */
	screen = gtk_widget_get_screen( GTK_WIDGET(dialog) );
	if (screen != NULL)
		wnckscreen = wnck_screen_get( gdk_screen_get_number (screen) );
	zonename = get_zonename(wnckscreen);

#ifdef CONSOLE 
		fprintf(console, "%s:%d:strcmp(\"%s\", \"global\") == %d\n",
			__FILE__, __LINE__, zonename?zonename:"NULL", 
			strcmp(zonename?zonename:"NULL", "global"));
#endif /* CONSOLE */
	sprintf(message, _("Device:"));
	g_object_set(admin_device, "label", message, NULL);
	sprintf(message, _("Owner:"));
	g_object_set(admin_owner, "label", message, NULL);
	sprintf(message, _("Label:"));
	g_object_set(admin_label, "label", message, NULL);
	reset_buttons(FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(admin_add), has_config_authorization());

	g_object_get(G_OBJECT(table), "visible", &shown, NULL);
	if (hilighted_device_zone != NULL)
	    g_free(hilighted_device_zone);
	hilighted_device_zone = NULL;

	if (shown == TRUE)
	{
		/* expander_table height + 10 for padding */
		height -= req.height + 10;
		gtk_widget_hide(GTK_WIDGET(table));
   
		expanded_view = FALSE;
	}
	else
	{
                GtkTreeView *av = view[AVAILABLE_LIST];
                GtkTreeModel *am = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(av));
                GtkTreeSelection *aselect = gtk_tree_view_get_selection(av);
                GtkTreeIter iter;

		/* Same as above. */
		height += req.height + 10;
		gtk_widget_show(GTK_WIDGET(table));
		expanded_view = TRUE;

                /* Check for available list selection */
                if (gtk_tree_selection_get_selected(aselect, NULL, &iter))
                {
		    GValue v1 = {0,};
                    /* insure remove and properties still active */
                    gtk_widget_set_sensitive(GTK_WIDGET(admin_properties),  has_config_authorization() && TRUE);
                    gtk_widget_set_sensitive(GTK_WIDGET(admin_remove), has_config_authorization()
&& TRUE);
		    /* insure Device name is still available */
            	    gtk_tree_model_get_value(am, &iter, AVAIL_COL_DEVNAME, &v1);
            	    sprintf(message, _("Device: %s"), g_value_get_string(&v1));
            	    g_value_unset(&v1);
            	    g_object_set(admin_device, "label", message, NULL);
                }
       	}	

        if (zonename)
	{
              load_devices((GtkListStore *)lva1, zonename, AVAILABLE_LIST);
              load_devices((GtkListStore *)lva2, zonename, ALLOCATED_LIST);
	}

	gtk_window_resize(GTK_WINDOW(dialog), width, height);
}

gboolean
is_widget_pinned(GtkWidget *widget)
{
	GdkWindow *gdkwindow;
	GtkWidget *parent;
	WnckWindow *wwindow;
	
	parent = gtk_widget_get_parent(GTK_WIDGET(widget));
	gdkwindow = gtk_widget_get_parent_window( parent );
	wwindow = wnck_window_get( GDK_WINDOW_XID (gdkwindow ) );

	return ( wnck_window_is_pinned( wwindow ) );
}

void
pin_the_widget(GtkWidget *widget)
{
	GdkWindow *gdkwindow;
	GtkWidget *parent;
	WnckWindow *wwindow;
	
	parent = gtk_widget_get_parent( GTK_WIDGET( widget ) );
	gdkwindow = gtk_widget_get_parent_window( parent );
	wwindow = wnck_window_get( GDK_WINDOW_XID ( gdkwindow ) );

	wnck_window_pin( wwindow );
}

int
get_current_user(WnckWorkspace *workspace)
{
    int i;
    int local_user = -1;
    char *role = (char *)wnck_workspace_get_role(workspace);

    for (i = 0; i < UserCount; i++)
    if (role && strcmp(role, User[i].p.pw_name) == 0)
    {
      local_user = i;
      break ; 
    }
    if (local_user == -1)
      local_user = 0;

    return ( local_user );
}

void
label_changed_callback(WnckWorkspace *workspace, gpointer data)
{
  char *zonename;
  gint value = GPOINTER_TO_INT(data);
  GtkTreeView *lv = view[value];
  GtkTreeModel *model = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(lv));
  char message[256];
  GtkTreeSelection *selection = gtk_tree_view_get_selection(lv);
  GtkTreeIter iter;
  GdkScreen *screen = NULL;
  WnckScreen *wnckscreen = NULL;
  WnckWorkspace *current_workspace;

  screen = gtk_widget_get_screen( GTK_WIDGET(device_add) );
  if (screen != NULL)
    wnckscreen = wnck_screen_get( gdk_screen_get_number (screen) );
  current_workspace = wnck_screen_get_active_workspace( wnckscreen );
  if (current_workspace != workspace)
    return ;
  zonename = get_zonename(wnckscreen);

  if (gtk_tree_selection_get_selected(selection, NULL, &iter))
  {
    reset_buttons(FALSE);
  
    sprintf(message, _("Device:"));
    g_object_set(admin_device, "label", message, NULL);
    sprintf(message, _("Owner:"));
    g_object_set(admin_owner, "label", message, NULL);
    sprintf(message, _("Label:"));
    g_object_set(admin_label, "label", message, NULL);
    if (hilighted_device_zone != NULL)
    {
      g_free(hilighted_device_zone);
      hilighted_device_zone = NULL;
    }
    if (value == AVAILABLE_LIST)
        gtk_widget_set_sensitive(GTK_WIDGET(device_add), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
  }
  
  gtk_list_store_clear((GtkListStore *)model);

  model = create_and_fill_model(GTK_TREE_VIEW(lv), zonename, value);
  gtk_tree_view_set_model (GTK_TREE_VIEW (lv), model);
}

void
workspace_changed_callback(WnckScreen *screen, WnckWorkspace *prev, 
			   gpointer data)
{
  char *zonename = get_zonename( screen );
  gint value = GPOINTER_TO_INT(data);
  GtkTreeView *lv = view[value];
  GtkTreeModel *model = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(lv));
  char message[256];
  GtkTreeSelection *selection = gtk_tree_view_get_selection(lv);
  GtkTreeIter iter;
  WnckWorkspace *workspace = wnck_screen_get_active_workspace( screen );
  int i;
  gboolean props_shown;

  if (global_props != NULL) 
    gtk_dialog_response(global_props, GTK_RESPONSE_CANCEL);

  current_user = get_current_user( workspace );
  sprintf(message, _("Devices Allocatable by: <b>%s</b>"),
                User[current_user].p.pw_name);

  if (! ( has_config_authorization() || has_revoke_authorization() ) )
	g_object_set(expander, "sensitive", FALSE, NULL);
  else
	g_object_set(expander, "sensitive", TRUE, NULL);

  gtk_widget_set_sensitive(GTK_WIDGET(admin_add), has_config_authorization() );

  gtk_label_set_markup (GTK_LABEL (user_label), message);

  if ( ! has_allocate_authorization () )
  	sprintf(message, _("<b>%s</b> does not have allocation capabilities--see user_attr(4)"),
                User[current_user].p.pw_name);
  else
	sprintf(message, "");

  gtk_label_set_markup (GTK_LABEL (alloc_label), message);

  if (gtk_tree_selection_get_selected(selection, NULL, &iter))
  {
    reset_buttons(FALSE);
  
    sprintf(message, _("Device:"));
    g_object_set(admin_device, "label", message, NULL);
    sprintf(message, _("Owner:"));
    g_object_set(admin_owner, "label", message, NULL);
    sprintf(message, _("Label:"));
    g_object_set(admin_label, "label", message, NULL);
    if (hilighted_device_zone != NULL)
    {
      g_free(hilighted_device_zone);
      hilighted_device_zone = NULL;
    }
    gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
  }
  
  gtk_list_store_clear((GtkListStore *)model);

  model = create_and_fill_model(GTK_TREE_VIEW(lv), zonename, value);
  gtk_tree_view_set_model (GTK_TREE_VIEW (lv), model);

  if ( ! is_widget_pinned ( GTK_WIDGET(lv) ) )
    pin_the_widget( GTK_WIDGET(lv) );

  if (value == ALLOCATED_LIST)
  {
    if (g_signal_handler_is_connected(gworkspace, handler_id))
      g_signal_handler_disconnect(gworkspace, handler_id);
    handler_id = g_signal_connect(gworkspace, "label-changed", G_CALLBACK(label_changed_callback),  GINT_TO_POINTER(value));
    gworkspace = workspace;
  }
}

extern GtkDialog *dev_mgr_props_add_dialog_new(void);
extern void dev_mgr_props_add_reset(GtkWidget *);
extern void dev_mgr_props_get_values(GtkWidget *, char **, char **, char **, char **, char **, char **, char **, char **, char **, char **);

/* Need to add code to call the properties dialog */
void admin_add_callback(GtkWidget *widget, gpointer data)
{
	gint resp;
 	GtkDialog *add;
	GValue value = {0,};
	int i = 0;
	int ret;
	char *devfile;
	char *devmin;
	char *devmax;
	char *devauth;
	char *devowner;
	char *devall;
	char *devlabel;
	char *devclean;
	char *devtype;
	char *devname;
	char *cmd[20];
	char optionstr[256];
	GtkTreeView *lav = view[AVAILABLE_LIST];
	GtkTreeModel *lam = gtk_tree_view_get_model(lav);
	GdkScreen *screen = NULL;
	WnckScreen *wnckscreen = NULL;
	char message[256];
	
	global_props = add = (GtkDialog *)dev_mgr_props_add_dialog_new();

	gtk_widget_show_all(GTK_WIDGET(add));
	do 
	{
		switch ((resp = gtk_dialog_run(add)))
		{
			case GTK_RESPONSE_HELP:
	        		/* Popup help application */
				break;
			case DEVMGR_PROPS_RESET:
				dev_mgr_props_add_reset(GTK_WIDGET(add));
				break;
 	        	case GTK_RESPONSE_CANCEL:
                	        break;
                	case GTK_RESPONSE_OK:
                		/* Commit the Property Changes */
				/* /usr/sbin/add_allocatable [-f (modify mode)] -n devname -t devtype -l devlist -a authlist -c cleanprog -o min=hexMIN max=hexMAX */
				dev_mgr_props_get_values(GTK_WIDGET(add),
					&devname, &devtype, &devfile, 
					&devmin, &devmax, &devauth, 
					&devall, &devowner, &devlabel, 
					&devclean);
				if (devname == NULL ||
				    devfile == NULL ||
				    devtype == NULL ||
				    devauth == NULL ||
				    devclean == NULL ||
				    devmin == NULL ||
				    devmax == NULL)
				    	break ;
				cmd[i++] = "/usr/sbin/add_allocatable";
				cmd[i++] = "-n";
				cmd[i++] = devname;
				cmd[i++] = "-l";
				cmd[i++] = devfile;
				cmd[i++] = "-t";
				cmd[i++] = devtype;
				cmd[i++] = "-a";
				cmd[i++] = devauth;
				cmd[i++] = "-c";
				cmd[i++] = devclean;
				cmd[i++] = "-o";
				snprintf(optionstr, sizeof(optionstr), 
					"%s=%s:%s=%s",
					DAOPT_MINLABEL, devmin?devmin:"",
					DAOPT_MAXLABEL, devmax?devmax:"");
				cmd[i++] = optionstr;
				cmd[i++] = NULL;
#ifdef CONSOLE
fprintf(console, "%s:%d:checking cmd array\n", __FILE__, __LINE__);
for (i--; i > -1; i--)
	fprintf(console, "%s:%d:cmd[%d] = %s\n", __FILE__, __LINE__, i,
		cmd[i]?cmd[i]:"NULL");
#endif /* CONSOLE */
				run_command(cmd);
                		/* update the AVAILABLE list */
				last_mtime = 0;
				screen = gtk_widget_get_screen( GTK_WIDGET(add) );
				if (screen != NULL)
		  			wnckscreen = wnck_screen_get( gdk_screen_get_number (screen) );
				zonename = get_zonename(wnckscreen);
		
                		/* reset amin buttons and labels */
                		reset_buttons(FALSE);
                		gtk_widget_set_sensitive(GTK_WIDGET(admin_add), has_config_authorization() && TRUE);
                		sprintf(message, _("Device:"));
                		g_object_set(admin_device, "label", message, NULL);
                		sprintf(message, _("Owner:"));
                		g_object_set(admin_owner, "label", message, NULL);
                		sprintf(message, _("Label:"));
                		g_object_set(admin_label, "label", message, NULL);
                		if (hilighted_device_zone != NULL)
					g_free(hilighted_device_zone);
                		hilighted_device_zone = NULL;
                	        break;
                	default:
                	        break;
		}
	} while (resp != GTK_RESPONSE_CANCEL && resp != GTK_RESPONSE_OK);
	
	gtk_widget_hide(GTK_WIDGET(add));
	global_props = NULL;
}

/* Need a popup to warn of removal */
void admin_remove_callback(GtkWidget *widget, gpointer data)
{
    char *cmd[20];
    int  i = 0;
    char *devname;
    GtkTreeView *lav = view[ALLOCATED_LIST];
    GtkTreeSelection *selection = gtk_tree_view_get_selection(lav);
    GtkTreeModel *model = gtk_tree_view_get_model(lav);
    GtkTreeIter iter;
    GtkDialog *dialog;
    GtkDialog *devdialog = GTK_DIALOG(data);
    GtkWidget *text, *hbox, *warning_icon;
    char question[256];
    char *owner = NULL;
    int ltype;
    GdkScreen *screen = NULL;
    WnckScreen *wnckscreen = NULL;
    char message[256];
    	
    g_object_get(admin_owner, "label", &owner, NULL);
    if (strcmp(owner, "Owner:") == 0)
    {
    	lav = view[AVAILABLE_LIST];
    	selection = gtk_tree_view_get_selection(lav);
    	model = gtk_tree_view_get_model(lav);
    	ltype = AVAILABLE_LIST;
    }
    else
    {
     	lav = view[ALLOCATED_LIST];
    	selection = gtk_tree_view_get_selection(lav);
    	model = gtk_tree_view_get_model(lav);
    	ltype = ALLOCATED_LIST;
    }
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	GValue value = {0,};
	GtkTreeModel *lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));
	
        /* get the selected device */		
	gtk_tree_model_get_value(model, &iter, ALLOC_COL_DEVNAME, &value);
	devname = g_strdup(g_value_get_string(&value));
	
	/* double check that they want to delete the device */
	dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
			_("Confirm Device Removal"),
			GTK_WINDOW(devdialog),
			GTK_DIALOG_MODAL,
			GTK_STOCK_CANCEL, 	GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK,		GTK_RESPONSE_OK,
			NULL));
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_CANCEL);

	hbox = g_object_new(GTK_TYPE_HBOX, "border-width", 8, NULL);
	warning_icon = g_object_new(GTK_TYPE_IMAGE,
				"stock",	GTK_STOCK_DIALOG_WARNING,
				"icon-size",	GTK_ICON_SIZE_DIALOG,
				"xalign",	0.5,
				"yalign",	1.0,
				NULL);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(warning_icon), 
			FALSE, FALSE, 0);
	
	sprintf(question, _("Do you want to remove the %s device?"), devname);
	text = g_object_new(GTK_TYPE_LABEL,
			"wrap",		TRUE,
			"label",	question,
			NULL);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(text),
			FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialog->vbox), GTK_WIDGET(hbox), 
			FALSE, FALSE, 0);
	
	gtk_widget_show_all(GTK_WIDGET(dialog));
	switch (gtk_dialog_run(dialog))
	{
	    case GTK_RESPONSE_HELP:
	        /* Popup help application */
		break;
 	    case GTK_RESPONSE_CANCEL:
                break;
            case GTK_RESPONSE_OK:
                /* Commit the removal by running 
                   /usr/sbin/remove_allocatable -n devname 
                */
                cmd[i++] = "/usr/sbin/remove_allocatable";
                cmd[i++] = "-n";
                cmd[i++] = devname;
                cmd[i++] = NULL;
                run_command(cmd);
                last_mtime = 0;
                /* reset amin buttons and labels */
        	reset_buttons(FALSE);
                gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
		sprintf(message, _("Device:"));
		g_object_set(admin_device, "label", message, NULL);
		sprintf(message, _("Owner:"));
		g_object_set(admin_owner, "label", message, NULL);
		sprintf(message, _("Label:"));
		g_object_set(admin_label, "label", message, NULL);
		if (hilighted_device_zone != NULL)
			g_free(hilighted_device_zone);
		hilighted_device_zone = NULL;
                break;
            default:
                break;
	}
	gtk_widget_hide_all(GTK_WIDGET(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

void admin_reclaim_callback(GtkWidget *widget, gpointer data)
{
    char *cmd[20];
    int  i = 0;
    char *devname;
    GtkTreeView *lav = view[ALLOCATED_LIST];
    GtkTreeSelection *selection = gtk_tree_view_get_selection(lav);
    GtkTreeModel *model = gtk_tree_view_get_model(lav);
    GtkTreeIter iter;
    GdkScreen *screen = NULL;
    WnckScreen *wnckscreen = NULL;
    GtkDialog *dialog = GTK_DIALOG(data);
    char message[256];
    
    screen = gtk_widget_get_screen( GTK_WIDGET(dialog) );
    if (screen != NULL)
	wnckscreen = wnck_screen_get( gdk_screen_get_number (screen) );
    zonename = get_zonename(wnckscreen);

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	GValue value = {0,};
	GtkTreeModel *lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));
		
        /* get the selected device */		
	gtk_tree_model_get_value(model, &iter, AVAIL_COL_DEVNAME, &value);
	devname = g_strdup(g_value_get_string(&value));
		   
        /* run /usr/sbin/deallocate -w -s -F devname */
        cmd[i++] = "/usr/sbin/deallocate";
        cmd[i++] = "-w";
        cmd[i++] = "-s";
        cmd[i++] = "-F";
        cmd[i++] = devname;
        cmd[i++] = NULL;
        run_command(cmd);
        last_mtime = 0;
        /* update the allocated list */
	gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
	/* Reset the available list */
	lav = view[AVAILABLE_LIST];
	lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));
	
	/* reset amin buttons and labels */
        reset_buttons(FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
	sprintf(message, _("Device:"));
	g_object_set(admin_device, "label", message, NULL);
	sprintf(message, _("Owner:"));
	g_object_set(admin_owner, "label", message, NULL);
	sprintf(message, _("Label:"));
	g_object_set(admin_label, "label", message, NULL);
	if (hilighted_device_zone != NULL)
		g_free(hilighted_device_zone);
	hilighted_device_zone = NULL;
    }
}

void admin_revoke_callback(GtkWidget *widget, gpointer data)
{
    char *cmd[20];
    int  i = 0;
    char *devname;
    GtkTreeView *lav = view[ALLOCATED_LIST];
    GtkTreeSelection *selection = gtk_tree_view_get_selection(lav);
    GtkTreeModel *model = gtk_tree_view_get_model(lav);
    GtkTreeIter iter;
    GdkScreen *screen = NULL;
    WnckScreen *wnckscreen = NULL;
    GtkDialog *dialog = GTK_DIALOG(data);
    char message[256];

    screen = gtk_widget_get_screen( GTK_WIDGET(dialog) );
    if (screen != NULL)
	wnckscreen = wnck_screen_get( gdk_screen_get_number (screen) );
    zonename = get_zonename(wnckscreen);

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
	GValue value = {0,};
	GtkTreeModel *lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));
		
        /* get the selected device */		
	gtk_tree_model_get_value(model, &iter, AVAIL_COL_DEVNAME, &value);
	devname = g_strdup(g_value_get_string(&value));
		   
        /* run /usr/sbin/deallocate -w -s -F devname */
        cmd[i++] = "/usr/sbin/deallocate";
        cmd[i++] = "-w";
        cmd[i++] = "-s";
        cmd[i++] = "-F";
        cmd[i++] = devname;
        cmd[i++] = NULL;
        run_command(cmd);
       	last_mtime = 0;
        /* update the list */
	gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
	
	/* Reset the available list */
	lav = view[AVAILABLE_LIST];
	lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));

	/* reset admin buttons and labels */
	reset_buttons(FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(admin_revoke), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
	sprintf(message, _("Device:"));
	g_object_set(admin_device, "label", message, NULL);
	sprintf(message, _("Owner:"));
	g_object_set(admin_owner, "label", message, NULL);
	sprintf(message, _("Label:"));
	g_object_set(admin_label, "label", message, NULL);
	if (hilighted_device_zone != NULL)
		g_free(hilighted_device_zone);
	hilighted_device_zone = NULL;
    }
}

/* undefined dev_mgr_props_init() should really include the
   devmgr-props-dialog.h file but not sure if that is kosher 
   with this type of class extension/programming */
extern GtkDialog *dev_mgr_props_init(char *, char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern void dev_mgr_props_reset(GtkWidget *, char *, char *, char *, char *, char *, char *, char *, char *);

void admin_properties_callback(GtkWidget *widget, gpointer data)
{
	/* need to add the static variables for device and type to the
	   creation of the props dialog */ 
    gint resp;
    GtkDialog *props;
    char *devname;
    char *devtype;
    char *devfile;
    char *devmin;
    char *devmax;
    char *devauth;
    char *devclean;
    char *devdeall;
    char *devowner;
    char *devlabel;
    char *ndevfile, 
         *ndevmin, 
         *ndevmax, 
         *ndevauth, 
         *ndeveall, 
         *ndevowner, 
         *ndevlabel, 
         *ndevclean,
	 *ndevtype,
	 *ndevname;
    GValue value = {0,};
    int i = 0;
    int ret;
    GtkTreeView *lav = view[ALLOCATED_LIST];
    GtkTreeModel *lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(lav);
    GtkTreeIter iter;
    int ltype;
    GdkScreen *screen = NULL;
    WnckScreen *wnckscreen = NULL;
    char *owner;
    char *cmd[20];
    char optionstr[256];

    g_object_get(admin_owner, "label", &owner, NULL);
    if (strcmp(owner, "Owner:") == 0)
    {
    	lav = view[AVAILABLE_LIST];
    	selection = gtk_tree_view_get_selection(lav);
    	lam = gtk_tree_view_get_model(lav);
    	ltype = AVAILABLE_LIST;
    }
    else
    {
     	lav = view[ALLOCATED_LIST];
    	selection = gtk_tree_view_get_selection(lav);
    	lam = gtk_tree_view_get_model(lav);
    	ltype = ALLOCATED_LIST;
    }
    if (gtk_tree_selection_get_selected(selection, &lam, &iter))
    {
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVNAME:AVAIL_COL_DEVNAME, 
		&value);
	devname = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);

	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVTYPE:AVAIL_COL_DEVTYPE,
		&value);
	devtype = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVFILE:AVAIL_COL_DEVFILE, 
		&value);
	devfile = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVMINSL:AVAIL_COL_DEVMINSL, 
		&value);
	devmin = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVMAXSL:AVAIL_COL_DEVMAXSL, 
		&value);
	devmax = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVAUTH:AVAIL_COL_DEVAUTH, 
		&value);
	devauth = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVDEALL:AVAIL_COL_DEVDEALL, 
		&value);
	devdeall = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVOWNER:AVAIL_COL_DEVOWNER, 
		&value);
	devowner = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_LABEL:AVAIL_COL_DEVLABEL, 
		&value);
	devlabel = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);
	
	gtk_tree_model_get_value(lam, &iter, 
		ltype ? ALLOC_COL_DEVCLEAN:AVAIL_COL_DEVCLEAN, 
		&value);
	devclean = g_strdup(g_value_get_string(&value));
	g_value_unset(&value);

	global_props = props = (GtkDialog *)dev_mgr_props_dialog_new(devname, devtype, devfile, devmin, devmax, devauth, devdeall, devowner, devlabel, devclean);

	gtk_widget_show_all(GTK_WIDGET(props));
	do 
	{
		switch ((resp = gtk_dialog_run(props)))
		{
			case GTK_RESPONSE_HELP:
	        		/* Popup help application */
				break;
			case DEVMGR_PROPS_RESET:
				dev_mgr_props_reset(GTK_WIDGET(props), devfile, devmin, devmax, devauth, devdeall, devowner, devlabel, devclean);
				break;
 	        	case GTK_RESPONSE_CANCEL:
                	        break;
                	case GTK_RESPONSE_OK:
                		/* Commit the Property Changes */
                		dev_mgr_props_get_values(GTK_WIDGET(props),
					&ndevname, &ndevtype, &ndevfile, 
					&ndevmin, &ndevmax, &ndevauth, 
					&ndeveall, &ndevowner, &ndevlabel, 
					&ndevclean);
#ifdef CONSOLE
fprintf(console, "%s:%d:values\n", __FILE__, __LINE__);
fprintf(console, "\tndevname  = %s\n", ndevname?ndevname:"NULL");
fprintf(console, "\tndevfile  = %s\n", ndevfile?ndevfile:"NULL");
fprintf(console, "\tndevtype  = %s\n", ndevtype?ndevtype:"NULL");
fprintf(console, "\tndevauth  = %s\n", ndevauth?ndevauth:"NULL");
fprintf(console, "\tndevclean = %s\n", ndevclean?ndevclean:"NULL");
fprintf(console, "\tndevmax   = %s\n", ndevmax?ndevmax:"NULL");
fprintf(console, "\tndevmin   = %s\n", ndevmin?ndevmin:"NULL");
fprintf(console, "%s:%d:values done\n", __FILE__, __LINE__);
#endif /* CONSOLE */

				if (ndevname == NULL ||
				    ndevfile == NULL ||
				    ndevtype == NULL ||
				    ndevauth == NULL ||
				    ndevclean == NULL ||
				    ndevmax == NULL ||
				    ndevmin == NULL)
				    	break ;
				cmd[i++] = "/usr/sbin/add_allocatable";
				cmd[i++] = "-f";
				cmd[i++] = "-n";
				cmd[i++] = ndevname;
				cmd[i++] = "-l";
				cmd[i++] = ndevfile;
				cmd[i++] = "-t";
				cmd[i++] = ndevtype;
				cmd[i++] = "-a";
				cmd[i++] = ndevauth;
				cmd[i++] = "-c";
				cmd[i++] = ndevclean;
				cmd[i++] = "-o";
				snprintf(optionstr, sizeof(optionstr), 
					"%s=%s:%s=%s",
					DAOPT_MINLABEL, ndevmin?ndevmin:"",
					DAOPT_MAXLABEL, ndevmax?ndevmax:"");
				cmd[i++] = optionstr;
				cmd[i++] = NULL;
#ifdef CONSOLE
fprintf(console, "%s:%d:checking cmd array\n", __FILE__, __LINE__);
for (i--; i > -1; i--)
	fprintf(console, "%s:%d:cmd[%d] = %s\n", __FILE__, __LINE__, i,
		cmd[i]?cmd[i]:"NULL");
#endif /* CONSOLE */
				run_command(cmd);
				last_mtime = 0;
                	        break;
                	default:
                	        break;
		}
	} while (resp != GTK_RESPONSE_CANCEL && resp != GTK_RESPONSE_OK);
	
	gtk_widget_hide(GTK_WIDGET(props));
    }
    global_props = NULL;
}

/* Add code to popup the +add button */
void add_callback(GtkWidget *widget, gpointer data)
{
	GtkTreeView *lv = view[AVAILABLE_LIST];
	GtkTreeSelection *selection = gtk_tree_view_get_selection(lv);
	GtkTreeModel *model = gtk_tree_view_get_model(lv);
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		GValue value = {0,};
		char *devname;
		char *cmd[20];
		char *zonename;
		GdkScreen *screen;
		WnckWorkspace *workspace;
		WnckScreen *wnckscreen;
		int i = 0;
		int ret;
		GtkTreeView *lav = view[ALLOCATED_LIST];
		GtkTreeModel *lam = gtk_tree_view_get_model(GTK_TREE_VIEW(lav));
		char message[256];

		screen = gtk_widget_get_screen ( widget );
		
		if (screen != NULL)
		  wnckscreen = wnck_screen_get ( gdk_screen_get_number (screen) );
		zonename = get_zonename ( wnckscreen );
#ifdef CONSOLE
		fprintf(console, "%s:%d:zonename = %s\n", __FILE__, __LINE__, zonename);
#endif /* CONSOLE */

		gtk_tree_model_get_value(model, &iter, AVAIL_COL_DEVNAME, &value);
		devname = g_strdup(g_value_get_string(&value));
		
		cmd[i++] = "/usr/sbin/allocate";
		cmd[i++] = "-w";
		cmd[i++] = "-z";
		cmd[i++] = zonename;
		cmd[i++] = devname;
		cmd[i++] = NULL;
		(void)run_command(cmd);
		last_mtime = 0;
	  	reset_buttons(FALSE);
		sprintf(message, _("Device:"));
		g_object_set(admin_device, "label", message, NULL);
		gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
	}
}

/* add code to popup for the remove button */
void remove_callback(GtkWidget *widget, gpointer data)
{
	GtkTreeView *lav = view[ALLOCATED_LIST];
	GtkTreeSelection *selection = gtk_tree_view_get_selection(lav);
	GtkTreeModel *model = gtk_tree_view_get_model(lav);
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		GValue v1 = {0,};
		GValue v2 = {0,};
		char *devname;
		char *cmd[20];
		char *zonename;
		GdkScreen *screen;
		WnckWorkspace *workspace;
		WnckScreen *wnckscreen;
		int i = 0;
		int ret;
		GtkTreeView *lvv = view[AVAILABLE_LIST];
		GtkTreeModel *lvm = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv));
		char message[256];
		
		screen = gtk_widget_get_screen ( widget );
		if (screen != NULL)
		  wnckscreen = wnck_screen_get ( gdk_screen_get_number (screen) );
		
		gtk_tree_model_get_value(model, &iter, ALLOC_COL_DEVNAME, &v1);
		devname = g_strdup(g_value_get_string(&v1));
		gtk_tree_model_get_value(model, &iter, ALLOC_COL_ZONE, &v2);
		zonename = g_strdup(g_value_get_string(&v2));
		if (zonename == NULL)
		  zonename = get_zonename ( wnckscreen );

#ifdef CONSOLE
		fprintf(console, "%s:%d:zonename = %s\n", __FILE__, __LINE__, zonename?zonename:"NULL");
#endif /* CONSOLE */

		cmd[i++] = "/usr/sbin/deallocate";
		cmd[i++] = "-w";
		cmd[i++] = "-s";
		cmd[i++] = "-z";
		cmd[i++] = zonename;
		cmd[i++] = devname;
		cmd[i++] = NULL;
		ret = run_command(cmd);
		last_mtime = 0;
	  	reset_buttons(FALSE);
		sprintf(message, _("Device:"));
		g_object_set(admin_device, "label", message, NULL);
		sprintf(message, _("Owner:"));
		g_object_set(admin_owner, "label", message, NULL);
		sprintf(message, _("Label:"));
		g_object_set(admin_label, "label", message, NULL);
		if (hilighted_device_zone != NULL)
			g_free(hilighted_device_zone);
		hilighted_device_zone = NULL;
		gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
	}
}

GtkTreeSelection *allocated_selection, *not_allocated_selection;

/* need to enable Props, revoke, -remove, ... buttons and update
   the device and owner static strings
 */
gboolean add_selection_callback(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
	GtkButton *button = (GtkButton *)data;
	gboolean sensitivity;
	char message[256];
	GValue v1 = {0,};
	GtkTreeIter iter;

	/* not sure what to do here but Add>> needs 
	   to be sensitive at this point */	
	if ( ! selected )
	{
	  reset_buttons(TRUE);
	  gtk_widget_set_sensitive(GTK_WIDGET(admin_reclaim), FALSE);
	  gtk_widget_set_sensitive(GTK_WIDGET(admin_revoke), FALSE);
	  gtk_tree_selection_unselect_all(allocated_selection);
	  if (gtk_tree_model_get_iter(model, &iter, path))
	  {
	    gtk_tree_model_get_value(model, &iter, AVAIL_COL_DEVNAME, &v1);
	    sprintf(message, _("Device: %s"), g_value_get_string(&v1));
	    g_value_unset(&v1);
	    g_object_set(admin_device, "label", message, NULL);
	  }
	  sprintf(message, _("Owner:"));
	  g_object_set(admin_owner, "label", message, NULL);
	  sprintf(message, _("Label:"));
	  g_object_set(admin_label, "label", message, NULL);
	  gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE && has_allocate_authorization());
	  if (hilighted_device_zone != NULL)
	      g_free(hilighted_device_zone);
	  hilighted_device_zone = NULL;
	}
	else
	  gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	  
	return ( TRUE );
}

/* really need to insure only 1 selected item in the list */
gboolean remove_selection_callback(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
  	char *tree_path_str = gtk_tree_path_to_string(path);
	GtkButton *button = (GtkButton *)data;
	gboolean sensitivity;
	static char *previous_path = NULL;
	gboolean ret = TRUE;
	gboolean dev_owner_error = FALSE;
	char message[256];
	GtkTreeIter iter;
	GValue v1 = {0,};

	if ( ! selected )
	{
	  struct passwd  *p = getpwuid (User[current_user].p.pw_uid);
	  GdkColor	 *label_color;

	  reset_buttons(FALSE && (! selected));
	  gtk_tree_selection_unselect_all(not_allocated_selection);
	  gtk_widget_set_sensitive(GTK_WIDGET(admin_revoke), has_revoke_authorization() && (! selected));
	  if (gtk_tree_model_get_iter(model, &iter, path))
	  {
	    char *zonename;
	    char *labelname;
	    char *str;
	    int i;

	    g_value_unset(&v1);
	    gtk_tree_model_get_value(model, &iter, ALLOC_COL_DEVOWNER, &v1);
            str = (char *)g_value_get_string(&v1);
            sscanf(str, "%d", &i);
            if (strcmp(str, "/ERROR") != 0 && strcmp(str, "/INVALID") != 0)
            {
	      p = getpwuid(i);
	      sprintf(message, _("Owner: %s"), p != NULL?p->pw_name:"Unknown");
            } 
            else 
            {
	      sprintf(message, _("Owner: %s"), str+1);
              dev_owner_error = TRUE;
            }
	    g_object_set(admin_owner, "label", message, NULL);
	    g_value_unset(&v1);
	    gtk_tree_model_get_value(model, &iter, ALLOC_COL_DEVNAME, &v1);
	    sprintf(message, _("Device: %s"), g_value_get_string(&v1));
	    g_object_set(admin_device, "label", message, NULL);
	    g_value_unset(&v1);
	    gtk_tree_model_get_value(model, &iter, ALLOC_COL_LABEL, &v1);
            labelname = g_strdup(g_value_get_string(&v1));
	    sprintf(message, _("Label: <b>%s</b>"), labelname);
	    label_color = g_new (GdkColor, 1);
	    g_value_unset(&v1);
	    gtk_tree_model_get_value(model, &iter, ALLOC_COL_ZONE, &v1);
            zonename = g_strdup(g_value_get_string(&v1));
	    g_value_unset(&v1);
            if (zonename == NULL)
            {
              m_label_t     *workspace_sl = NULL;
              m_label_t     *tmp_sl = NULL;
              zoneid_t       zid;
              char           lz[ZONENAME_MAX];
              int            error = 0;

              str_to_label(labelname, &workspace_sl, MAC_LABEL, L_NO_CORRECTION, &error);
              if ((zid = getzoneidbylabel(workspace_sl)) > 0) {
                if (getzonenamebyid(zid, lz, sizeof (lz)) == -1) {
                  strcpy(lz, GLOBAL_ZONENAME);
                }
              } else {
                 strcpy(lz, GLOBAL_ZONENAME);
              }
              zonename = g_strdup(lz);
            }
            g_free(labelname);
	    gdk_color_parse((const char *)zonename, label_color);
	    if (hilighted_device_zone != NULL)
	      g_free(hilighted_device_zone);
	    hilighted_device_zone = g_strdup(zonename);
            g_free(zonename);
	    /* need to use a <span background='#nnnnnn'> to gain a label color */
	    g_object_set(admin_label, 
	    		"label", 		message, 
	    		NULL);
            gtk_widget_set_sensitive(GTK_WIDGET(admin_add), has_config_authorization() );
	    if (! dev_owner_error && p!= NULL && strcmp(p->pw_name, User[current_user].p.pw_name) == 0)
	      gtk_widget_set_sensitive(GTK_WIDGET(button), has_allocate_authorization());

	  }
	}
	else
	  gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
        if (dev_owner_error)
          gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE );
	 
	return ( ret );
}

#define BITSET(_bits, _bit)       (((_bits)&(_bit))==(_bit))

gint
sort_list(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	gint bits = GPOINTER_TO_INT(data);
	gint ret = 0;
	gchar *name1, *name2;
	gint sortcol;
	
	sortcol = BITSET(bits, 1<<DEVICE_COL)?DEVICE_COL:
		  BITSET(bits, 1<<IMAGE_COL)?IMAGE_COL:
		  BITSET(bits, 1<<LABEL_COL)?LABEL_COL:0;
	gtk_tree_model_get(model, a, sortcol, &name1, -1);
	gtk_tree_model_get(model, b, sortcol, &name2, -1);

	if (name1 == NULL || name2 == NULL)
	{
		if (name1 == NULL && name2 == NULL)
			ret = 0;
		else
			ret = (name1 == NULL) ? -1 : 1;
	} 
	else 
	{
		ret = g_utf8_collate(name1,name2);
		g_free(name1);
		g_free(name2);
	}
	
	return ( ret );
}


void
cell_label_set(GtkTreeViewColumn *col, GtkCellRenderer *cell, 
	        GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gchar *label;
	GdkColor *label_color;
	GValue v = {1,};

/*	
	colormap = gtk_widget_get_colormap(GTK_WIDGET(img));
	if (gdk_color_parse("black", &color))
	  if (gdk_colormap_alloc_color(colormap, &color, TRUE, TRUE))
	    gdk_gc_set_foreground(gc, &color);
	gdk_draw_rectangle(GDK_DRAWABLE(pix), gc, TRUE, 0, 0, 16, 16);	
*/
	gtk_tree_model_get(model, iter, ALLOC_COL_LABEL, &label, -1);
	label_color = g_new(GdkColor, 1);
	if (gdk_color_parse((const char *)label?label:"white", label_color))
	{
/*
	  if (gdk_colormap_alloc_color(colormap, &color, TRUE, TRUE))
	    gdk_gc_set_foreground(gc, &color);
	  gdk_draw_rectangle(GDK_DRAWABLE(pix), gc, TRUE, 1, 1, 14, 14);
*/	  
	}

	g_free(label);
	g_free(label_color);
}


void column_callback(GtkTreeViewColumn *column, gpointer data)
{
	static guint sorted = GTK_SORT_DESCENDING;
	static GtkTreeViewColumn *last = NULL;
	gint bits = GPOINTER_TO_INT(data);
	GtkTreeSortable *sortable;
	GtkListStore *ll;
	gint sortcol;
	
	if ((last != NULL) && last != column)
		g_object_set(last, "sort-indicator", FALSE, NULL);
	last = column;
	
	ll = list[BITSET(bits, 1<<(ALLOCATED_LIST+NUM_COLS))?ALLOCATED_LIST:AVAILABLE_LIST];
	sortcol = BITSET(bits, 1<<DEVICE_COL)?DEVICE_COL:
		  BITSET(bits, 1<<IMAGE_COL)?IMAGE_COL:
		  BITSET(bits, 1<<LABEL_COL)?LABEL_COL:0;

	sortable = GTK_TREE_SORTABLE(ll);

	g_object_set(column,
		"sort-order",		sorted,
		"sort-indicator",	TRUE,
		NULL);
		
	sorted = (sorted == GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING:
			GTK_SORT_ASCENDING;
			
	gtk_tree_sortable_set_sort_column_id(sortable, sortcol, sorted);
}

GtkListStore *create_list(gint whichlist)
{
	GtkListStore *ll;
	GtkTreeSortable *sortable;
	
	if (list[whichlist] != NULL)
		return ( list[whichlist] );
		
	/* need to add all the properties for a device to the list store but
	   not display the extra stuff */
	ll = list[whichlist] = gtk_list_store_new(NUM_COLS,
			GDK_TYPE_PIXBUF,
			G_TYPE_STRING,
			G_TYPE_STRING);
	sortable = GTK_TREE_SORTABLE(ll);
	gtk_tree_sortable_set_sort_func(sortable, DEVICE_COL, sort_list, GINT_TO_POINTER((1<<DEVICE_COL)+(1<<(whichlist+NUM_COLS))), NULL);
	gtk_tree_sortable_set_sort_func(sortable, LABEL_COL, sort_list, GINT_TO_POINTER((1<<LABEL_COL)+(1<<(whichlist+NUM_COLS))), NULL);

	return ( ll );
}

/* create allocated and available tree functions should be combined into
   one function.  This would allow me to load all the allocated devices
   and then remove them from the available devices.  */
GtkTreeView *create_allocated_tree(char *zonename)
{
	GtkListStore *ll;
	GtkLabel *label;
	GtkButton *button;
	GtkTreeIter iter;
	GtkCellRenderer *rcheck, *rimage, *rdevice, *rlabel;
	GtkTreeViewColumn *allocated_col, *image_col, *device_col, *label_col;
	GtkTreeSelection  *selection;
	GtkTreeSortable *sortable;
	GdkPixbuf *cdrom, *audio, *disk;
	gchar *cdrom_file, *audio_file, *disk_file;
	GtkTreeModel		*model;
	GtkTreeView	*lv = NULL;
	GtkTreeViewColumn 	*column;
	GtkCellRenderer *cell;

	if (view[ALLOCATED_LIST] == NULL)
	{
	    lv = view[ALLOCATED_LIST] = (GtkTreeView *)gtk_tree_view_new ();
  
	    column = gtk_tree_view_column_new ();
 	    gtk_tree_view_column_set_reorderable (column, TRUE);
 	    gtk_tree_view_column_set_title (column, _("Device"));

	    cell = gtk_cell_renderer_pixbuf_new ();
	    gtk_tree_view_column_pack_start (column, cell, FALSE);
	    gtk_tree_view_column_set_attributes (column, cell,
                                       "pixbuf", ALLOC_COL_IMAGE,
                                       NULL);

	    gtk_tree_view_column_set_resizable (column, TRUE);
	    cell = gtk_cell_renderer_text_new ();
	    gtk_tree_view_column_pack_start (column, cell, TRUE);
	    gtk_tree_view_column_set_attributes (column, cell,
                                       "text", ALLOC_COL_DEVNAME,
                                       NULL);
	    gtk_tree_view_column_set_sort_column_id (column, ALLOC_COL_DEVNAME);
	    gtk_tree_view_append_column (GTK_TREE_VIEW(lv), column);
	    
	    column = gtk_tree_view_column_new ();
 	    gtk_tree_view_column_set_reorderable (column, TRUE);
 	    gtk_tree_view_column_set_title (column, _("Label"));
 	    
 	    cell = gtk_cell_renderer_pixbuf_new ();
	    gtk_tree_view_column_pack_start (column, cell, FALSE);
	    gtk_tree_view_column_set_attributes (column, cell,
                                       "pixbuf", ALLOC_COL_IMAGE_LABEL,
                                       NULL);
#if 0
            gtk_tree_view_column_set_cell_data_func(column, cell,
            		cell_label_set, NULL, NULL);
#endif
            gtk_tree_view_column_set_resizable (column, TRUE);
	    cell = gtk_cell_renderer_text_new ();
	    gtk_tree_view_column_pack_start (column, cell, TRUE);
	    gtk_tree_view_column_set_attributes (column, cell,
                                       "text", ALLOC_COL_LABEL,
                                       NULL);
	    gtk_tree_view_column_set_sort_column_id (column, ALLOC_COL_LABEL);
	    gtk_tree_view_append_column (GTK_TREE_VIEW(lv), column);
	}

	model = create_and_fill_model (GTK_TREE_VIEW(lv), zonename, ALLOCATED_LIST);

	gtk_tree_view_set_model (GTK_TREE_VIEW (lv), model);

  /* The tree view has acquired its own reference to the
   *  model, so we can drop ours. That way the model will
   *  be freed automatically when the tree view is destroyed */

	g_object_unref (G_OBJECT(model));

	return ( lv );
}

/* Same as above. */
GtkTreeView *create_available_tree(char *zonename)
{

	GtkListStore *ll;
	GtkLabel *label;
	GtkButton *button;
	GtkTreeIter iter;
	GtkCellRenderer *rcheck, *rimage, *rdevice, *rlabel;
	GtkTreeViewColumn *allocated_col, *image_col, *device_col, *label_col;
	GtkTreeSelection  *selection;
	GtkTreeSortable *sortable;
	GdkPixbuf *cdrom, *audio, *disk;
	gchar *cdrom_file, *audio_file, *disk_file;
	GtkTreeModel		*model;
	GtkTreeViewColumn 	*column;
	GtkCellRenderer *cell;
	GtkTreeView		*lv;
	
	if (view[AVAILABLE_LIST] == NULL)
	{
	    lv = view[AVAILABLE_LIST] = (GtkTreeView *)gtk_tree_view_new ();
  
	    column = gtk_tree_view_column_new ();
 	    gtk_tree_view_column_set_reorderable (column, TRUE);
 	    gtk_tree_view_column_set_title (column, _("Device"));

	    cell = gtk_cell_renderer_pixbuf_new ();
	    gtk_tree_view_column_pack_start (column, cell, FALSE);
	    gtk_tree_view_column_set_attributes (column, cell,
                                       "pixbuf", AVAIL_COL_IMAGE,
                                       NULL);

	    gtk_tree_view_column_set_resizable (column, TRUE);
	    cell = gtk_cell_renderer_text_new ();
	    gtk_tree_view_column_pack_start (column, cell, TRUE);
	    gtk_tree_view_column_set_attributes (column, cell,
                                       "text", AVAIL_COL_DEVNAME,
                                       NULL);
	    gtk_tree_view_column_set_sort_column_id (column, AVAIL_COL_DEVNAME);
	    gtk_tree_view_append_column (GTK_TREE_VIEW(lv), column);
	}

	model = create_and_fill_model (GTK_TREE_VIEW(lv), zonename, AVAILABLE_LIST);

	gtk_tree_view_set_model (GTK_TREE_VIEW (lv), model);

  /* The tree view has acquired its own reference to the
   *  model, so we can drop ours. That way the model will
   *  be freed automatically when the tree view is destroyed */

	g_object_unref (G_OBJECT(model));

	return ( lv );
}

gboolean check_for_new_devices(char *zname)
{
	gboolean ret = FALSE;
	time_t current_mtime;

	current_mtime = get_latest_mtime(time_stamp_file);
	if (current_mtime != last_mtime)
	{
		last_mtime = current_mtime;
		ret = TRUE ;
	}

	return ( ret );
}

static gboolean update_timeout_func(gpointer data)
{
	GtkTreeView *lvv1 = view[AVAILABLE_LIST];
	GtkTreeModel *lvm = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv1));
	GtkTreeView *lvv2 = view[ALLOCATED_LIST];
	GtkTreeModel *lva = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv2));
	char *zonename = (char *)data;

        load_devices((GtkListStore *)lva, zonename, ALLOCATED_LIST);
	load_devices((GtkListStore *)lvm, zonename, AVAILABLE_LIST);
	
	return ( FALSE );
}

gboolean timeout_func(gpointer data)
{
	GdkScreen *screen = NULL;
	WnckScreen *wnckscreen = NULL;
	char *zonename;
	GtkTreeView *lvv1 = view[AVAILABLE_LIST];
	GtkTreeModel *lvm = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv1));
	GtkTreeView *lvv2 = view[ALLOCATED_LIST];
	GtkTreeModel *lva = gtk_tree_view_get_model(GTK_TREE_VIEW(lvv2));
	GtkDialog *dialog = (GtkDialog *)data;

	screen = gtk_widget_get_screen( GTK_WIDGET(dialog) );
	if (screen != NULL)
	    wnckscreen = wnck_screen_get( gdk_screen_get_number (screen) );
	zonename = get_zonename(wnckscreen);


	if (check_for_new_devices(zonename))
	{
	  gtk_widget_set_sensitive(GTK_WIDGET(device_add), FALSE);
	  gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);
          reset_buttons(FALSE);
          gtk_widget_set_sensitive(GTK_WIDGET(admin_add), has_config_authorization());
	  gtk_list_store_clear((GtkListStore *)lvm);
          gtk_list_store_clear((GtkListStore *)lva);
	  g_timeout_add(100, update_timeout_func, zonename);
	}
	return ( TRUE );
}

static void
get_time_stamp_filename(void)
{
	char buf[BUFSIZ];
	FILE *ptr;
	char cmd[1024];

	/* get the time_stamp_file name */
	snprintf(cmd, sizeof(cmd), "/usr/sbin/list_devices -w");
	if ((ptr = popen(cmd, "r")) != NULL) {
		if (fgets(buf, BUFSIZ, ptr) != NULL) {
		/* remove trailing newline */
			int i;
			i = strlen(buf);
			if ( i > 0 && buf[i-1] == '\n') {
				buf[i-1]= '\0';
			}
			time_stamp_file = strdup(buf);
#ifdef DEBUG
            		fprintf(stderr, "time_stamp_file = %s\n", time_stamp_file);
#endif /* DEBUG */
       		} else {
			perror("devmgr/get_time_stamp_filename(): fgets() failed.");
		}
		(void) pclose(ptr);
	}else {
		perror("devmgr: failed to execute list_device -w");
	}
}

char *
getusrattrval(userattr_t *uattr, char *keywd)
{
        char *value;

        if (uattr != NULL) {
                value = kva_match(uattr->attr, keywd);
                if (value != NULL)
                        return value; /* found from user_attr */
        }

        return ( NULL );
}

static void
get_user_and_roles(void)
{
        int             errorcode = 0;
        char           *username;
        struct passwd  *p;
        userattr_t      *u;
        char           *s1, *s2;
        char           *roles;

        if ((logname = getenv("LOGNAME")) == NULL)
                errorcode = 1;
        getpwnam_r(logname, &User[user_index].p, User[user_index].pw_buffer, PW_BUF_LEN, &p);
        User[user_index].u = u = getusernam(logname);
        UserCount = 1;

        roles = getusrattrval(u, USERATTR_ROLES_KW);
        if (roles == NULL)
                roles = "";

        s1 = s2 = strdup(roles);
        while ((s1 = strtok(s1, ",")) != NULL) {
                struct passwd  *p = NULL;
                int             i;

                if (!strncmp(s1, "none", 4))
                        break;

                for (i = 0; i < UserCount; i++) {
                        if (!strcmp(s1, User[i].p.pw_name)) {
                                p = &User[i].p;
                                break;
                        }
                }
                if (p == NULL) {
                        WmUser         *U = &User[UserCount];
                        p = &U->p;
                        getpwnam_r(s1, p, U->pw_buffer, PW_BUF_LEN, &p);
                        if (p == NULL){
                                s1 = NULL;
                                continue;
                        }
                        u = U->u = getusernam(s1);
                        U->do_accred_check =
                            chkauthattr(SYS_ACCRED_SET_AUTH, s1) == 1 ?
                            False : True;
                        UserCount++;
                }
                s1 = NULL;
        }
        free(s2);
}

static void
devmgr_dialog_instance_init (DevMgrDialog *devmgr_dialog) 
{
	GdkPixbuf *pixbuf;
	GtkWidget *label, *image;
	GtkVBox *vbox, *vbox2;
	GtkHBox *hbox;
	GtkTable *dtable, *table;
	GtkLabel *available_label, *allocated_label;
	GtkWidget *scroller, *text_view, *combo;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	char *str;
	DevMgrDialogDetails *details;
	char message[256];
	struct passwd *p;
	char pw_buffer[256];
	char *logname;
	GdkScreen *screen;
	WnckScreen *wnckscreen = NULL;
	WnckWorkspace *workspace = NULL;
	char *zonename;
	GdkWindow *gdkwindow;
        char *dpyenv;
	GError *err;
	GList *diconlist;

	get_time_stamp_filename();

	GtkDialog *dialog = GTK_DIALOG (devmgr_dialog);

        global_dialog = dialog;

	gtk_dialog_add_buttons (dialog, 
			GTK_STOCK_HELP, 	GTK_RESPONSE_HELP,
/*
			GTK_STOCK_PROPERTIES,	DEVMGR_PROPERTIES,
*/
			GTK_STOCK_CANCEL, 	GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, 		GTK_RESPONSE_OK, 
			NULL);
	
	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);
	gtk_window_set_default_size(GTK_WINDOW(dialog), DEFAULT_WIDTH, DEFAULT_HEIGHT);
		
	gtk_window_set_title (GTK_WINDOW (dialog), _("Device Manager"));

	g_signal_connect(dialog, "destroy", G_CALLBACK(quit_callback), NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(delete_event), NULL);

	screen = gtk_widget_get_screen ( GTK_WIDGET(dialog) );

	if (screen != NULL)
		wnckscreen = wnck_screen_get ( gdk_screen_get_number (screen) );
	
	zonename = get_zonename(wnckscreen);
	
	gtk_window_stick ( GTK_WINDOW (dialog) );
	
	vbox = GTK_VBOX(dialog->vbox);
	
	dtable = g_object_new(GTK_TYPE_TABLE,
			"n-rows",	7,
			"n-columns",	1,
			NULL);

	if ((dpyenv = getenv ("DISPLAY")) != NULL) {
		/* isolate the display number, dropping the hostname
		   and screen number if they are present */
		(void) strncpy (displayNumber, strchr (dpyenv, ':') +1,
			       sizeof (displayNumber));
		if ((dpyenv = strchr (displayNumber, '.')) != NULL)
			*dpyenv = '\0';
	} else {
		strncpy (displayNumber, "0", sizeof (displayNumber));
	}

        get_user_and_roles();
	current_user = get_current_user( wnck_screen_get_active_workspace( wnckscreen ) );

	user_label = (GtkLabel *)gtk_label_new (NULL);
	sprintf(message, _("Devices Allocatable by: <b>%s</b>"),
		User[current_user].p.pw_name);
	
	gtk_label_set_markup (GTK_LABEL (user_label), message);
	
	gtk_table_attach(dtable, GTK_WIDGET(user_label), 0, 1, 0, 1,
		GTK_FILL, GTK_FILL, 0, 0);

	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(dtable), TRUE, TRUE, 0);

	/* Add message for users who do not have allocation capabilities */
	alloc_label = (GtkLabel *)gtk_label_new (NULL);
	if (! has_allocate_authorization() )
  		sprintf(message, _("<b>%s</b> does not have allocation capabilities--see user_attr(4)"),
			User[current_user].p.pw_name);
	else
		sprintf(message, "%s", User[current_user].p.pw_name);

	gtk_label_set_markup (GTK_LABEL (alloc_label), message);
	
	gtk_table_attach(dtable, GTK_WIDGET(alloc_label), 0, 1, 1, 2,
		GTK_FILL, GTK_FILL, 0, 0);

	table = g_object_new(GTK_TYPE_TABLE,
			"n-rows",	2,
			"n-columns",	3,
			NULL);
	
	available_label = g_object_new(GTK_TYPE_LABEL,
			"label",		_("Available:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (available_label), 0.0, 0.0);
	allocated_label = g_object_new(GTK_TYPE_LABEL,
			"label",		_("Allocated:"),
			NULL);
	gtk_misc_set_alignment (GTK_MISC (allocated_label), 0.0, 0.0);

	gtk_table_attach(table, GTK_WIDGET(available_label), 0, 1, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 10, 0);
	gtk_table_attach(table, GTK_WIDGET(allocated_label), 2, 3, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK, 10, 0);
			
	view = create_available_tree(zonename);
	g_signal_connect(wnckscreen, "active-workspace-changed", G_CALLBACK(workspace_changed_callback),  GINT_TO_POINTER(AVAILABLE_LIST));
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	
	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW, NULL);

	gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(view));
	gtk_table_attach(table, GTK_WIDGET(scroller), 0, 1, 1, 2, 
		GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 0);
	
	vbox2 = (GtkVBox *)gtk_vbox_new(FALSE, 0);
		
	device_add = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("_Add >>"),
			"use-underline",	TRUE,
			"sensitive",		FALSE, /* No device selected yet */
			NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(device_add), FALSE);

	not_allocated_selection = selection;
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)add_selection_callback, device_add, NULL);
#if 0
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
#endif

	device_remove = g_object_new(GTK_TYPE_BUTTON,
			"label",	_("<< _Remove"),
			"use-underline",	TRUE,
			"sensitive",		FALSE, /* No device selected yet */
			NULL);
	gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(device_add), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(device_remove), FALSE, FALSE, 10);
	gtk_widget_set_sensitive(GTK_WIDGET(device_remove), FALSE);

	g_signal_connect(device_add, "clicked", G_CALLBACK(add_callback), NULL);	
	g_signal_connect(device_remove, "clicked", G_CALLBACK(remove_callback), NULL);
	
	gtk_table_attach(table, GTK_WIDGET(vbox2), 1, 2, 1, 2,
			GTK_SHRINK, GTK_SHRINK, 0, 0);

	view = create_allocated_tree(zonename);

	g_signal_connect(wnckscreen, "active-workspace-changed", G_CALLBACK(workspace_changed_callback), GINT_TO_POINTER(ALLOCATED_LIST));
        gworkspace = wnck_screen_get_active_workspace( wnckscreen );
        handler_id = g_signal_connect(gworkspace, "label-changed", G_CALLBACK(label_changed_callback),  GINT_TO_POINTER(ALLOCATED_LIST));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
#if 0
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE | GTK_SELECTION_SINGLE);
#endif
	allocated_selection = selection;
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)remove_selection_callback,GINT_TO_POINTER(device_remove), NULL);
	
	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW, 
			NULL);
	
	gtk_container_add(GTK_CONTAINER(scroller), GTK_WIDGET(view));
	gtk_table_attach(table, GTK_WIDGET(scroller), 2, 3, 1, 2, 
		GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 0);

	gtk_table_attach(dtable, GTK_WIDGET(table), 0, 1, 2, 3,
		GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
			
	expander = g_object_new(GTK_TYPE_EXPANDER,
			"label",	_("Administration"),
			NULL);
	gtk_table_attach(dtable, GTK_WIDGET(expander), 0, 1, 3, 4,
		GTK_FILL, GTK_FILL, 10, 10);
	expanded_view = FALSE;

	if (! ( has_config_authorization() || has_revoke_authorization() ) )
		g_object_set(expander, "sensitive", FALSE, NULL);
		
	expander_table = g_object_new(GTK_TYPE_TABLE, 
		"n-rows",	2,
		"n-columns",	5,
		NULL);
	gtk_container_add(GTK_CONTAINER(expander), GTK_WIDGET(expander_table));
	/*
	gtk_table_attach(dtable, GTK_WIDGET(expander_table), 0, 1, 3, 4,
		GTK_EXPAND|GTK_FILL, GTK_SHRINK, 5, 5);
	*/
	sprintf(message, _("Device:"));
	admin_device = g_object_new(GTK_TYPE_LABEL, "label", message, NULL);
	gtk_misc_set_alignment(GTK_MISC(admin_device), 0.0, 0.0);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_device), 0, 1, 0, 1,
			GTK_FILL, GTK_SHRINK, 5, 5);
	sprintf(message, _("Owner: "));
	admin_owner = g_object_new(GTK_TYPE_LABEL, "label", message, NULL);
	gtk_misc_set_alignment (GTK_MISC (admin_owner), 0.0, 0.0);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_owner), 1, 2, 0, 1,
			GTK_FILL, GTK_SHRINK, 5, 5);
	label = g_object_new(GTK_TYPE_LABEL, NULL);
	gtk_table_attach_defaults(expander_table, GTK_WIDGET(label), 1, 2, 0, 1);
	sprintf(message, _("Label: "));
	admin_label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (admin_label), message);
	gtk_misc_set_alignment (GTK_MISC (admin_label), 0.0, 0.0);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_label), 2, 5, 0, 1,
			GTK_FILL, GTK_SHRINK, 5, 5);

	g_signal_connect(G_OBJECT(admin_label), "expose_event", G_CALLBACK(label_expose), NULL);

	admin_add = gtk_button_new_from_stock (GTK_STOCK_ADD);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_add), 0, 1, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_SHRINK, 5, 0);

	admin_remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_remove), 1, 2, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	admin_reclaim = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("R_eclaim"),
			"use-underline",	TRUE,
			NULL);

	gtk_table_attach(expander_table, GTK_WIDGET(admin_reclaim), 2, 3, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);

	admin_revoke = g_object_new(GTK_TYPE_BUTTON,
			"label",		_("Re_voke"),
			"use-underline",	TRUE,
			NULL);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_revoke), 3, 4, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

	admin_properties = gtk_button_new_from_stock (GTK_STOCK_PROPERTIES);
	gtk_table_attach(expander_table, GTK_WIDGET(admin_properties), 4, 5, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_SHRINK, 5, 0);
	
	g_signal_connect (G_OBJECT(admin_add), "clicked", G_CALLBACK(admin_add_callback), NULL);
	g_signal_connect (G_OBJECT(admin_remove), "clicked", G_CALLBACK(admin_remove_callback), dialog);
	g_signal_connect (G_OBJECT(admin_reclaim), "clicked", G_CALLBACK(admin_reclaim_callback), NULL);
	g_signal_connect (G_OBJECT(admin_revoke), "clicked", G_CALLBACK(admin_revoke_callback), NULL);
	g_signal_connect (G_OBJECT(admin_properties), "clicked", G_CALLBACK(admin_properties_callback), NULL);

        reset_buttons(FALSE);	
	g_signal_connect (G_OBJECT(expander), "activate", G_CALLBACK(expand_callback), dialog);

#ifdef INTERVAL_TIMEOUT
	g_timeout_add(INTERVAL_TIMEOUT, timeout_func, dialog);
#endif
	last_mtime = get_latest_mtime(time_stamp_file);

	err = NULL;
        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
				      "gnome-dev-removable", 48, 0, &err);
	if (pixbuf == NULL)
	{
		g_printerr (_("Could not load icon \"%s\": %s\n"),
       		     "gnome-dev-removable", err->message);
		g_error_free (err);
	}

	diconlist = NULL;
	diconlist = g_list_prepend (diconlist, pixbuf);

	gtk_window_set_default_icon_list (diconlist);

	g_list_free (diconlist);
	g_object_unref (G_OBJECT (pixbuf));
}

static void
devmgr_dialog_class_init (DevMgrDialogClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	
	gobject_class->finalize = devmgr_dialog_finalize;
}

GtkWidget *
dev_mgr_dialog_new ()
{
	GtkWidget *dialog = g_object_new (DEVMGR_TYPE_DIALOG, 
			"default-width",	DEFAULT_WIDTH,
			"default-height",	DEFAULT_HEIGHT,
			NULL);
	DevMgrDialogDetails *details = DEVMGR_DIALOG (dialog)->details;
        
	return dialog;
}

