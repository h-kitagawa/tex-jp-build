/* This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002-2021 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team.
    
    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "system.h"
#include "error.h"
#include "mem.h"

#include "dpxconf.h"
#include "dpxfile.h"
#include "dpxutil.h"

#include "pdfobj.h"

#include "pdfdoc.h"
#include "pdfdev.h"
#include "pdfdraw.h"
#include "pdfnames.h"

#include "epdf.h"
#include "mpost.h"
#include "pngimage.h"
#include "jpegimage.h"
#include "jp2image.h"
#include "bmpimage.h"

#include "pdfximage.h"

static int  check_for_ps    (FILE *image_file);
static int  check_for_mp    (FILE *image_file);
static int  ps_include_page (pdf_ximage *ximage,
                             const char *ident, load_options options);


#define IMAGE_TYPE_UNKNOWN -1
#define IMAGE_TYPE_PDF      0
#define IMAGE_TYPE_JPEG     1
#define IMAGE_TYPE_PNG      2
#define IMAGE_TYPE_MPS      4
#define IMAGE_TYPE_EPS      5
#define IMAGE_TYPE_BMP      6
#define IMAGE_TYPE_JP2      7

#if defined(_WIN32)
extern int fsyscp_stat(const char *path, struct stat *buffer);
#endif /* _WIN32 */

struct attr_
{
  int      width, height;
  double   xdensity, ydensity;
  pdf_rect bbox;

  /* Not appropriate place but... someone need them. */
  int      page_no;
  int      page_count;
  int      bbox_type;  /* Ugh */
  pdf_obj *dict;
  char     tempfile;
  char    *page_name;
};

struct pdf_ximage_
{
  char        *ident;
  char         res_name[16];

  int          subtype;

  struct attr_ attr;

  char        *filename;
  char        *fullname;
  pdf_obj     *reference;
  pdf_obj     *resource;

  int          reserved;
};


/* verbose, verbose, verbose... */
struct opt_
{
  char  *cmdtmpl;
};

static struct opt_ _opts = {
  NULL
};

struct ic_
{
  int         count, capacity;
  pdf_ximage *ximages;
};

static struct ic_  _ic = {
  0, 0, NULL
};

static void
pdf_init_ximage_struct (pdf_ximage *I)
{
  I->ident    = NULL;
  I->filename = NULL;
  I->fullname = NULL;

  I->subtype  = -1;
  memset(I->res_name, 0, 16);
  I->reference = NULL;
  I->resource  = NULL;
  I->reserved  = 0;

  I->attr.width = I->attr.height = 0;
  I->attr.xdensity = I->attr.ydensity = 1.0;
  I->attr.bbox.llx = I->attr.bbox.lly = 0;
  I->attr.bbox.urx = I->attr.bbox.ury = 0;

  I->attr.page_no    = 1;
  I->attr.page_count = 1;
  I->attr.bbox_type  = 0;

  I->attr.dict     = NULL;
  I->attr.tempfile = 0;

  I->attr.page_name = NULL;
}

static void
pdf_set_ximage_tempfile (pdf_ximage *I, const char *fullname)
{
  if (I->fullname)
    RELEASE(I->fullname);
  I->fullname = NEW(strlen(fullname)+1, char);
  strcpy(I->fullname, fullname);
  I->attr.tempfile = 1;
}

static void
pdf_clean_ximage_struct (pdf_ximage *I)
{
  if (I->ident)
    RELEASE(I->ident);
  if (I->filename)
    RELEASE(I->filename);
  if (I->fullname)
    RELEASE(I->fullname);
  if (I->reference)
    pdf_release_obj(I->reference);
  if (I->resource)
    pdf_release_obj(I->resource);
  if (I->attr.dict) /* unsafe? */
    pdf_release_obj(I->attr.dict);
  pdf_init_ximage_struct(I);
}


void
pdf_init_images (void)
{
  struct ic_ *ic = &_ic;
  ic->count    = 0;
  ic->capacity = 0;
  ic->ximages  = NULL;
}

void
pdf_close_images (void)
{
  struct ic_ *ic = &_ic;
  if (ic->ximages) {
    int  i;
    for (i = 0; i < ic->count; i++) {
      pdf_ximage *I = ic->ximages+i;
      if (I->attr.tempfile) {
        /*
         * It is important to remove temporary files at the end because
         * we cache file names. Since we use mkstemp to create them, we
         * might get the same file name again if we delete the first file.
         * (This happens on NetBSD, reported by Jukka Salmi.)
         * We also use this to convert a PS file only once if multiple
         * pages are imported from that file.
         */
        if (dpx_conf.verbose_level > 1 &&
            dpx_conf.file.keep_cache != 1)
          MESG("pdf_image>> deleting temporary file \"%s\"\n", I->filename);
        dpx_delete_temp_file(I->fullname, false); /* temporary filename freed here */
        I->fullname = NULL;
      }
      pdf_clean_ximage_struct(I);
    }
    RELEASE(ic->ximages);
    ic->ximages = NULL;
    ic->count = ic->capacity = 0;
  }

  if (_opts.cmdtmpl)
    RELEASE(_opts.cmdtmpl);
  _opts.cmdtmpl = NULL;
}

