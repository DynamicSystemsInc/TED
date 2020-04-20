/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

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

#include <string.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "constraint-scaling.h"
#include "tsol-pics.h"

static GdkPixbuf *
bilinear_gradient (GdkPixbuf * src,
		   gint src_x,
		   gint src_y,
		   gint width,
		   gint height)
{
	guint           n_channels = gdk_pixbuf_get_n_channels (src);
	guint           src_rowstride = gdk_pixbuf_get_rowstride (src);
	guchar         *src_pixels = gdk_pixbuf_get_pixels (src);
	guchar         *p1, *p2, *p3, *p4;
	guint           dest_rowstride;
	guchar         *dest_pixels;
	GdkPixbuf      *result;
	int             i, j, k;

	p1 = src_pixels + (src_y - 1) * src_rowstride + (src_x - 1) * n_channels;
	p2 = p1 + n_channels;
	p3 = src_pixels + src_y * src_rowstride + (src_x - 1) * n_channels;
	p4 = p3 + n_channels;

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
				 width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (result);
	dest_pixels = gdk_pixbuf_get_pixels (result);

	for (i = 0; i < height; i++) {
		guchar         *p = dest_pixels + dest_rowstride * i;
		guint           v[4];
		gint            dv[4];

		for (k = 0; k < n_channels; k++) {
			guint           start = ((height - i) * p1[k] + (1 + i) * p3[k]) / (height + 1);
			guint           end = ((height - i) * p2[k] + (1 + i) * p4[k]) / (height + 1);

			dv[k] = (((gint) end - (gint) start) << 16) / (width + 1);
			v[k] = (start << 16) + dv[k] + 0x8000;
		}

		for (j = width; j; j--) {
			for (k = 0; k < n_channels; k++) {
				*(p++) = v[k] >> 16;
				v[k] += dv[k];
			}
		}
	}

	return result;
}

static GdkPixbuf *
horizontal_gradient (GdkPixbuf * src,
		     gint src_x,
		     gint src_y,
		     gint width,
		     gint height)
{
	guint           n_channels = gdk_pixbuf_get_n_channels (src);
	guint           src_rowstride = gdk_pixbuf_get_rowstride (src);
	guchar         *src_pixels = gdk_pixbuf_get_pixels (src);
	guint           dest_rowstride;
	guchar         *dest_pixels;
	GdkPixbuf      *result;
	int             i, j, k;

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
				 width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (result);
	dest_pixels = gdk_pixbuf_get_pixels (result);

	for (i = 0; i < height; i++) {
		guchar         *p = dest_pixels + dest_rowstride * i;
		guchar         *p1 = src_pixels + (src_y + i) * src_rowstride + (src_x - 1) * n_channels;
		guchar         *p2 = p1 + n_channels;

		guint           v[4];
		gint            dv[4];

		for (k = 0; k < n_channels; k++) {
			dv[k] = (((gint) p2[k] - (gint) p1[k]) << 16) / (width + 1);
			v[k] = (p1[k] << 16) + dv[k] + 0x8000;
		}

		for (j = width; j; j--) {
			for (k = 0; k < n_channels; k++) {
				*(p++) = v[k] >> 16;
				v[k] += dv[k];
			}
		}
	}

	return result;
}

static GdkPixbuf *
vertical_gradient (GdkPixbuf * src,
		   gint src_x,
		   gint src_y,
		   gint width,
		   gint height)
{
	guint           n_channels = gdk_pixbuf_get_n_channels (src);
	guint           src_rowstride = gdk_pixbuf_get_rowstride (src);
	guchar         *src_pixels = gdk_pixbuf_get_pixels (src);
	guchar         *top_pixels, *bottom_pixels;
	guint           dest_rowstride;
	guchar         *dest_pixels;
	GdkPixbuf      *result;
	int             i, j;

	top_pixels = src_pixels + (src_y - 1) * src_rowstride + (src_x) * n_channels;
	bottom_pixels = top_pixels + src_rowstride;

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
				 width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (result);
	dest_pixels = gdk_pixbuf_get_pixels (result);

	for (i = 0; i < height; i++) {
		guchar         *p = dest_pixels + dest_rowstride * i;
		guchar         *p1 = top_pixels;
		guchar         *p2 = bottom_pixels;

		for (j = width * n_channels; j; j--)
			*(p++) = ((height - i) * *(p1++) + (1 + i) * *(p2++)) / (height + 1);
	}

	return result;
}

