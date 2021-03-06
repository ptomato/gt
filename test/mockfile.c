/*
 * Copyright 2014, 2015 Philip Chimento <philip.chimento@gmail.com>
 *
 * This file is part of Gt.
 *
 * Gt is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Gt is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Gt. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>

#include "gt.h"

#define SAMPLE_UTF8_CONTENTS "My big sphinx of quartz"

typedef struct
{
  GFile *file;
} Fixture;

static void
setup (Fixture      *fixture,
       gconstpointer unused)
{
  fixture->file = G_FILE (gt_mock_file_new ());
}

static void
teardown (Fixture      *fixture,
          gconstpointer unused)
{
  g_object_unref (fixture->file);
}

static void
test_g_object_new (void)
{
  GFile *file = G_FILE (g_object_new (GT_TYPE_MOCK_FILE, NULL));
  char *uri = g_file_get_uri (file);
  g_assert_true (g_regex_match_simple ("^gt-mock://[a-f0-9]{32}$", uri, 0, 0));
  g_free (uri);
}

static void
test_new_with_id (void)
{
  GFile *file = G_FILE (gt_mock_file_new_with_id ("0123456789abcdef0123456789abcdef"));
  char *uri = g_file_get_uri (file);
  g_assert_cmpstr(uri, ==, "gt-mock://0123456789abcdef0123456789abcdef");
  g_free (uri);
}

static void
test_mock_does_not_prevent_creating_normal_files_from_path (void)
{
  GFile *file = g_file_new_for_path ("a/b/c");
  g_assert_false (GT_IS_MOCK_FILE (file));
  g_object_unref (file);
}

static void
test_mock_does_not_prevent_creating_normal_files_from_uri (void)
{
  GFile *file = g_file_new_for_uri ("file:///a/b/c");
  g_assert_false (GT_IS_MOCK_FILE (file));
  g_object_unref (file);
}

static void
test_mock_does_not_prevent_parsing_normal_files_names (void)
{
  GFile *file = g_file_new_for_path ("a/b/c");
  char *parse_name = g_file_get_parse_name (file);
  g_object_unref (file);
  file = g_file_parse_name (parse_name);
  g_free (parse_name);
  g_assert_false (GT_IS_MOCK_FILE (file));
  g_object_unref (file);
}

static void
test_mock_is_not_native (Fixture      *fixture,
                         gconstpointer unused)
{
  g_assert_false (g_file_is_native (fixture->file));
}

static void
test_mock_has_uri_scheme (Fixture      *fixture,
                          gconstpointer unused)
{
  g_assert (g_file_has_uri_scheme (fixture->file, "gt-mock"));
}

static void
test_mock_get_uri_scheme (Fixture      *fixture,
                          gconstpointer unused)
{
  char *scheme = g_file_get_uri_scheme (fixture->file);
  g_assert_cmpstr (scheme, ==, "gt-mock");
  g_free (scheme);
}

static void
test_mock_exists_by_default (Fixture      *fixture,
                             gconstpointer unused)
{
  g_assert_true (gt_mock_file_get_exists (GT_MOCK_FILE (fixture->file)));
  g_assert_true (g_file_query_exists (fixture->file, NULL));
}

static void
test_mock_is_regular_file (Fixture      *fixture,
                           gconstpointer unused)
{
  GFileType file_type = g_file_query_file_type (fixture->file, G_FILE_QUERY_INFO_NONE, NULL);
  g_assert_cmpuint (file_type, ==, G_FILE_TYPE_REGULAR);
}

static void
test_mock_starts_empty (Fixture      *fixture,
                        gconstpointer unused)
{
  GBytes *bytes = gt_mock_file_get_contents (GT_MOCK_FILE (fixture->file));
  g_assert_cmpuint(g_bytes_get_size (bytes), ==, 0);

  const char *contents = gt_mock_file_get_contents_utf8 (GT_MOCK_FILE (fixture->file));
  g_assert_cmpstr (contents, ==, "");
}

static void
test_mock_stores_contents (Fixture      *fixture,
                           gconstpointer unused)
{
  static guint8 sample_bytes[] = {0, 1, 2, 3, 4, 5};
  GBytes *contents = g_bytes_new_static (sample_bytes, 6);
  int index;
  const guint8 *data;
  size_t size;

  gt_mock_file_set_contents (GT_MOCK_FILE (fixture->file), contents);
  g_bytes_unref (contents);

  contents = gt_mock_file_get_contents (GT_MOCK_FILE (fixture->file));
  data = g_bytes_get_data (contents, &size);

  g_assert_cmpint (size, ==, 6);
  for (index = 0; index < 6; index++)
    g_assert_cmpuint (data[index], ==, index);
}

static void
test_mock_stores_contents_utf8 (Fixture      *fixture,
                                gconstpointer unused)
{
  gt_mock_file_set_contents_utf8 (GT_MOCK_FILE (fixture->file),
                                  SAMPLE_UTF8_CONTENTS);
  const char *contents = gt_mock_file_get_contents_utf8 (GT_MOCK_FILE (fixture->file));
  g_assert_cmpstr (contents, ==, SAMPLE_UTF8_CONTENTS);
}

static void
test_mock_reads_contents (Fixture      *fixture,
                          gconstpointer unused)
{
  GError *error = NULL;
  gt_mock_file_set_contents_utf8 (GT_MOCK_FILE (fixture->file),
                                  SAMPLE_UTF8_CONTENTS);
  GFileInputStream *istream = g_file_read (fixture->file, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (istream);

  GDataInputStream *dis = g_data_input_stream_new (G_INPUT_STREAM (istream));
  g_object_unref (istream);
  char *contents = g_data_input_stream_read_line_utf8 (dis, NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (contents);
  g_object_unref (dis);

  g_assert_cmpstr(contents, ==, SAMPLE_UTF8_CONTENTS);
  g_free (contents);
}

static void
test_mock_writes_contents (void)
{
  GFile *file = G_FILE (g_object_new (GT_TYPE_MOCK_FILE,
                                      "exists", FALSE,
                                      NULL));
  GError *error = NULL;
  GFileOutputStream *ostream = g_file_create (file, G_FILE_CREATE_NONE, NULL,
                                              &error);
  g_assert_no_error (error);
  g_assert_nonnull (ostream);

  GDataOutputStream *dos = g_data_output_stream_new (G_OUTPUT_STREAM (ostream));
  g_object_unref (ostream);
  g_assert (g_data_output_stream_put_string (dos, SAMPLE_UTF8_CONTENTS,
                                             NULL, &error));
  g_assert_no_error (error);
  g_object_unref (dos);

  const char *contents = gt_mock_file_get_contents_utf8 (GT_MOCK_FILE (file));
  g_assert_cmpstr (contents, ==, SAMPLE_UTF8_CONTENTS);

  g_object_unref (file);
}


int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/mock/g-object-new-constructor", test_g_object_new);
  g_test_add_func ("/mock/new-with-id", test_new_with_id);
  g_test_add_func ("/mock/does-not-prevent-creating-normal-files-from-path",
                   test_mock_does_not_prevent_creating_normal_files_from_path);
  g_test_add_func ("/mock/does-not-prevent-creating-normal-files-from-uri",
                   test_mock_does_not_prevent_creating_normal_files_from_uri);
  g_test_add_func ("/mock/does-not-prevent-parsing-normal-files-names",
                   test_mock_does_not_prevent_parsing_normal_files_names);

#define ADD_MOCK_FILE_TEST(path, test_func) \
  g_test_add ((path), Fixture, NULL, setup, (test_func), teardown)

  /* Specific functionality of GtMockFile */
  ADD_MOCK_FILE_TEST ("/mock/is-not-native", test_mock_is_not_native);
  ADD_MOCK_FILE_TEST ("/mock/has-uri-scheme", test_mock_has_uri_scheme);
  ADD_MOCK_FILE_TEST ("/mock/get-uri-scheme", test_mock_get_uri_scheme);
  ADD_MOCK_FILE_TEST ("/mock/exists-by-default", test_mock_exists_by_default);
  ADD_MOCK_FILE_TEST ("/mock/is-regular-file", test_mock_is_regular_file);
  ADD_MOCK_FILE_TEST ("/mock/starts-empty", test_mock_starts_empty);
  ADD_MOCK_FILE_TEST ("/mock/stores-contents", test_mock_stores_contents);
  ADD_MOCK_FILE_TEST ("/mock/stores-contents-utf8", test_mock_stores_contents_utf8);
  ADD_MOCK_FILE_TEST ("/mock/reads-contents", test_mock_reads_contents);

#undef ADD_MOCK_FILE_TEST

  g_test_add_func ("/mock/writes-contents", test_mock_writes_contents);

  return g_test_run ();
}
