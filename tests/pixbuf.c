#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <check.h>
#include <stdlib.h>
#include <libmb/mb.h>

/**
 * Contains the in-memory representation of oh.png, to verify the PNG loader
 * works.
 */
#include "oh.h"

Display *dpy;
MBPixbuf *pb = NULL;

/**
 * Utility routine to dump a MBPixbufImage as a raw file.
 */
static void dump_image(MBPixbufImage *img)
{
  FILE *f;
  int i;
  f = fopen("dump.raw", "wb");
  for (i = 0; i < (img->width * img->height * (3 + img->has_alpha)); i=i+4) {
    fputc(img->rgba[i], f);
    fputc(img->rgba[i+1], f);
    fputc(img->rgba[i+2], f);
  }
  fclose(f);
}

/**
 * Setup for the tests. Connects to the X server and constructs a MBPixmap.
 */
static void setup(void) { 
  dpy = XOpenDisplay(NULL);
  fail_unless (dpy != NULL, "setup(): could not connect to X server");
  pb = mb_pixbuf_new(dpy, DefaultScreen(dpy));
  fail_unless (pb != NULL, NULL);
}

/**
 * Teardown for the tests.  Closes the X connection.
 */
static void teardown(void)
{
  /* TODO: Destroy MBPixbuf when it is available */
  XCloseDisplay(dpy);
}


static int compare_with_pixel(MBPixbufImage *img, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  int x, y;
  for (y = 0; y < mb_pixbuf_img_get_height (img); ++y) {
    for (x = 0; x < mb_pixbuf_img_get_width (img); ++x) {
      unsigned char tr, tg, tb, ta;
      mb_pixbuf_img_get_pixel (pb, img, x, y, &tr, &tg, &tb, &ta);
      if (!(tr == r && tg == g && tb == b && ta == a)) return 0;
    }
  }
  return 1;
}

static int compare_with_array(MBPixbufImage *img, unsigned char* data)
{
  int i;
  if (img == NULL || data == NULL) return 0;
  for (i = 0; i < (img->width * img->height * (3 + img->has_alpha)); ++i) {
    if (img->rgba[i] != data[i]) return 0;
  }
  return 1;
}

static int compare_with_image (MBPixbufImage *a, MBPixbufImage *b)
{
  int i;
  if (a == NULL || b == NULL) return 0;
  if (a->width != b->width || a->height != b->height || a->has_alpha != b->has_alpha) return 0;
  for (i = 0; i < (a->width * a->height * (3 + a->has_alpha)); ++i) {
    //printf("Comparing %d with %d\n", a->rgba[i], b->rgba[i]);
    if (a->rgba[i] != b->rgba[i]) return 0;
  }
  return 1;
}


START_TEST (pixbuf_rgb_new_fill)
{
  MBPixbufImage *img;
  img = mb_pixbuf_img_rgb_new (pb, 32, 32);
  fail_unless (img != NULL, NULL);
  /* Image should be solid black, with non-transparent pixels */
  fail_unless (compare_with_pixel (img, 0, 0, 0, 255), NULL);
  mb_pixbuf_img_fill (pb, img, 10, 20, 30, 40);
  /* We asked for an RGB image, so the alpha should have been ignored */
  fail_unless (compare_with_pixel (img, 10, 20, 30, 255), NULL);
  mb_pixbuf_img_free (pb, img);
}
END_TEST

START_TEST (pixbuf_rgba_new_fill)
{
  MBPixbufImage *img;
  img = mb_pixbuf_img_rgba_new (pb, 32, 32);
  fail_unless (img != NULL, NULL);
  /* Image should be 100% transparent and black */
  fail_unless (compare_with_pixel (img, 0, 0, 0, 0), NULL);
  mb_pixbuf_img_fill (pb, img, 10, 20, 30, 40);
  /* We asked for an RGBA image, so the alpha should have been respected this time */
  fail_unless (compare_with_pixel (img, 10, 20, 30, 40), NULL);
  mb_pixbuf_img_free (pb, img);
}
END_TEST