static int
source_image_type (FILE *fp)
{
  int  format = IMAGE_TYPE_UNKNOWN;

  rewind(fp);
  /*
   * Make sure we check for PS *after* checking for MP since
   * MP is a special case of PS.
   */
  if (check_for_jpeg(fp))
  {
    format = IMAGE_TYPE_JPEG;
  }
  else if (check_for_jp2(fp))
  {
    format = IMAGE_TYPE_JP2;
  }
#ifdef  HAVE_LIBPNG
  else if (check_for_png(fp))
  {
    format = IMAGE_TYPE_PNG;
  }
#endif
  else if (check_for_bmp(fp))
  {
    format = IMAGE_TYPE_BMP;
  } else if (check_for_pdf(fp)) {
    format = IMAGE_TYPE_PDF;
  } else if (check_for_mp(fp)) {
    format = IMAGE_TYPE_MPS;
  } else if (check_for_ps(fp)) {
    format = IMAGE_TYPE_EPS;
  } else {
    format = IMAGE_TYPE_UNKNOWN;
  }
  rewind(fp);

  return  format;
}

static int
load_image (const char *ident, const char *filename, const char *fullname, int format, FILE  *fp,
            load_options options)
{
  struct ic_ *ic = &_ic;
  int         id = -1, reserved = 0;
  pdf_ximage *I;

  if (ident) {
    for (id = 0; id < ic->count; id++) {
      I = &ic->ximages[id];
      if (I->ident && !strcmp(ident, I->ident)) {
        if (I->reserved) {
          reserved = 1;
          break;
        }
      }
    }
  }
  if (!reserved) {
    id = ic->count;
    if (ic->count >= ic->capacity) {
      ic->capacity += 16;
      ic->ximages   = RENEW(ic->ximages, ic->capacity, pdf_ximage);
    }
    I  = &ic->ximages[id];
    pdf_init_ximage_struct(I);
    if (ident) {
      I->ident = NEW(strlen(ident)+1, char);
      strcpy(I->ident, ident);
    }
    ic->count++;
  }
  if (filename) {
    I->filename = NEW(strlen(filename)+1, char);
    strcpy(I->filename, filename);
  }
  if (fullname) {
    I->fullname = NEW(strlen(fullname)+1, char);
    strcpy(I->fullname, fullname);
  }

  I->attr.page_no   = options.page_no;
  I->attr.page_name = options.page_name;
  I->attr.bbox_type = options.bbox_type;
  I->attr.dict      = options.dict; /* unsafe? */

  switch (format) {
  case  IMAGE_TYPE_JPEG:
    if (dpx_conf.verbose_level > 0)
      MESG("[JPEG]");
    if (jpeg_include_image(I, fp) < 0)
      goto error;
    I->subtype  = PDF_XOBJECT_TYPE_IMAGE;
    break;
  case  IMAGE_TYPE_JP2:
    if (dpx_conf.verbose_level > 0)
      MESG("[JP2]");
    if (jp2_include_image(I, fp) < 0)
      goto error;
    I->subtype  = PDF_XOBJECT_TYPE_IMAGE;
    break;
#ifdef HAVE_LIBPNG
  case  IMAGE_TYPE_PNG:
    if (dpx_conf.verbose_level > 0)
      MESG("[PNG]");
    if (png_include_image(I, fp) < 0)
      goto error;
    I->subtype  = PDF_XOBJECT_TYPE_IMAGE;
    break;
#endif
  case  IMAGE_TYPE_BMP:
    if (dpx_conf.verbose_level > 0)
      MESG("[BMP]");
    if (bmp_include_image(I, fp) < 0)
      goto error;
    I->subtype  = PDF_XOBJECT_TYPE_IMAGE;
    break;
  case  IMAGE_TYPE_PDF:
    if (dpx_conf.verbose_level > 0)
      MESG("[PDF]");
    {
      int result = pdf_include_page(I, fp, fullname, options);
      if (result > 0)
        /* PDF version too recent */
        result = ps_include_page(I, fullname, options);
      if (result < 0)
        goto error;
    }
    if (dpx_conf.verbose_level > 0)
      MESG(",Page:%ld", I->attr.page_no);
    I->subtype  = PDF_XOBJECT_TYPE_FORM;
    break;
/*
 * case  IMAGE_TYPE_EPS:
 */
  default:
    if (dpx_conf.verbose_level > 0)
      MESG(format == IMAGE_TYPE_EPS ? "[PS]" : "[UNKNOWN]");
    if (ps_include_page(I, fullname, options) < 0)
      goto error;
    if (dpx_conf.verbose_level > 0)
      MESG(",Page:%ld", I->attr.page_no);
    I->subtype  = PDF_XOBJECT_TYPE_FORM;
  }

  switch (I->subtype) {
  case PDF_XOBJECT_TYPE_IMAGE:
    sprintf(I->res_name, "Im%d", id);
    break;
  case PDF_XOBJECT_TYPE_FORM:
    sprintf(I->res_name, "Fm%d", id);
    break;
  default:
    ERROR("Unknown XObject subtype: %d", I->subtype);
    goto error;
  }

  return  id;

 error:
  pdf_clean_ximage_struct(I);
  return -1;
}