static GdkPixbuf *
replicate_single (GdkPixbuf * src,
		  gint src_x,
		  gint src_y,
		  gint width,
		  gint height)
{
	guint           n_channels = gdk_pixbuf_get_n_channels (src);
	guchar         *pixels = (gdk_pixbuf_get_pixels (src) +
				  src_y * gdk_pixbuf_get_rowstride (src) +
				  src_x * n_channels);
	guchar          r = *(pixels++);
	guchar          g = *(pixels++);
	guchar          b = *(pixels++);
	guint           dest_rowstride;
	guchar         *dest_pixels;
	guchar          a = 0;
	GdkPixbuf      *result;
	int             i, j;

	if (n_channels == 4)
		a = *(pixels++);

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
				 width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (result);
	dest_pixels = gdk_pixbuf_get_pixels (result);

	for (i = 0; i < height; i++) {
		guchar         *p = dest_pixels + dest_rowstride * i;

		for (j = 0; j < width; j++) {
			*(p++) = r;
			*(p++) = g;
			*(p++) = b;

			if (n_channels == 4)
				*(p++) = a;
		}
	}

	return result;
}

static GdkPixbuf *
replicate_rows (GdkPixbuf * src,
		gint src_x,
		gint src_y,
		gint width,
		gint height)
{
	guint           n_channels = gdk_pixbuf_get_n_channels (src);
	guint           src_rowstride = gdk_pixbuf_get_rowstride (src);
	guchar         *pixels = (gdk_pixbuf_get_pixels (src) + src_y * src_rowstride + src_x * n_channels);
	guchar         *dest_pixels;
	GdkPixbuf      *result;
	guint           dest_rowstride;
	int             i;

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
				 width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (result);
	dest_pixels = gdk_pixbuf_get_pixels (result);

	for (i = 0; i < height; i++)
		memcpy (dest_pixels + dest_rowstride * i, pixels, n_channels * width);

	return result;
}

static GdkPixbuf *
replicate_cols (GdkPixbuf * src,
		gint src_x,
		gint src_y,
		gint width,
		gint height)
{
	guint           n_channels = gdk_pixbuf_get_n_channels (src);
	guint           src_rowstride = gdk_pixbuf_get_rowstride (src);
	guchar         *pixels = (gdk_pixbuf_get_pixels (src) + src_y * src_rowstride + src_x * n_channels);
	guchar         *dest_pixels;
	GdkPixbuf      *result;
	guint           dest_rowstride;
	int             i, j;

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
				 width, height);
	dest_rowstride = gdk_pixbuf_get_rowstride (result);
	dest_pixels = gdk_pixbuf_get_pixels (result);

	for (i = 0; i < height; i++) {
		guchar         *p = dest_pixels + dest_rowstride * i;
		guchar         *q = pixels + src_rowstride * i;

		guchar          r = *(q++);
		guchar          g = *(q++);
		guchar          b = *(q++);
		guchar          a = 0;

		if (n_channels == 4)
			a = *(q++);

		for (j = 0; j < width; j++) {
			*(p++) = r;
			*(p++) = g;
			*(p++) = b;

			if (n_channels == 4)
				*(p++) = a;
		}
	}

	return result;
}

/*
 * Scale the rectangle (src_x, src_y, src_width, src_height) onto the
 * rectangle (dest_x, dest_y, dest_width, dest_height) of the destination,
 * clip by clip_rect and render into a pixbuf
 */