/**
 * Test that mbpixmap can load a PNG correctly.
 */
START_TEST (pixbuf_load_png)
{
  MBPixbufImage *img;
  img = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (img != NULL, NULL);
  fail_unless (mb_pixbuf_img_get_width (img) == 16, NULL);
  fail_unless (mb_pixbuf_img_get_height (img) == 16, NULL);
  fail_unless (compare_with_array (img, OH), NULL);
  mb_pixbuf_img_free (pb, img);
}
END_TEST

/**
 * Test that mbpixmap can clone.
 */
START_TEST (pixbuf_clone)
{
  MBPixbufImage *img1, *img2;
  img1 = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (img1 != NULL, NULL);
  img2 = mb_pixbuf_img_clone (pb, img1);
  fail_unless (img2 != NULL, NULL);
  fail_unless (img2 != img1, NULL);
  fail_unless (compare_with_image (img1, img2), NULL);
  mb_pixbuf_img_free (pb, img1);
  mb_pixbuf_img_free (pb, img2);
}
END_TEST

START_TEST (pixbuf_composite)
{
  MBPixbufImage *oh, *overlay, *expected;
  oh = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (oh != NULL, NULL);
  overlay = mb_pixbuf_img_new_from_file (pb, "overlay.png");
  fail_unless (overlay != NULL, NULL);
  expected = mb_pixbuf_img_new_from_file (pb, "oh-overlayed.png");
  fail_unless (expected != NULL, NULL);
  mb_pixbuf_img_copy_composite (pb, oh, overlay, 0, 0, 16, 16, 0, 0);
  fail_unless (compare_with_image (oh, expected), "Composite image incorrect");
  mb_pixbuf_img_free (pb, oh);
  mb_pixbuf_img_free (pb, overlay);
  mb_pixbuf_img_free (pb, expected);
}
END_TEST

START_TEST (pixbuf_rotate_90_identity)
{
  MBPixbufImage *img1, *img2, *orig;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (orig != NULL, NULL);
  img1 = mb_pixbuf_img_transform (pb, orig, MBPIXBUF_TRANS_ROTATE_90);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_ROTATE_90); mb_pixbuf_img_free (pb, img1);
  fail_unless (!compare_with_image (img2, orig), NULL);
  img1 = mb_pixbuf_img_transform (pb, img2, MBPIXBUF_TRANS_ROTATE_90); mb_pixbuf_img_free (pb, img2);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_ROTATE_90); mb_pixbuf_img_free (pb, img1);
  fail_unless (compare_with_image (img2, orig), NULL);
  mb_pixbuf_img_free (pb, img2);
  mb_pixbuf_img_free (pb, orig);
}
END_TEST

START_TEST (pixbuf_rotate_270_identity)
{
  MBPixbufImage *img1, *img2, *orig;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (orig != NULL, NULL);
  img1 = mb_pixbuf_img_transform (pb, orig, MBPIXBUF_TRANS_ROTATE_270);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_ROTATE_270); mb_pixbuf_img_free (pb, img1);
  fail_unless (!compare_with_image (img2, orig), NULL);
  img1 = mb_pixbuf_img_transform (pb, img2, MBPIXBUF_TRANS_ROTATE_270); mb_pixbuf_img_free (pb, img2);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_ROTATE_270); mb_pixbuf_img_free (pb, img1);
  fail_unless (compare_with_image (img2, orig), NULL);
  mb_pixbuf_img_free (pb, img2);
  mb_pixbuf_img_free (pb, orig);
}
END_TEST

START_TEST (pixbuf_rotate_180_identity)
{
  MBPixbufImage *img1, *img2, *orig;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (orig != NULL, NULL);
  img1 = mb_pixbuf_img_transform (pb, orig, MBPIXBUF_TRANS_ROTATE_180);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_ROTATE_180); mb_pixbuf_img_free (pb, img1);
  fail_unless (compare_with_image (img2, orig), NULL);
  mb_pixbuf_img_free (pb, img2);
  mb_pixbuf_img_free (pb, orig);
}
END_TEST

