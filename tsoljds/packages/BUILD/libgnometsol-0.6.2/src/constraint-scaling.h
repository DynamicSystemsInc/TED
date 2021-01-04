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

#ifndef __CONSTRAINT_SCALING_H__
#define __CONSTRAINT_SCALING_H__
#include <gtk/gtk.h>

/* constraint scaling stuff */
typedef struct _ConstraintImage ConstraintImage;
struct _ConstraintImage
{
  gchar     *filename;
  GdkPixbuf *pixbuf;
  GdkPixbuf *scaled;
  gboolean   stretch;
  gint       border_left;
  gint       border_right;
  gint       border_bottom;
  gint       border_top;
  guint      hints[3][3];
  gboolean   recolorable;
  GdkRGBA   colorize_color;
  gboolean   use_as_bkg_mask;
};

typedef struct _HighlightStripe HighlightStripe;

struct _HighlightStripe
{
  ConstraintImage *image;
  char           *name;
};

typedef enum {
  CONSTRAINT_CONSTANT_ROWS = 1 << 0,
  CONSTRAINT_CONSTANT_COLS = 1 << 1,
  CONSTRAINT_MISSING = 1 << 2
} ConstraintHints;

typedef enum
{
  COMPONENT_NORTH_WEST = 1 << 0,
  COMPONENT_NORTH      = 1 << 1,
  COMPONENT_NORTH_EAST = 1 << 2,
  COMPONENT_WEST       = 1 << 3,
  COMPONENT_CENTER     = 1 << 4,
  COMPONENT_EAST       = 1 << 5,
  COMPONENT_SOUTH_EAST = 1 << 6,
  COMPONENT_SOUTH      = 1 << 7,
  COMPONENT_SOUTH_WEST = 1 << 8,
  COMPONENT_ALL           = 1 << 9
} ConstraintComponent;

void
gnome_tsol_render_coloured_label (cairo_t *cr, GtkWidget *label);

void
gnome_tsol_render_coloured_label_for_zone (GtkWidget *label, const char *zonename);

void
gnome_tsol_constraint_image_render (cairo_t *cr,
				    ConstraintImage *cimage,
				    GdkWindow    *window,
				    GdkRectangle *clip_rect,
				    gboolean      center,
				    gint          x,
				    gint          y,
				    gint          width,
				    gint          height);

void
gnome_tsol_constraint_image_set_border (ConstraintImage *pb,
					gint         left,
					gint         right,
					gint         top,
					gint         bottom);

void
gnome_tsol_constraint_image_set_stretch (ConstraintImage *pb,
					 gboolean     stretch);

void
gnome_tsol_constraint_image_colorize (ConstraintImage *image,
				      GdkRGBA  *color,
				      int	alpha,
				      gboolean   use_alpha);

#endif /*__CONSTRAINT_SCALING_H__*/