static void
internal_constraint_render (GdkPixbuf * src,
			    guint hints,
			    GdkPixbuf * scaled,
			    GdkRectangle * clip_rect,
			    gint src_x,
			    gint src_y,
			    gint src_width,
			    gint src_height,
			    gint dest_x,
			    gint dest_y,
			    gint dest_width,
			    gint dest_height)
{
	GdkPixbuf      *tmp_pixbuf;
	GdkRectangle    rect;
	int             x_offset, y_offset;
	gboolean        has_alpha = gdk_pixbuf_get_has_alpha (src);
	gint            src_rowstride = gdk_pixbuf_get_rowstride (src);
	gint            src_n_channels = gdk_pixbuf_get_n_channels (src);

	if (dest_width <= 0 || dest_height <= 0)
		return;

	rect.x = dest_x;
	rect.y = dest_y;
	rect.width = dest_width;
	rect.height = dest_height;

	if (hints & CONSTRAINT_MISSING)
		return;

	if (dest_width == src_width && dest_height == src_height) {
		tmp_pixbuf = g_object_ref (src);

		x_offset = src_x + rect.x - dest_x;
		y_offset = src_y + rect.y - dest_y;
	} else if (src_width == 0 && src_height == 0) {
		tmp_pixbuf = bilinear_gradient (src, src_x, src_y, dest_width, dest_height);

		x_offset = rect.x - dest_x;
		y_offset = rect.y - dest_y;
	} else if (src_width == 0 && dest_height == src_height) {
		tmp_pixbuf = horizontal_gradient (src, src_x, src_y, dest_width, dest_height);

		x_offset = rect.x - dest_x;
		y_offset = rect.y - dest_y;
	} else if (src_height == 0 && dest_width == src_width) {
		tmp_pixbuf = vertical_gradient (src, src_x, src_y, dest_width, dest_height);

		x_offset = rect.x - dest_x;
		y_offset = rect.y - dest_y;
	} else if ((hints & CONSTRAINT_CONSTANT_COLS) && (hints & CONSTRAINT_CONSTANT_ROWS)) {
		tmp_pixbuf = replicate_single (src, src_x, src_y, dest_width, dest_height);

		x_offset = rect.x - dest_x;
		y_offset = rect.y - dest_y;
	} else if (dest_width == src_width && (hints & CONSTRAINT_CONSTANT_COLS)) {
		tmp_pixbuf = replicate_rows (src, src_x, src_y, dest_width, dest_height);

		x_offset = rect.x - dest_x;
		y_offset = rect.y - dest_y;
	} else if (dest_height == src_height && (hints & CONSTRAINT_CONSTANT_ROWS)) {
		tmp_pixbuf = replicate_cols (src, src_x, src_y, dest_width, dest_height);

		x_offset = rect.x - dest_x;
		y_offset = rect.y - dest_y;
	} else {
		double          x_scale = (double) dest_width / src_width;
		double          y_scale = (double) dest_height / src_height;
		guchar         *pixels;
		GdkPixbuf      *partial_src;

		pixels = (gdk_pixbuf_get_pixels (src)
			  + src_y * src_rowstride
			  + src_x * src_n_channels);

		partial_src = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
							has_alpha,
						   8, src_width, src_height,
							src_rowstride,
							NULL, NULL);

		tmp_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
					     has_alpha, 8,
					     rect.width, rect.height);

		gdk_pixbuf_scale (partial_src, tmp_pixbuf,
				  0, 0, rect.width, rect.height,
				  dest_x - rect.x, dest_y - rect.y,
				  x_scale, y_scale,
				  GDK_INTERP_BILINEAR);

		g_object_unref (partial_src);

		x_offset = 0;
		y_offset = 0;
	}

	if (rect.x >= 0 && rect.x + rect.width <= gdk_pixbuf_get_width (scaled) &&
			rect.y >= 0 && rect.y + rect.height <= gdk_pixbuf_get_height (scaled)) {
		gdk_pixbuf_copy_area (tmp_pixbuf,
				      x_offset, y_offset,
				      rect.width, rect.height,
				      scaled,
				      rect.x,
				      rect.y);
	}
	g_object_unref (tmp_pixbuf);
}

/**
 * gnome_tsol_constraint_image_render:
 * @cimage: A #ConstraintImage to render on @window
 * @window: A #GdkWindow on which the cimage is to be rendered
 * @clip_rect: A #GdkRectangle which defines the clipping area on
 *	       which the @cimage will be rendered
 * @center: A #gboolean which defines wether or not the @cimage will
 *	    be centered on not.
 * @x: the x coordinates representing where the @cimage will be renderer
 *     on @window
 * @y: the y coordinates representing where the @cimage will be renderer
 *     on @window
 * @width: the width at which the @cimage will be rendered on @window
 * @height:  the height at which the @cimage will be rendered on @window
 *
 * usually @clip_rect and x,y, width, height are usually of the same
 * value.
 */