START_TEST (pixbuf_flip_h_identity)
{
  MBPixbufImage *img1, *img2, *orig;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (orig != NULL, NULL);
  img1 = mb_pixbuf_img_transform (pb, orig, MBPIXBUF_TRANS_FLIP_HORIZ);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_FLIP_HORIZ); mb_pixbuf_img_free (pb, img1);
  fail_unless (compare_with_image (img2, orig), NULL);
  mb_pixbuf_img_free (pb, img2);
  mb_pixbuf_img_free (pb, orig);
}
END_TEST

START_TEST (pixbuf_flip_v_identity)
{
  MBPixbufImage *img1, *img2, *orig;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (orig != NULL, NULL);
  img1 = mb_pixbuf_img_transform (pb, orig, MBPIXBUF_TRANS_FLIP_VERT);
  fail_unless (!compare_with_image (img1, orig), NULL);
  img2 = mb_pixbuf_img_transform (pb, img1, MBPIXBUF_TRANS_FLIP_VERT); mb_pixbuf_img_free (pb, img1);
  fail_unless (compare_with_image (img2, orig), NULL);
  mb_pixbuf_img_free (pb, img2);
  mb_pixbuf_img_free (pb, orig);
}
END_TEST

START_TEST (pixbuf_scale)
{
  MBPixbufImage *img, *orig, *exp;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  fail_unless (orig != NULL, NULL);
  exp = mb_pixbuf_img_new_from_file (pb, "oh-scaled.png");
  fail_unless (orig != NULL, NULL);
  img = mb_pixbuf_img_scale (pb, orig, 32, 32);
  fail_unless (!compare_with_image (img, orig), NULL);
  fail_unless (compare_with_image (img, exp), NULL);
  mb_pixbuf_img_free (pb, img);
  mb_pixbuf_img_free (pb, orig);
  mb_pixbuf_img_free (pb, exp);
}
END_TEST

/**
 * Test that the scale_(up|down) functions correctly return NULL if you try and
 * scale an image the wrong way.
 */
START_TEST (pixbuf_scale_failures)
{
  MBPixbufImage *img, *orig;
  orig = mb_pixbuf_img_new_from_file (pb, "oh.png");
  img = mb_pixbuf_img_scale_down (pb, orig, 32, 32);
  fail_unless (img == NULL, NULL);
  img = mb_pixbuf_img_scale_up (pb, orig, 8, 8);
  fail_unless (img == NULL, NULL);
  mb_pixbuf_img_free (pb, orig);
}
END_TEST

Suite *pixbuf_suite(void)
{
  Suite *s = suite_create("MbPixbuf");
  TCase *tc_core = tcase_create("Core");
  tcase_add_checked_fixture(tc_core, setup, teardown);
  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, pixbuf_rgb_new_fill);
  tcase_add_test(tc_core, pixbuf_rgba_new_fill);
  tcase_add_test(tc_core, pixbuf_load_png);
  tcase_add_test(tc_core, pixbuf_clone);
  tcase_add_test(tc_core, pixbuf_composite);
  tcase_add_test(tc_core, pixbuf_rotate_90_identity);
  tcase_add_test(tc_core, pixbuf_rotate_180_identity);
  tcase_add_test(tc_core, pixbuf_rotate_270_identity);
  tcase_add_test(tc_core, pixbuf_flip_h_identity);
  tcase_add_test(tc_core, pixbuf_flip_v_identity);
  tcase_add_test(tc_core, pixbuf_scale);
  tcase_add_test(tc_core, pixbuf_scale_failures);
  return s;
}

int main(void)
{
  int nf;
  Suite *s = pixbuf_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  nf = srunner_ntests_failed(sr);
  suite_free (s);
  srunner_free(sr);
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