#define dpx_find_file(n,d,s) (kpse_find_pict((n)))
#define dpx_fopen(n,m) (MFOPEN((n),(m)))
#define dpx_fclose(f)  (MFCLOSE((f)))

#if defined(WIN32)
int utf8name_failed = 0;
#endif /* WIN32 */

int
pdf_ximage_load_image (const char *ident, const char *filename, load_options options)
{
  struct ic_ *ic = &_ic;
  int         i, id = -1;
  pdf_ximage *I;
  char       *fullname, *f = NULL;
  int         format;
  FILE       *fp;

  for (i = 0; i < ic->count; i++) {
    I = &ic->ximages[i];
    if (I->filename && !strcmp(filename, I->filename)) {
      id = i;
      break;
    }
  }
  if (id >= 0) {
    if (I->attr.page_no == options.page_no &&
        strcmp(I->attr.page_name, options.page_name) == 0 &&
        !pdf_compare_object(I->attr.dict, options.dict) && /* ????? */
        I->attr.bbox_type == options.bbox_type) {
      return id;
    }
    f = I->fullname;
  }
  if (f) {
    /* we already have converted this file; f is the temporary file name */
    fullname = NEW(strlen(f)+1, char);
    strcpy(fullname, f);
  } else {
    /* try loading image */
#if defined(WIN32)
    utf8name_failed = 0;
#endif /* WIN32 */
    fullname = dpx_find_file(filename, "_pic_", "");
#if defined(WIN32)
    if (!fullname && file_system_codepage != win32_codepage) {
      int tmpcp = file_system_codepage;
      utf8name_failed = 1;
      file_system_codepage = win32_codepage;
      fullname = dpx_find_file(filename, "_pic_", "");
      file_system_codepage = tmpcp;
    }
#endif /* WIN32 */
    if (!fullname) {
#if defined(WIN32)
      utf8name_failed = 0;
#endif /* WIN32 */
      if (dpx_conf.compat_mode == dpx_mode_compat_mode) {
        WARN("Image inclusion failed. Could not find file: %s", filename);
      } else {
        ERROR("Image inclusion failed. Could not find file: %s", filename);
      }
      return  -1;
    }
  }

  fp = dpx_fopen(fullname, FOPEN_RBIN_MODE);
  if (!fp) {
    if (dpx_conf.compat_mode == dpx_mode_compat_mode) {
      WARN("Error opening image file \"%s\"", fullname);
      RELEASE(fullname);
    } else {
      RELEASE(fullname);
      ERROR("Error opening image file \"%s\".", filename);
    }
    return  -1;
  }
  if (dpx_conf.verbose_level > 0) {
    MESG("(Image:%s", filename);
    if (dpx_conf.verbose_level > 1)
      MESG("[%s]", fullname);
  }

  format = source_image_type(fp);
  switch (format) {
  case IMAGE_TYPE_MPS:
    if (dpx_conf.verbose_level > 0)
      MESG("[MPS]");
    id = mps_include_page(filename, fp);
    if (id < 0) {
      WARN("Try again with the distiller.");
      format = IMAGE_TYPE_EPS;
      rewind(fp);
    } else {
      /* Workaround for the problem reported.
       * mps_include_page() above doesn't set I->filename...
       */
      I = &ic->ximages[id];
      if (!I->filename) {
        I->filename = NEW(strlen(filename)+1, char);
        strcpy(I->filename, filename);
      }
      break;
    }
  default:
    id = load_image(ident, filename, fullname, format, fp, options);
    break;
  }
  dpx_fclose(fp);

  RELEASE(fullname);

  if (dpx_conf.verbose_level > 0)
    MESG(")");

  if (id < 0) {
    if (dpx_conf.compat_mode == dpx_mode_compat_mode) {
      if (format == IMAGE_TYPE_PDF || format == IMAGE_TYPE_EPS) {
        WARN("Image inclusion failed for \"%s\" (page=%d).", filename, options.page_no);
      } else {
        WARN("Image inclusion failed for \"%s\"", filename);
      }
    } else {
      if (format == IMAGE_TYPE_PDF || format == IMAGE_TYPE_EPS) {
        ERROR("Image inclusion failed for \"%s\" (page=%d).", filename, options.page_no);
      } else {
        ERROR("Image inclusion failed for \"%s\"", filename);
      }
    }
  }

  return  id;
}

int
pdf_ximage_findresource (const char *ident)
{
  struct ic_ *ic = &_ic;
  int         id = -1;
  pdf_ximage *I;

  for (id = 0; id < ic->count; id++) {
    I = &ic->ximages[id];
    if (I->ident && !strcmp(ident, I->ident)) {
      return id;
    }
  }

  return -1;
}