void
gnome_tsol_constraint_image_render (cairo_t *cr,
				    ConstraintImage * cimage,
				    GdkWindow * window,
				    GdkRectangle * clip_rect,
				    gboolean center,
				    gint x,
				    gint y,
				    gint width,
				    gint height)
{
	GdkPixbuf      *pixbuf = cimage->pixbuf;
	gint            src_x[4], src_y[4], dest_x[4], dest_y[4];
	gint            pixbuf_width = gdk_pixbuf_get_width (pixbuf);
	gint            pixbuf_height = gdk_pixbuf_get_height (pixbuf);

	GtkStyleContext *context;
	GtkStateFlags	state;
	GdkRGBA		rgba;

	/*
	 * @component_mask: A #ConstraintComponent which defines which part
	 * of the cimage to render (usually COMPONENT_ALL is used).
	 */
	/* assuming COMPONENT_ALL is always wanted */
	guint           component_mask = COMPONENT_ALL;

	if (!pixbuf)
		return;

	if (clip_rect) {
		/* clip */
		gdk_cairo_rectangle(cr, clip_rect);
		cairo_clip(cr);
	}
	if (cimage->stretch) {
		if (cimage->scaled &&
			 (gdk_pixbuf_get_width (cimage->scaled) == width) &&
		       (gdk_pixbuf_get_height (cimage->scaled) == height)) {
			gdk_cairo_set_source_pixbuf(cr, cimage->scaled, x, y);
			cairo_paint(cr);
		} else {
			if (cimage->scaled)
				g_object_unref (cimage->scaled);

			cimage->scaled = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (cimage->pixbuf),
				  gdk_pixbuf_get_has_alpha (cimage->pixbuf),
			    gdk_pixbuf_get_bits_per_sample (cimage->pixbuf),
							 width,
							 height);

			src_x[0] = 0;
			src_x[1] = cimage->border_left;
			src_x[2] = pixbuf_width - cimage->border_right;
			src_x[3] = pixbuf_width;

			src_y[0] = 0;
			src_y[1] = cimage->border_top;
			src_y[2] = pixbuf_height - cimage->border_bottom;
			src_y[3] = pixbuf_height;

			dest_x[0] = 0;
			dest_x[1] = cimage->border_left;
			dest_x[2] = width - cimage->border_right;
			dest_x[3] = width;

			dest_y[0] = 0;
			dest_y[1] = cimage->border_top;
			dest_y[2] = height - cimage->border_bottom;
			dest_y[3] = height;


			if (component_mask & COMPONENT_ALL)
				component_mask = (COMPONENT_ALL - 1) & ~component_mask;

#define RENDER_COMPONENT(X1,X2,Y1,Y2)				  \
  internal_constraint_render (pixbuf, cimage->hints[Y1][X1],	  \
			      cimage->scaled, clip_rect,	  \
			      src_x[X1], src_y[Y1],		  \
			      src_x[X2] - src_x[X1], src_y[Y2] - src_y[Y1], \
			      dest_x[X1], dest_y[Y1],	     	  \
			      dest_x[X2] - dest_x[X1], dest_y[Y2] - dest_y[Y1]);

			if (component_mask & COMPONENT_NORTH_WEST)
				RENDER_COMPONENT (0, 1, 0, 1);

			if (component_mask & COMPONENT_NORTH)
				RENDER_COMPONENT (1, 2, 0, 1);

			if (component_mask & COMPONENT_NORTH_EAST)
				RENDER_COMPONENT (2, 3, 0, 1);

			if (component_mask & COMPONENT_WEST)
				RENDER_COMPONENT (0, 1, 1, 2);

			if (component_mask & COMPONENT_CENTER)
				RENDER_COMPONENT (1, 2, 1, 2);

			if (component_mask & COMPONENT_EAST)
				RENDER_COMPONENT (2, 3, 1, 2);

			if (component_mask & COMPONENT_SOUTH_WEST)
				RENDER_COMPONENT (0, 1, 2, 3);

			if (component_mask & COMPONENT_SOUTH)
				RENDER_COMPONENT (1, 2, 2, 3);

			if (component_mask & COMPONENT_SOUTH_EAST)
				RENDER_COMPONENT (2, 3, 2, 3);

			gdk_cairo_set_source_pixbuf(cr, cimage->scaled, x, y);
			cairo_paint(cr);
		}


	} else {
		if (center) {
			x += (width - pixbuf_width) / 2;
			y += (height - pixbuf_height) / 2;

			gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
			cairo_paint(cr);
		} else {
			gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
			cairo_pattern_set_extend(cairo_get_source(cr),CAIRO_EXTEND_REPEAT);
			if (clip_rect)
				cairo_rectangle(cr, clip_rect->x, clip_rect->y,
					clip_rect->width, clip_rect->height);
			else
				cairo_rectangle(cr, x, y, width, height);
			cairo_fill(cr);
		}
	}
}


