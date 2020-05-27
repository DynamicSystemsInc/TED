/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.
  Copyright (c) 2020, Dynamic Systems, Inc.

  The contents of this library are subject to the terms of the
  GNU Lesser General Public License version 2 (the "License")
  as published by the Free Software Foundation. You may not use
  this library except in compliance with the License.

  You should have received a copy of the License along with this
  library; see the file COPYING.  If not,you can obtain a copy
  at http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html or by writing
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA. See the License for specific language
  governing permissions and limitations under the License.
*/

#ifndef LABEL_BUILDER_H
#define LABEL_BUILDER_H

G_BEGIN_DECLS

#include <tsol/label.h>
#include <glib.h>
#include <gtk/gtk.h>

#define GNOME_TYPE_LABEL_BUILDER            (gnome_label_builder_get_type ())
#define GNOME_LABEL_BUILDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_LABEL_BUILDER, GnomeLabelBuilder))
#define GNOME_LABEL_BUILDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_LABEL_BUILDER, GnomeLabelBuilderClass))
#define GNOME_IS_LABEL_BUILDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_LABEL_BUILDER))
#define GNOME_IS_LABEL_BUILDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_LABEL_BUILDER))

enum {
	LBUILD_MODE_SL,
	LBUILD_MODE_CLR
};

enum {
	PROP_LBUILD_0,
	PROP_MODE,
	PROP_MESSAGE,
	PROP_SL,
	PROP_CLR,
	PROP_UPPER_BOUND,
	PROP_LOWER_BOUND
};

typedef struct _GnomeLabelBuilderDetails	GnomeLabelBuilderDetails;

typedef struct _GnomeLabelBuilder {
        GtkDialog parent;
	
        GnomeLabelBuilderDetails *details;
} GnomeLabelBuilder;

typedef struct _GnomeLabelBuilderClass {
	GtkDialogClass parent_class;

} GnomeLabelBuilderClass;

GType gnome_label_builder_get_type (void);
GtkWidget *gnome_label_builder_new (char *msg, blevel_t *upper,
				    blevel_t *lower, int mode);

gboolean gnome_label_builder_show_help (GtkWidget *w);
G_END_DECLS

#endif /* LABEL_BUILDER_H */