/* Reference: PDF Reference 1.5 v6, pp.321--322
 *
 * TABLE 4.42 Additional entries specific to a type 1 form dictionary
 *
 * BBox rectangle (Required) An array of four numbers in the form coordinate
 *                system, giving the coordinates of the left, bottom, right,
 *                and top edges, respectively, of the form XObject's bounding
 *                box. These boundaries are used to clip the form XObject and
 *                to determine its size for caching.
 *
 * Matrix array   (Optional) An array of six numbers specifying the form
 *                matrix, which maps form space into user space.
 *                Default value: the identity matrix [1 0 0 1 0 0].
 */
void
pdf_ximage_init_form_info (xform_info *info)
{
  info->flags    = 0;
  info->bbox.llx = 0;
  info->bbox.lly = 0;
  info->bbox.urx = 0;
  info->bbox.ury = 0;
  info->matrix.a = 1.0;
  info->matrix.b = 0.0;
  info->matrix.c = 0.0;
  info->matrix.d = 1.0;
  info->matrix.e = 0.0;
  info->matrix.f = 0.0;
}

/* Reference: PDF Reference 1.5 v6, pp.303--306
 *
 * TABLE 4.42 Additional entries specific to an image dictionary
 *
 * Width integer  (Required) The width of the image, in samples.
 *
 * Height integer (Required) The height of the image, in samples.
 *
 * ColorSpace name or array
 *                (Required for images, except those that use the JPXDecode
 *                filter; not allowed for image masks) The color space in
 *                which image samples are specified. This may be any type
 *                of color space except Patter.
 *
 *                If the image uses the JPXDecode filter, this entry is
 *                optional.
 *
 * BitsPerComponent integer
 *                (Required except for image masks and images that use the
 *                JPXDecode filter) The number of bits used to represent
 *                each color component. Only a single value may be specified;
 *                the number of bits is the same for all color components.
 *                Valid values are 1,2,4,8, and (in PDF1.5) 16. If ImageMask
 *                is true, this entry is optional, and if speficified, its
 *                value must be 1.
 *
 *                If the image stream uses the JPXDecode filter, this entry
 *                is optional and ignored if present. The bit depth is
 *                determined in the process of decoding the JPEG2000 image.
 */
void
pdf_ximage_init_image_info (ximage_info *info)
{
  info->flags  = 0;
  info->width  = 0;
  info->height = 0;
  info->bits_per_component = 0;
  info->num_components = 0;
  info->min_dpi = 0;
  info->xdensity = info->ydensity = 1.0;
}

void
pdf_ximage_set_image (pdf_ximage *I, void *image_info, pdf_obj *resource)
{
  pdf_obj     *dict;
  ximage_info *info = image_info;

  if (!PDF_OBJ_STREAMTYPE(resource))
    ERROR("Image XObject must be of stream type.");

  I->subtype = PDF_XOBJECT_TYPE_IMAGE;

  I->attr.width  = info->width;  /* The width of the image, in samples */
  I->attr.height = info->height; /* The height of the image, in samples */
  I->attr.xdensity = info->xdensity;
  I->attr.ydensity = info->ydensity;

  dict = pdf_stream_dict(resource);
  pdf_add_dict(dict, pdf_new_name("Type"),    pdf_new_name("XObject"));
  pdf_add_dict(dict, pdf_new_name("Subtype"), pdf_new_name("Image"));
  pdf_add_dict(dict, pdf_new_name("Width"),   pdf_new_number(info->width));
  pdf_add_dict(dict, pdf_new_name("Height"),  pdf_new_number(info->height));
  if (info->bits_per_component > 0) /* Ignored for JPXDecode filter. FIXME */
    pdf_add_dict(dict, pdf_new_name("BitsPerComponent"),
                 pdf_new_number(info->bits_per_component));
  if (I->attr.dict)
    pdf_merge_dict(dict, I->attr.dict);

  if (I->ident) {
    int error;

    error = pdf_names_add_object(global_names, I->ident, strlen(I->ident), pdf_link_obj(resource));
    if (I->reference)
      pdf_release_obj(I->reference);
    if (error) {
      I->reference = pdf_ref_obj(resource);
    } else {
      /* Need to create object reference before closing it */
      I->reference = pdf_names_lookup_reference(global_names, I->ident, strlen(I->ident));
      pdf_names_close_object(global_names, I->ident, strlen(I->ident));
    }
    I->reserved = 0;
  } else {
    I->reference = pdf_ref_obj(resource);
  }
  pdf_release_obj(resource); /* Caller don't know we are using reference. */
  I->resource  = NULL;
}