static          guint
compute_hint (GdkPixbuf * pixbuf,
	      gint x0,
	      gint x1,
	      gint y0,
	      gint y1)
{
	int             i, j;
	int             hints = CONSTRAINT_CONSTANT_ROWS | CONSTRAINT_CONSTANT_COLS | CONSTRAINT_MISSING;
	int             n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	guchar         *data = gdk_pixbuf_get_pixels (pixbuf);
	int             rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	if (x0 == x1 || y0 == y1)
		return 0;

	for (i = y0; i < y1; i++) {
		guchar         *p = data + i * rowstride + x0 * n_channels;
		guchar          r = p[0];
		guchar          g = p[1];
		guchar          b = p[2];
		guchar          a = 0;

		if (n_channels == 4)
			a = p[3];

		for (j = x0; j < x1; j++) {
			if (n_channels != 4 || p[3] != 0) {
				hints &= ~CONSTRAINT_MISSING;
				if (!(hints & CONSTRAINT_CONSTANT_ROWS))
					goto cols;
			}
			if (r != *(p++) ||
					g != *(p++) ||
					b != *(p++) ||
					(n_channels != 4 && a != *(p++))) {
				hints &= ~CONSTRAINT_CONSTANT_ROWS;
				if (!(hints & CONSTRAINT_MISSING))
					goto cols;
			}
		}
	}

cols:
	for (i = y0 + 1; i < y1; i++) {
		guchar         *base = data + y0 * rowstride + x0 * n_channels;
		guchar         *p = data + i * rowstride + x0 * n_channels;

		if (memcmp (p, base, n_channels * (x1 - x0)) != 0) {
			hints &= ~CONSTRAINT_CONSTANT_COLS;
			return hints;
		}
	}

	return hints;
}

static void
constraint_compute_hints (ConstraintImage * pb)
{
	int             i, j;
	gint            width = gdk_pixbuf_get_width (pb->pixbuf);
	gint            height = gdk_pixbuf_get_height (pb->pixbuf);

	if (pb->border_left + pb->border_right > width ||
			pb->border_top + pb->border_bottom > height) {
		g_warning ("Invalid borders specified for theme constraint:\n"
			   "        %s,\n"
			"borders don't fit within the image", pb->filename);
		if (pb->border_left + pb->border_right > width) {
			pb->border_left = width / 2;
			pb->border_right = (width + 1) / 2;
		}
		if (pb->border_bottom + pb->border_top > height) {
			pb->border_top = height / 2;
			pb->border_bottom = (height + 1) / 2;
		}
	}
	for (i = 0; i < 3; i++) {
		gint            y0, y1;

		switch (i) {
		case 0:
			y0 = 0;
			y1 = pb->border_top;
			break;
		case 1:
			y0 = pb->border_top;
			y1 = height - pb->border_bottom;
			break;
		default:
			y0 = height - pb->border_bottom;
			y1 = height;
			break;
		}

		for (j = 0; j < 3; j++) {
			gint            x0, x1;

			switch (j) {
			case 0:
				x0 = 0;
				x1 = pb->border_left;
				break;
			case 1:
				x0 = pb->border_left;
				x1 = width - pb->border_right;
				break;
			default:
				x0 = width - pb->border_right;
				x1 = width;
				break;
			}

			pb->hints[i][j] = compute_hint (pb->pixbuf, x0, x1, y0, y1);
		}
	}

}

/**
 * gnome_tsol_constraint_image_set_border:
 * @pb: a #ConstraintImage
 * @left: a int in pixel
 * @right: a int in pixel
 * @top: a int in pixel
 * @bottom: a int in pixel
 *
 * constraint_set_border defines the outer rectangle in the ConstraintImage
 * @pb which will be reserved from scaling and/or stretching
 * when rendered with constraint_render
 */

void
gnome_tsol_constraint_image_set_border (ConstraintImage * pb,
					gint left,
					gint right,
					gint top,
					gint bottom)
{
	pb->border_left = left;
	pb->border_right = right;
	pb->border_top = top;
	pb->border_bottom = bottom;

	if (pb->pixbuf)
		constraint_compute_hints (pb);
}

/**
 * gnome_tsol_constraint_image_set_stretch:
 * @pb: a #ConstraintImage
 * @stretch: a boolean
 *
 * defines if the #ConstraintImage @pb will be
 * stretched or not. If it set to FALSE, the @pb will be tiled
 * when rendered with constraint_render
 */

void
gnome_tsol_constraint_image_set_stretch (ConstraintImage * pb,
					 gboolean stretch)
{
	pb->stretch = stretch;

	if (pb->pixbuf)
		constraint_compute_hints (pb);
}

static          gint
hls_value (gdouble n1,
	   gdouble n2,
	   gdouble hue)
{
	gdouble         value;

	if (hue > 255)
		hue -= 255;
	else if (hue < 0)
		hue += 255;
	if (hue < 42.5)
		value = n1 + (n2 - n1) * (hue / 42.5);
	else if (hue < 127.5)
		value = n2;
	else if (hue < 170)
		value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
	else
		value = n1;

	return (gint) (value * 255);
}

static void
hls_to_rgb (gint * hue,
	    gint * lightness,
	    gint * saturation)
{
	gdouble         h, l, s;
	gdouble         m1, m2;

	h = *hue;
	l = *lightness;
	s = *saturation;

	if (s == 0) {
		/* achromatic case  */
		*hue = l;
		*lightness = l;
		*saturation = l;
	} else {
		if (l < 128)
			m2 = (l * (255 + s)) / 65025.0;
		else
			m2 = (l + s - (l * s) / 255.0) / 255.0;

		m1 = (l / 127.5) - m2;

		/* chromatic case  */
		*hue = hls_value (m1, m2, h + 85);
		*lightness = hls_value (m1, m2, h);
		*saturation = hls_value (m1, m2, h - 85);
	}
}

static void
rgb_to_hls (gint * red,
	    gint * green,
	    gint * blue)
{
	gint            r, g, b;
	gdouble         h, l, s;
	gint            min, max;
	gint            delta;

	r = *red;
	g = *green;
	b = *blue;

	if (r > g) {
		max = MAX (r, b);
		min = MIN (g, b);
	} else {
		max = MAX (g, b);
		min = MIN (r, b);
	}

	l = (max + min) / 2.0;

	if (max == min) {
		s = 0.0;
		h = 0.0;
	} else {
		delta = (max - min);

		if (l < 128)
			s = 255 * (gdouble) delta / (gdouble) (max + min);
		else
			s = 255 * (gdouble) delta / (gdouble) (511 - max - min);

		if (r == max)
			h = (g - b) / (gdouble) delta;
		else if (g == max)
			h = 2 + (b - r) / (gdouble) delta;
		else
			h = 4 + (r - g) / (gdouble) delta;

		h = h * 42.5;

		if (h < 0)
			h += 255;
		else if (h > 255)
			h -= 255;
	}

	*red = h;
	*green = l;
	*blue = s;
}

#define CLAMP_UCHAR(v) ((guchar) (CLAMP (((int)v), (int)0, (int)255)))
#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)
static GdkPixbuf *
colorize_pixbuf (GdkPixbuf * orig,
		 GdkRGBA * new_color)
{
	GdkPixbuf      *pixbuf;
	double          intensity;
	int             x, y;
	const guchar   *src;
	guchar         *dest;
	int             orig_rowstride;
	int             dest_rowstride;
	int             width, height;
	gboolean        has_alpha;
	const guchar   *src_pixels;
	guchar         *dest_pixels;

	/*
	 * Convert GdkRGBA to GdkColor
	 */
	new_color->red *= 255;
	new_color->green *= 255;
	new_color->blue *= 255;

	pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (orig), gdk_pixbuf_get_has_alpha (orig),
				 gdk_pixbuf_get_bits_per_sample (orig),
		 gdk_pixbuf_get_width (orig), gdk_pixbuf_get_height (orig));

	if (pixbuf == NULL)
		return NULL;

	orig_rowstride = gdk_pixbuf_get_rowstride (orig);
	dest_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	has_alpha = gdk_pixbuf_get_has_alpha (orig);
	src_pixels = gdk_pixbuf_get_pixels (orig);
	dest_pixels = gdk_pixbuf_get_pixels (pixbuf);

	for (y = 0; y < height; y++) {
		src = src_pixels + y * orig_rowstride;
		dest = dest_pixels + y * dest_rowstride;

		for (x = 0; x < width; x++) {
			double          dr, dg, db;

			intensity = INTENSITY (src[0], src[1], src[2]) / 255.0;

			if (intensity <= 0.5) {
				/*
				 * Go from black at intensity = 0.0 to
				 * new_color at intensity = 0.5
				 */
				dr = (new_color->red * intensity * 2.0);
				dg = (new_color->green * intensity * 2.0);
				db = (new_color->blue * intensity * 2.0);
			} else {
				/*
				 * Go from new_color at intensity = 0.5 to
				 * white at intensity = 1.0
				 */
				dr = (new_color->red + (255 - new_color->red) * (intensity - 0.5) * 2.0);
				dg = (new_color->green + (255 - new_color->green) * (intensity - 0.5) * 2.0);
				db = (new_color->blue + (255 - new_color->blue) * (intensity - 0.5) * 2.0);
			}

			dest[0] = CLAMP_UCHAR (dr);
			dest[1] = CLAMP_UCHAR (dg);
			dest[2] = CLAMP_UCHAR (db);

			if (has_alpha) {
				dest[3] = src[3];
				src += 4;
				dest += 4;
			} else {
				src += 3;
				dest += 3;
			}
		}
	}

	return pixbuf;
}