void
pdf_ximage_set_form (pdf_ximage *I, void *form_info, pdf_obj *resource)
{
  xform_info *info = form_info;
  pdf_coord   p1, p2, p3, p4;

  I->subtype   = PDF_XOBJECT_TYPE_FORM;

  /* Image's attribute "bbox" here is affected by /Rotate entry of included
   * PDF page.
   */
  p1.x = info->bbox.llx; p1.y = info->bbox.lly;
  pdf_dev_transform(&p1, &info->matrix);
  p2.x = info->bbox.urx; p1.y = info->bbox.lly;
  pdf_dev_transform(&p2, &info->matrix);
  p3.x = info->bbox.urx; p3.y = info->bbox.ury;
  pdf_dev_transform(&p3, &info->matrix);
  p4.x = info->bbox.llx; p4.y = info->bbox.ury;
  pdf_dev_transform(&p4, &info->matrix);

  I->attr.bbox.llx = min4(p1.x, p2.x, p3.x, p4.x);
  I->attr.bbox.lly = min4(p1.y, p2.y, p3.y, p4.y);
  I->attr.bbox.urx = max4(p1.x, p2.x, p3.x, p4.x);
  I->attr.bbox.ury = max4(p1.y, p2.y, p3.y, p4.y);

  if (I->ident) {
    int error;
    
    error = pdf_names_add_object(global_names, I->ident, strlen(I->ident), pdf_link_obj(resource));
    if (I->reference)
      pdf_release_obj(I->reference);
    if (error) {
      I->reference = pdf_ref_obj(resource);
    } else {
      /* Need to create object reference before closing it */
      I->reference = pdf_names_lookup_reference(global_names, I->ident, strlen(I->ident));
      pdf_names_close_object(global_names, I->ident, strlen(I->ident));
    }
    I->reserved = 0;
  } else {
    I->reference = pdf_ref_obj(resource);
  }
  pdf_release_obj(resource); /* Caller don't know we are using reference. */
  I->resource  = NULL;
}

int
pdf_ximage_get_page (pdf_ximage *I)
{
  return I->attr.page_no;
}

#define CHECK_ID(c,n) do {\
  if ((n) < 0 || (n) >= (c)->count) {\
    ERROR("Invalid XObject ID: %d", (n));\
  }\
} while (0)
#define GET_IMAGE(c,n) (&((c)->ximages[(n)]))

pdf_obj *
pdf_ximage_get_reference (int id)
{
  struct ic_ *ic = &_ic;
  pdf_ximage *I;

  CHECK_ID(ic, id);

  I = GET_IMAGE(ic, id);
  if (!I->reference && I->resource)
    I->reference = pdf_ref_obj(I->resource);

  return pdf_link_obj(I->reference);
}

/* called from pdfdoc.c only for late binding */
int
pdf_ximage_defineresource (const char *ident,
                           int subtype, void *info, pdf_obj *resource)
{
  struct ic_ *ic = &_ic;
  int         id, reserved = 0;
  pdf_ximage *I;

  if (ident) {
    for (id = 0; id < ic->count; id++) {
      I = &ic->ximages[id];
      if (I->ident && !strcmp(ident, I->ident) && I->reserved) {
        reserved = 1;
        break;
      }
    }
  }

  if (!reserved) {
    id = ic->count;
    if (ic->count >= ic->capacity) {
      ic->capacity += 16;
      ic->ximages   = RENEW(ic->ximages, ic->capacity, pdf_ximage);
    }
    I = &ic->ximages[id];
    pdf_init_ximage_struct(I);

    if (ident) {
      I->ident = NEW(strlen(ident)+1, char);
      strcpy(I->ident, ident);
    }
    ic->count++;
  }

  switch (subtype) {
  case PDF_XOBJECT_TYPE_IMAGE:
    pdf_ximage_set_image(I, info, resource);
    sprintf(I->res_name, "Im%d", id);
    break;
  case PDF_XOBJECT_TYPE_FORM:
    pdf_ximage_set_form (I, info, resource);
    sprintf(I->res_name, "Fm%d", id);
    break;
  default:
    ERROR("Unknown XObject subtype: %d", subtype);
  }

  return  id;
}

int
pdf_ximage_reserve (const char *ident)
{
   struct ic_ *ic = &_ic;
  int         id;
  pdf_ximage *I;

  for (id = 0; id < ic->count; id++) {
    I = &ic->ximages[id];
    if (I->ident && !strcmp(ident, I->ident)) {
      WARN("XObject ID \"%s\" already used!", ident);
      return -1;
    }
  }

  id = ic->count;
  if (ic->count >= ic->capacity) {
    ic->capacity += 16;
    ic->ximages   = RENEW(ic->ximages, ic->capacity, pdf_ximage);
  }

  I = &ic->ximages[id];

  pdf_init_ximage_struct(I);

  if (ident) {
    I->ident = NEW(strlen(ident)+1, char);
    strcpy(I->ident, ident);
  }
  I->reference  = pdf_names_reserve(global_names, ident, strlen(ident));
  sprintf(I->res_name, "Fm%d", id);
  I->reserved = 1;
  ic->count++;

  return id;
}

char *
pdf_ximage_get_resname (int id)
{
  struct ic_ *ic = &_ic;
  pdf_ximage *I;

  CHECK_ID(ic, id);

  I = GET_IMAGE(ic, id);

  return I->res_name;
}

int
pdf_ximage_get_subtype (int id)
{
  struct ic_ *ic = &_ic;
  pdf_ximage *I;

  CHECK_ID(ic, id);

  I = GET_IMAGE(ic, id);

  return I->subtype;
}

void
pdf_ximage_set_attr (int id, int width, int height, double xdensity, double ydensity, double llx, double lly, double urx, double ury)
{
  struct ic_ *ic = &_ic;
  pdf_ximage *I;

  CHECK_ID(ic, id);

  I = GET_IMAGE(ic, id);
  I->attr.width = width;
  I->attr.height = height;
  I->attr.xdensity = xdensity;
  I->attr.ydensity = ydensity;
  I->attr.bbox.llx = llx;
  I->attr.bbox.lly = lly;
  I->attr.bbox.urx = urx;
  I->attr.bbox.ury = ury;
}

/* depth...
 * Dvipdfm treat "depth" as "yoffset" for pdf:image and pdf:uxobj
 * not as vertical dimension of scaled image. (And there are bugs.)
 * This part contains incompatibile behaviour than dvipdfm!
 */
#define EBB_DPI 72

static void
scale_to_fit_I (pdf_tmatrix    *T,
                transform_info *p,
                pdf_ximage     *I)
{
  double  s_x, s_y, d_x, d_y;
  double  wd0, ht0, dp, xscale, yscale;

  if (p->flags & INFO_HAS_USER_BBOX) {
    wd0 =  p->bbox.urx - p->bbox.llx;
    ht0 =  p->bbox.ury - p->bbox.lly;
    xscale = I->attr.width * I->attr.xdensity / wd0;
    yscale = I->attr.height * I->attr.ydensity / ht0;
    d_x = -p->bbox.llx / wd0;
    d_y = -p->bbox.lly / ht0;
  } else {
    wd0 = I->attr.width * I->attr.xdensity;
    ht0 = I->attr.height * I->attr.ydensity;
    xscale = yscale = 1.0;
    d_x = 0.0;
    d_y = 0.0; 
  }

  if (wd0 == 0.0) {
    WARN("Image width=0.0!");
    wd0 = 1.0;
  }
  if (ht0 == 0.0) {
    WARN("Image height=0.0!");
    ht0 = 1.0;
  }

  if ( (p->flags & INFO_HAS_WIDTH ) &&
       (p->flags & INFO_HAS_HEIGHT) ) {
    s_x = p->width * xscale;
    s_y = (p->height + p->depth) * yscale;
    dp  = p->depth * yscale;
  } else if ( p->flags & INFO_HAS_WIDTH ) {
    s_x = p->width * xscale;
    s_y = s_x * ((double)I->attr.height / I->attr.width);
    dp  = 0.0;
  } else if ( p->flags & INFO_HAS_HEIGHT) {
    s_y = (p->height + p->depth) * yscale;
    s_x = s_y * ((double)I->attr.width / I->attr.height);
    dp  = p->depth * yscale;
  } else {
    s_x = wd0;
    s_y = ht0;
    dp  = 0.0;
  }
  T->a = s_x; T->c = 0.0;
  T->b = 0.0; T->d = s_y;
  T->e = d_x * s_x / xscale; T->f = d_y * s_y / yscale - dp;

  return;
}


static void
scale_to_fit_F (pdf_tmatrix    *T,
                transform_info *p,
                pdf_ximage     *I)
{
  double  s_x, s_y, d_x, d_y;
  double  wd0, ht0, dp;

  if (p->flags & INFO_HAS_USER_BBOX) {
    wd0 =  p->bbox.urx - p->bbox.llx;
    ht0 =  p->bbox.ury - p->bbox.lly;
    d_x = -p->bbox.llx;
    d_y = -p->bbox.lly;
  } else {
    wd0 = I->attr.bbox.urx - I->attr.bbox.llx;
    ht0 = I->attr.bbox.ury - I->attr.bbox.lly;
    d_x = 0.0;
    d_y = 0.0; 
  }

  if (wd0 == 0.0) {
    WARN("Image width=0.0!");
    wd0 = 1.0;
  }
  if (ht0 == 0.0) {
    WARN("Image height=0.0!");
    ht0 = 1.0;
  }

  if ( (p->flags & INFO_HAS_WIDTH ) &&
       (p->flags & INFO_HAS_HEIGHT) ) {
    s_x = p->width  / wd0;
    s_y = (p->height + p->depth) / ht0;
    dp  = p->depth;
  } else if ( p->flags & INFO_HAS_WIDTH ) {
    s_x = p->width  / wd0;
    s_y = s_x;
    dp  = 0.0;
  } else if ( p->flags & INFO_HAS_HEIGHT) {
    s_y = (p->height + p->depth) / ht0;
    s_x = s_y;
    dp  = p->depth;
  } else {
    s_x = s_y = 1.0;
    dp  = 0.0;
  }

  T->a = s_x; T->c = 0.0;
  T->b = 0.0; T->d = s_y;
  T->e = s_x * d_x; T->f = s_y * d_y - dp;

  return;
}