/**
 * gnome_tsol_constraint_image_colorize:
 * @image : a #ConstraintImage to be colorized
 * @color: the #GdkRGBA to be applied to the @image
 * @alpha: the transparency (from 0 (fully transparent to 255 fully opaque)
 * @use_alpha: a #gboolean indicating wether to use transparency
 *
 * Note this is a destructive process the original pixbuf of the ConstraintImage
 * is modified.
 */

void
gnome_tsol_constraint_image_colorize (ConstraintImage * image,
				      GdkRGBA * color,
				      int alpha,
				      gboolean use_alpha)
{
	GdkPixbuf      *new_pb;


	new_pb = colorize_pixbuf (image->pixbuf, color);

	g_object_unref (image->pixbuf);

	image->pixbuf = new_pb;

	/*
	 * guchar *pixels; gint r, g, b, a, hue, lum, sat; gint r_src, g_src,
	 * b_src, a_src; guchar *p; gint w, h; int width =
	 * gdk_pixbuf_get_width (image->pixbuf); int height =
	 * gdk_pixbuf_get_height (image->pixbuf); int n_channels =
	 * gdk_pixbuf_get_n_channels (image->pixbuf); int rowstride =
	 * gdk_pixbuf_get_rowstride (image->pixbuf);
	 * 
	 * g_return_if_fail (GDK_IS_PIXBUF (image->pixbuf));
	 * 
	 * if (width == 0 || height == 0) return;
	 * 
	 * pixels = gdk_pixbuf_get_pixels(image->pixbuf);
	 * 
	 * r = hue = color->red; g = lum = color->green; b = sat = color->blue;
	 * a = alpha;
	 * 
	 * rgb_to_hls (&hue, &lum, &sat);
	 * 
	 * h = height;
	 * 
	 * while (h--) { w = width; p = pixels;
	 * 
	 * while (w--) { r_src = p[0];      g_src = p[1]; b_src = p[2];
	 * 
	 * rgb_to_hls (&r_src, &g_src, &b_src);
	 * 
	 * r_src = hue; b_src = sat;
	 * 
	 * hls_to_rgb (&r_src, &g_src, &b_src);
	 * 
	 * p[0] = r_src; p[1] = g_src; p[2] = b_src;
	 * 
	 * switch (n_channels) { case 3: p +=3; break; case 4: a_src = p[3];
	 * 
	 * if (use_alpha) p[3] = MIN (p[3], alpha); p += 4; break; default:
	 * break; } } pixels += rowstride; }
	 */
}

static          gint
label_string_compare (HighlightStripe * tmp, char *searched_label)
{
	return strcmp (searched_label, tmp->name);
}

ConstraintImage *
gnome_tsol_get_highlight_stripe (const char *name,
				 GdkRGBA * label_color)
{
	static GSList  *hl_stripe_list = NULL;
	GSList         *stored_hl_stripe = NULL;
	HighlightStripe *hl_stripe;

	if ((name == NULL) || (label_color == NULL))
		return NULL;


	stored_hl_stripe = g_slist_find_custom (hl_stripe_list,
						name,
				       (GCompareFunc) label_string_compare);
	if (stored_hl_stripe)
		return ((HighlightStripe *) stored_hl_stripe->data)->image;

	hl_stripe = g_new0 (HighlightStripe, 1);

	hl_stripe->name = g_strdup (name);

	hl_stripe->image = g_new0 (ConstraintImage, 1);

	hl_stripe->image->pixbuf = gdk_pixbuf_new_from_inline (-1,
							highlight_stripe_pb,
							       TRUE, NULL);

	gnome_tsol_constraint_image_set_border (hl_stripe->image, 9, 9, 0, 0);
	gnome_tsol_constraint_image_set_stretch (hl_stripe->image, TRUE);
	gnome_tsol_constraint_image_colorize (hl_stripe->image, label_color, 255, TRUE);

	hl_stripe_list = g_slist_append (hl_stripe_list, hl_stripe);
	return hl_stripe->image;
}