/* called from pdfdev.c and spc_html.c */
int
pdf_ximage_scale_image (int            id,
                        pdf_tmatrix    *M, /* return value for trans matrix */
                        pdf_rect       *r, /* return value for clipping */
                        transform_info *p  /* argument from specials */
                       )
{
  struct ic_ *ic = &_ic;
  pdf_ximage *I;

  CHECK_ID(ic, id);

  I = GET_IMAGE(ic, id);

  pdf_setmatrix(M, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

  switch (I->subtype) {
  /* Reference: PDF Reference 1.5 v6, p.302
   *
   * An image can be placed on the output page in any desired position,
   * orientation, and size by using the cm operator to modify the current
   * transformation matrix (CTM) so as to map the unit square of user space
   * to the rectangle or parallelogram in which the image is to be painted.
   *
   * There is neither BBox nor Matrix key in the image XObject.
   * Everything must be controlled by the cm operator.
   *
   * The argument [p] contains the user-defined bounding box, the scailing
   * factor of which is bp as EPS and PDF. On the other hand, I->attr
   * contains the (sampling) width and the (sampling) height of the image.
   *
   * There is no problem if a bitmap image has density information.
   * Otherwise, DVIPDFM's ebb generates bounding box as 100px = 72bp = 1in.
   * In this case, screen captured images look bad. Moreover, DVIPDFM's ebb
   * ignores all density information and use just 100px = 72bp = 1in.
   *
   * On the other hand, pdfTeX uses 100px = 100bp to get a better quality
   * for screen captured images.
   *
   * DVIPDFMx's xbb generates bounding box as 100px = 100bp in the same
   * way as pdfTeX. Furthermore, it takes care of density information too.
   */
  case PDF_XOBJECT_TYPE_IMAGE:
    scale_to_fit_I(M, p, I);
    if (p->flags & INFO_HAS_USER_BBOX) {
      r->llx = p->bbox.llx / (I->attr.width * I->attr.xdensity);
      r->lly = p->bbox.lly / (I->attr.height * I->attr.ydensity);
      r->urx = p->bbox.urx / (I->attr.width * I->attr.xdensity);
      r->ury = p->bbox.ury / (I->attr.height * I->attr.ydensity);
    } else {
      r->llx = 0.0;
      r->lly = 0.0;
      r->urx = 1.0;
      r->ury = 1.0;
    }
    break;
  /* User-defined transformation and clipping are controlled by
   * the cm operator and W operator, explicitly */
  case PDF_XOBJECT_TYPE_FORM:
    scale_to_fit_F(M, p, I);
    if (p->flags & INFO_HAS_USER_BBOX) {
      r->llx = p->bbox.llx;
      r->lly = p->bbox.lly;
      r->urx = p->bbox.urx;
      r->ury = p->bbox.ury;
    } else { /* I->attr.bbox from the image bounding box */
      r->llx = I->attr.bbox.llx;
      r->lly = I->attr.bbox.lly;
      r->urx = I->attr.bbox.urx;
      r->ury = I->attr.bbox.ury;
    }
    break;
  default: /* maybe reserved */
    if (p->flags & INFO_HAS_USER_BBOX) {
      r->llx = p->bbox.llx;
      r->lly = p->bbox.lly;
      r->urx = p->bbox.urx;
      r->ury = p->bbox.ury;
    } else { /* I->attr.bbox from the image bounding box */
      r->llx = 0.0;
      r->lly = 0.0;
      r->urx = 1.0;
      r->ury = 1.0;
    }
  }

  return  0;
}


/* Migrated from psimage.c */

void set_distiller_template (char *s) 
{
  if (_opts.cmdtmpl)
    RELEASE(_opts.cmdtmpl);
  if (!s || *s == '\0')
    _opts.cmdtmpl = NULL;
  else {
    _opts.cmdtmpl = NEW(strlen(s) + 1, char);
    strcpy(_opts.cmdtmpl, s);
  }
  return;
}

char *get_distiller_template (void)
{
  return _opts.cmdtmpl;
}

static int
ps_include_page (pdf_ximage *ximage, const char *filename, load_options options)
{
  char  *distiller_template = _opts.cmdtmpl;
  char  *temp;
  FILE  *fp;
  int    error = 0;
  struct stat stat_o, stat_t;
#if defined(_WIN32)
  char *utf8temp;
  wchar_t *wtemp;
  char *ftest = NULL;
#endif
  if (!distiller_template) {
    WARN("No image converter available for converting file \"%s\" to PDF format.", filename);
    WARN(">> Please check if you have 'D' option in config file.");
    return  -1;
  }

  temp = dpx_create_fix_temp_file(filename);
  if (!temp) {
    WARN("Failed to create temporary file for image conversion: %s", filename);
    return  -1;
  }

#ifdef MIKTEX
  {
    char *p;
    for (p = (char *)filename; *p; p++) {
      if (*p == '\\') *p = '/';
    }
    for (p = (char *)temp; *p; p++) {
      if (*p == '\\') *p = '/';
    }
  }
#endif

#if defined(_WIN32)
/* temp is always win32_codepage only. So fsyscp_stat() is not
 * necessary for temp. However, filename can be non-ASCII UTF-8.
 */
  if (dpx_conf.file.keep_cache != -1 &&
      stat(temp, &stat_t)==0 &&
      (fsyscp_stat(filename, &stat_o)==0 ||
       stat(filename, &stat_o)==0) && 
      stat_t.st_mtime > stat_o.st_mtime) {
#else
  if (dpx_conf.file.keep_cache != -1 &&
      stat(temp, &stat_t)==0 && stat(filename, &stat_o)==0 && 
      stat_t.st_mtime > stat_o.st_mtime) {
#endif /* _WIN32 */
    /* cache exist */
    /*printf("\nLast file modification: %s", ctime(&stat_o.st_mtime));
      printf("Last file modification: %s", ctime(&stat_t.st_mtime));*/
      ;
  } else {
    if (dpx_conf.verbose_level > 1) {
      MESG("\n");
      MESG("pdf_image>> Converting file \"%s\" --> \"%s\" via:\n", filename, temp);
      MESG("pdf_image>>   %s\n", distiller_template);
      MESG("pdf_image>> ...");
    }

/* Support non-ascii TEMP and TMP on Windows to call Ghostscript */
#if defined(_WIN32)
    ftest = dpx_find_file(filename, "_pic_", "");
    if (ftest) {
       wtemp = get_wstring_from_mbstring(win32_codepage,
               temp, wtemp = NULL);
       utf8temp = get_mbstring_from_wstring(file_system_codepage,
                  wtemp, utf8temp = NULL);
       error = dpx_file_apply_filter(distiller_template, filename,
               utf8temp, pdf_get_version());
    } else {
       utf8name_failed = 1;
       error = dpx_file_apply_filter(distiller_template, filename,
               temp, pdf_get_version());
       utf8name_failed = 0;
    }
#else
    error = dpx_file_apply_filter(distiller_template, filename, temp,
      pdf_get_version());
#endif
    if (error) {
      WARN("Image format conversion for \"%s\" failed...", filename);
      dpx_delete_temp_file(temp, true);
      return  error;
    }
  }
#if defined(_WIN32)
/* Use a simple fopen() since temp is in win32_codepage */
  fp = fopen(temp, FOPEN_RBIN_MODE);
#else
  fp = MFOPEN(temp, FOPEN_RBIN_MODE);
#endif
  if (!fp) {
    WARN("Could not open conversion result \"%s\" for image \"%s\". Why?", temp, filename);
    dpx_delete_temp_file(temp, true);
    return  -1;
  }
  pdf_set_ximage_tempfile(ximage, temp);
  error = pdf_include_page(ximage, fp, temp, options);
  MFCLOSE(fp);

  /* See pdf_close_images for why we cannot delete temporary files here. */
#if defined(_WIN32)
  if (ftest) {
    RELEASE(utf8temp);
    RELEASE(wtemp);
    RELEASE(ftest);
  }
#endif
  RELEASE(temp);

  if (error) {
    WARN("Failed to include image file \"%s\"", filename);
    WARN(">> Please check if");
    WARN(">>   %s", distiller_template);
    WARN(">>   %%o = output filename, %%i = input filename, %%b = input filename without suffix");
    WARN(">> can really convert \"%s\" to PDF format image.", filename);
  }

  return  error;
}

static int check_for_ps (FILE *image_file) 
{
  rewind (image_file);
  mfgets (work_buffer, WORK_BUFFER_SIZE, image_file);
  if (!strncmp (work_buffer, "%!", 2))
    return 1;
  return 0;
}

static int check_for_mp (FILE *image_file) 
{
  int try_count = 10;

  rewind (image_file);
  mfgets(work_buffer, WORK_BUFFER_SIZE, image_file);
  if (strncmp(work_buffer, "%!PS", 4))
    return 0;

  while (try_count > 0) {
    mfgets(work_buffer, WORK_BUFFER_SIZE, image_file);
    if (!strncmp(work_buffer, "%%Creator:", 10)) {
      if (strlen(work_buffer+10) >= 8 &&
	  strstr(work_buffer+10, "MetaPost"))
	break;
    }
    try_count--;
  }

  return ((try_count > 0) ? 1 : 0);
}

/* ERROR() can't be used here otherwise the cleanup routine is recursively called. */
#undef ERROR
void
pdf_error_cleanup_cache (void)
{
  struct ic_ *ic = &_ic;
  int         i;
  pdf_ximage *I;

  for (i = 0; i < ic->count; i++) {
    I = &ic->ximages[i];
    if (I->attr.tempfile) {
      dpx_delete_temp_file(I->fullname, false); /* temporary filename freed here */
    }
  }
}