gboolean
gnome_tsol_label_bg_should_be_black (GdkRGBA * color)
{
	double  ntsc;

	ntsc = ((color->red) * .4450 +
		(color->blue) * .030 +
		(color->green) * .525);

	if ((1.0 - ntsc) < .61)
		return TRUE;
	return FALSE;
}

void
gnome_tsol_render_coloured_label (cairo_t *cr, GtkWidget * label)
{
	//GdkGC          *gc;
	int             text_height, pango_height, pango_width;
	GdkRectangle    area;
	PangoLayout    *pango_layout;
	//GdkRGBA        color;
	const char     *text;
	char           *color_str;
	int             error;
	m_label_t      *mlabel = NULL;

	GtkStateFlags	state;
	GdkRGBA		bg_rgba;
	GdkRGBA		fg_rgba;

	gtk_widget_get_allocation(label, &area);
	text = gtk_label_get_text (GTK_LABEL (label));

	str_to_label (text, &mlabel, MAC_LABEL, L_NO_CORRECTION, &error);
	label_to_str (mlabel, &color_str, M_COLOR, DEF_NAMES);
	if (color_str == NULL)
		color_str = g_strdup ("white");
	gdk_rgba_parse(&bg_rgba, color_str);
	g_free (color_str);
	blabel_free (mlabel);

	cairo_set_source_rgb(cr, bg_rgba.red, bg_rgba.green, bg_rgba.blue);
	cairo_rectangle(cr, area.x, area.y, area.width, area.height);
	cairo_fill(cr);

	pango_layout = gtk_widget_create_pango_layout (label, text);
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

/*
	if (gnome_tsol_label_bg_should_be_black (&color)) {
		gc = label->style->black_gc;
	} else {
		gc = label->style->white_gc;
	}

	gtk_widget_modify_bg (label->parent, GTK_STATE_NORMAL, &color);

	gdk_draw_layout (label->window, gc,
			 area.x,
			 area.y + (text_height / 2),
			 pango_layout);

 */
	/* set the correct source color */
	if (gnome_tsol_label_bg_should_be_black (&bg_rgba)) {
		gdk_rgba_parse(&fg_rgba, "black");
	} else {
		gdk_rgba_parse(&fg_rgba, "white");
	}

	/* draw the label */
	cairo_move_to(cr, area.x, area.y + (text_height / 2));
	cairo_set_source_rgb(cr, fg_rgba.red, fg_rgba.green, fg_rgba.blue);
	pango_cairo_show_layout(cr, pango_layout);

	g_object_unref (pango_layout);

}

void
gnome_tsol_render_coloured_label_for_zone (GtkWidget * label, const char *zonename)
{
	//GdkGC          *gc;
	int             text_height, pango_height, pango_width;
	GdkRectangle    area;
	PangoLayout    *pango_layout;
	const char     *text;
	char           *color_str = NULL;
	int             error;
	m_label_t      *mlabel = NULL;

	GtkStyleContext	*context;
	GtkStateFlags	state;
	GdkRGBA		rgba;
	cairo_t		*cr;

	gtk_widget_get_allocation(label, &area);
	text = gtk_label_get_text (GTK_LABEL (label));

	mlabel = getzonelabelbyname (zonename);
	label_to_str (mlabel, &color_str, M_COLOR, DEF_NAMES);
	if (color_str == NULL)
		color_str = g_strdup ("white");
	gdk_rgba_parse(&rgba, color_str);
	g_free (color_str);

	pango_layout = gtk_widget_create_pango_layout (label, text);
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	cr = gdk_cairo_create(gtk_widget_get_window(label));

	/* set the correct source color */
	context= gtk_widget_get_style_context(label);
	state = gtk_widget_get_state_flags(label);
	gtk_style_context_get_color(context, state, &rgba);
	if (gnome_tsol_label_bg_should_be_black (&rgba)) {
		gdk_rgba_parse(&rgba, "black");
	} else {
		gdk_rgba_parse(&rgba, "white");
	}
	cairo_set_source_rgb(cr, rgba.red, rgba.green, rgba.blue);

	/* draw the label */
	cairo_move_to(cr, area.x, area.y + (text_height / 2));
	pango_cairo_show_layout(cr, pango_layout);
	cairo_destroy(cr);

	g_object_unref (pango_layout);

}
