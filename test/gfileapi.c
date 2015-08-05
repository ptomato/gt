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

#include <string.h>

#include "gt.h"
#include "gt-expect.h"

#define SAMPLE_UTF8_CONTENTS "My big sphinx of quartz"

/* FIXTURE */

typedef GObject *(*FileFinishFunc)(GFile *, GAsyncResult *, GError **);
typedef gboolean (*FileBoolFinishFunc)(GFile *, GAsyncResult *, GError **);

typedef struct
{
  GFile *file;
  GCancellable *cancellable;
  GMainLoop *mainloop;
  FileFinishFunc finish_func;
  gboolean measure_disk_usage_progress_callback_called;
  gboolean copy_progress_callback_called;
  gboolean copy_progress_callback_called_complete;
} Fixture;

static void
setup (Fixture      *fixture,
       gconstpointer unused)
{
  fixture->file = G_FILE (g_object_new (GT_TYPE_MOCK_FILE,
                                        "exists", FALSE,
                                        NULL));
  fixture->cancellable = g_cancellable_new ();
  fixture->mainloop = g_main_loop_new (NULL, FALSE);
}

static void
teardown (Fixture      *fixture,
          gconstpointer unused)
{
  g_object_unref (fixture->file);
  g_object_unref (fixture->cancellable);
  g_main_loop_unref (fixture->mainloop);
}

/* HELPER FUNCTIONS */

/* FIXME: __VA_ARGS__ is C99 and ##__VA_ARGS__ is a GCC extension? */

#define GENERATE_IO_METHOD_TEST_SYNC(method, ...) \
  static void \
  test_##method (Fixture      *fixture, \
                 gconstpointer unused) \
  { \
    GError *error = NULL; \
    GObject *result = G_OBJECT (g_file_##method (fixture->file , ##__VA_ARGS__, NULL, &error)); \
    assert_and_free_object_or_error (result, error); \
  }

#define GENERATE_IO_METHOD_TESTS_ASYNC(method, ...) \
  static void \
  test_##method##_async (Fixture      *fixture, \
                         gconstpointer unused) \
  { \
    fixture->finish_func = (FileFinishFunc)g_file_##method##_finish; \
    g_file_##method##_async (fixture->file , ##__VA_ARGS__, G_PRIORITY_DEFAULT, NULL, \
                             on_async_assert_object, fixture); \
    g_main_loop_run (fixture->mainloop); \
  } \
  \
  static void \
  test_##method##_cancel (Fixture      *fixture, \
                          gconstpointer unused) \
  { \
    fixture->finish_func = (FileFinishFunc)g_file_##method##_finish; \
    g_file_##method##_async (fixture->file , ##__VA_ARGS__, G_PRIORITY_DEFAULT, fixture->cancellable, \
                             on_async_assert_cancelled, fixture); \
    g_cancellable_cancel (fixture->cancellable); \
    g_main_loop_run (fixture->mainloop); \
  }

#define GENERATE_IO_METHOD_TESTS(method, ...) \
  GENERATE_IO_METHOD_TEST_SYNC (method , ##__VA_ARGS__) \
  GENERATE_IO_METHOD_TESTS_ASYNC (method , ##__VA_ARGS__)

#define GENERATE_IO_METHOD_TEST_SYNC_BOOL(method, ...) \
  static void \
  test_##method (Fixture      *fixture, \
                 gconstpointer unused) \
  { \
    GError *error = NULL; \
    gboolean success = g_file_##method (fixture->file , ##__VA_ARGS__, NULL, &error); \
    assert_success_or_free_error (success, error); \
  }

#define GENERATE_IO_METHOD_TESTS_ASYNC_BOOL(method, ...) \
  static void \
  test_##method##_async (Fixture      *fixture, \
                         gconstpointer unused) \
  { \
    fixture->finish_func = (FileFinishFunc)g_file_##method##_finish; \
    g_file_##method##_async (fixture->file , ##__VA_ARGS__, G_PRIORITY_DEFAULT, NULL, \
                             on_async_assert_bool, fixture); \
    g_main_loop_run (fixture->mainloop); \
  } \
  \
  static void \
  test_##method##_cancel (Fixture      *fixture, \
                          gconstpointer unused) \
  { \
    fixture->finish_func = (FileFinishFunc)g_file_##method##_finish; \
    g_file_##method##_async (fixture->file , ##__VA_ARGS__, G_PRIORITY_DEFAULT, fixture->cancellable, \
                             on_async_assert_bool_cancelled, fixture); \
    g_cancellable_cancel (fixture->cancellable); \
    g_main_loop_run (fixture->mainloop); \
  }

#define GENERATE_IO_METHOD_TESTS_BOOL(method, ...) \
  GENERATE_IO_METHOD_TEST_SYNC_BOOL(method , ##__VA_ARGS__) \
  GENERATE_IO_METHOD_TESTS_ASYNC_BOOL(method , ## __VA_ARGS__)

#define GENERATE_COPY_METHOD_TEST_SYNC(method) \
  static void \
  test_##method (Fixture      *fixture, \
                 gconstpointer unused) \
  { \
    GError *error = NULL; \
    GFile *file2 = G_FILE (gt_mock_file_new ()); \
    gboolean success = g_file_##method (fixture->file, file2, G_FILE_COPY_NONE, NULL, \
                                        (GFileProgressCallback)copy_progress_callback, fixture, \
                                        &error); \
    assert_success_or_free_error (success, error); \
    if (success) \
      { \
        gt_expect_bool (fixture->copy_progress_callback_called, TO_BE_TRUE); \
        gt_expect_bool (fixture->copy_progress_callback_called_complete, TO_BE_TRUE); \
      } \
  }

#define GENERATE_ATTRIBUTE_LIST_METHOD_TEST_SYNC(method) \
  static void \
  test_##method (Fixture      *fixture, \
                 gconstpointer unused) \
  { \
    GError *error = NULL; \
    GFileAttributeInfoList *retval = g_file_##method (fixture->file, NULL, &error); \
    if (retval != NULL) \
      { \
        gt_expect_error (error, TO_BE_CLEAR); \
        g_file_attribute_info_list_unref (retval); \
      } \
    else \
      { \
        g_error_free (error); \
      } \
  }

#define GENERATE_MOUNT_METHOD_TESTS(method, ...) \
  static void \
  test_##method (Fixture      *fixture, \
                 gconstpointer unused) \
  { \
    fixture->finish_func = (FileFinishFunc)g_file_##method##_finish; \
    g_file_##method (fixture->file , ##__VA_ARGS__, NULL, NULL, \
                     on_async_assert_bool, fixture); \
    g_main_loop_run (fixture->mainloop); \
  } \
  \
  static void \
  test_##method##_cancel (Fixture      *fixture, \
                          gconstpointer unused) \
  { \
    fixture->finish_func = (FileFinishFunc)g_file_##method##_finish; \
    /* mount methods don't necessarily start the main loop before checking for \
    cancellation */ \
    g_cancellable_cancel (fixture->cancellable); \
    g_file_##method (fixture->file , ##__VA_ARGS__, NULL, fixture->cancellable, \
                     on_async_assert_bool_cancelled, fixture); \
    g_main_loop_run (fixture->mainloop); \
  }

static void
assert_and_free_object_or_error (gpointer object,
                                 GError  *error)
{
  if (object != NULL)
    {
      gt_expect_object (object, TO_BE_A, G_TYPE_OBJECT);
      gt_expect_error (error, TO_BE_CLEAR);
      g_object_unref (object);
    }
  else
    {
      gt_expect_error (error, NOT TO_BE_CLEAR);
      g_error_free (error);
    }
}

static void
assert_success_or_free_error (gboolean success,
                              GError  *error)
{
  if (success)
    {
      gt_expect_error (error, TO_BE_CLEAR);
    }
  else
    {
      gt_expect_error (error, NOT TO_BE_CLEAR);
      g_error_free (error);
    }
}

static void
on_async_assert_object (GObject      *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  GObject *retval = fixture->finish_func (fixture->file, res, &error);
  assert_and_free_object_or_error (retval, error);
  g_main_loop_quit (fixture->mainloop);
}

static void
on_async_assert_cancelled (GObject      *source,
                           GAsyncResult *res,
                           gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  GObject *retval = fixture->finish_func (fixture->file, res, &error);
  gt_expect_object (retval, TO_BE_NULL);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_main_loop_quit (fixture->mainloop);
}

static void
on_async_assert_bool (GObject      *source,
                      GAsyncResult *res,
                      gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  gboolean success = ((FileBoolFinishFunc)fixture->finish_func) (fixture->file, res, &error);
  assert_success_or_free_error (success, error);
  g_main_loop_quit (fixture->mainloop);
}

static void
on_async_assert_bool_cancelled (GObject      *source,
                                GAsyncResult *res,
                                gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  gboolean success = ((FileBoolFinishFunc)fixture->finish_func) (fixture->file, res, &error);
  gt_expect_bool (success, TO_BE_FALSE);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_main_loop_quit (fixture->mainloop);
}

static void
copy_progress_callback (goffset  current_num_bytes,
                        goffset  total_num_bytes,
                        Fixture *fixture)
{
  fixture->copy_progress_callback_called = TRUE;
  if (current_num_bytes == total_num_bytes)
    fixture->copy_progress_callback_called_complete = TRUE;
}

/* GFile API TESTS */

static void
test_dup (Fixture      *fixture,
          gconstpointer unused)
{
  GFile *dup = g_file_dup (fixture->file);
  gt_expect_object (dup, TO_BE_A, GT_TYPE_MOCK_FILE);
  g_object_unref (dup);
}

static void
test_hash (Fixture      *fixture,
           gconstpointer unused)
{
  guint hash = g_file_hash (fixture->file);
  gt_expect_uint (hash, NOT TO_EQUAL, 0);
}

static void
test_equal_yes (Fixture      *fixture,
                gconstpointer unused)
{
  gt_expect_file (fixture->file, TO_EQUAL, fixture->file);
}

static void
test_equal_no (Fixture      *fixture,
               gconstpointer unused)
{
  GFile *file2 = G_FILE (gt_mock_file_new ());
  gt_expect_file (fixture->file, NOT TO_EQUAL, file2);
  g_object_unref (file2);
}

static void
test_get_basename (Fixture      *fixture,
                   gconstpointer unused)
{
  char *basename = g_file_get_basename (fixture->file);
  gt_expect_string (basename, NOT TO_BE_NULL);
  gt_expect_string (basename, NOT TO_BE_EMPTY);
  g_free (basename);
}

static void
test_get_path (Fixture      *fixture,
               gconstpointer unused)
{
  char *path = g_file_get_path (fixture->file);
  if (path != NULL)
    gt_expect_string (path, NOT TO_BE_EMPTY);
  g_free (path);
}

static void
test_get_uri (Fixture      *fixture,
              gconstpointer unused)
{
  char *uri = g_file_get_uri (fixture->file);
  gt_expect_string (uri, NOT TO_BE_NULL);
  gt_expect_string (uri, NOT TO_BE_EMPTY);
  g_free (uri);
}

static void
test_get_parse_name (Fixture      *fixture,
                     gconstpointer unused)
{
  char *parse_name = g_file_get_parse_name (fixture->file);
  gt_expect_string (parse_name, NOT TO_BE_NULL);
  gt_expect_string (parse_name, NOT TO_BE_EMPTY);
  g_free (parse_name);
}

static void
test_get_parent (Fixture      *fixture,
                 gconstpointer unused)
{
  GFile *parent = g_file_get_parent (fixture->file);
  if (parent != NULL) {
    gt_expect_file (fixture->file, TO_HAVE_PARENT, parent);
    g_object_unref (parent);
  }
}

static void
test_has_any_parent (Fixture      *fixture,
                     gconstpointer unused)
{
  g_file_has_parent (fixture->file, NULL);
}

static void
test_has_parent_no (Fixture      *fixture,
                    gconstpointer unused)
{
  GFile *not_parent = g_file_new_for_path ("foobar");
  gt_expect_file (fixture->file, NOT TO_HAVE_PARENT, not_parent);
}

static void
test_get_child (Fixture      *fixture,
                gconstpointer unused)
{
  GFile *child = g_file_get_child (fixture->file, "foobar");
  gt_expect_file (child, NOT TO_BE_NULL);
  char *basename = g_file_get_basename (child);
  gt_expect_string (basename, TO_EQUAL, "foobar");
  g_free (basename);
  gt_expect_file (child, TO_HAVE_PARENT, fixture->file);
  g_object_unref (child);
}

static void
test_get_child_for_display_name (Fixture      *fixture,
                                 gconstpointer unused)
{
  GError *error = NULL;
  GFile *child = g_file_get_child_for_display_name (fixture->file, "foobar", &error);
  if (child != NULL)
    {
      gt_expect_file (child, TO_HAVE_PARENT, fixture->file);
      GFileInfo *info = g_file_query_info (child,
                                           G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                           G_FILE_QUERY_INFO_NONE, NULL, NULL);
      if (info != NULL) {
        const char *display_name = g_file_info_get_display_name (info);
        gt_expect_string (display_name, TO_EQUAL, "foobar");
        g_object_unref (info);
      }
    }
  assert_and_free_object_or_error (child, error);
}

static void
test_has_prefix_not_related (Fixture      *fixture,
                             gconstpointer unused)
{
  GFile *file2 = g_file_new_for_path ("foobar");
  gt_expect_file (fixture->file, NOT TO_HAVE_PREFIX, file2);
  /* g_file_has_prefix() may return false negatives, but not false positives */
  g_object_unref (file2);
}

static void
test_has_prefix_same_file (Fixture      *fixture,
                           gconstpointer unused)
{
  gt_expect_file (fixture->file, TO_HAVE_PREFIX, fixture->file);
}

static void
test_has_prefix_child (Fixture      *fixture,
                       gconstpointer unused)
{
  GFile *child = g_file_get_child (fixture->file, "foobar");
  gt_expect_file (child, TO_HAVE_PREFIX, fixture->file);
  g_object_unref (child);
}

static void
test_has_prefix_descendant (Fixture      *fixture,
                            gconstpointer unused)
{
  GFile *child1 = g_file_get_child (fixture->file, "foobar");
  GFile *child2 = g_file_get_child (child1, "baz");
  g_object_unref (child1);
  gt_expect_file (child2, TO_HAVE_PREFIX, fixture->file);
  g_object_unref (child2);
}

static void
test_has_prefix_parent (Fixture      *fixture,
                        gconstpointer unused)
{
  GFile *parent = g_file_get_parent (fixture->file);
  gt_expect_file (fixture->file, TO_HAVE_PREFIX, parent);
  g_object_unref (parent);
}

static void
test_has_prefix_ancestor (Fixture      *fixture,
                                 gconstpointer unused)
{
  GFile *parent1 = g_file_get_parent (fixture->file);
  GFile *parent2 = g_file_get_parent (parent1);
  g_object_unref (parent1);
  gt_expect_file (fixture->file, TO_HAVE_PREFIX, parent2);
  g_object_unref (parent2);
}

static void
test_get_relative_path_not_related (Fixture      *fixture,
                                    gconstpointer unused)
{
  GFile *file2 = g_file_new_for_path ("foobar");
  gt_expect_string (g_file_get_relative_path (fixture->file, file2), TO_BE_NULL);
  g_object_unref (file2);
}

static void
test_get_relative_path_same_file (Fixture      *fixture,
                                  gconstpointer unused)
{
  char *relative_path = g_file_get_relative_path (fixture->file, fixture->file);
  gt_expect_string (relative_path, NOT TO_BE_NULL);
  gt_expect_string (relative_path, TO_EQUAL, ".");
  g_free (relative_path);
}

static void
test_get_relative_path_child (Fixture      *fixture,
                              gconstpointer unused)
{
  GFile *child = g_file_get_child (fixture->file, "foobar");
  char *relative_path = g_file_get_relative_path (fixture->file, child);
  g_object_unref (child);
  gt_expect_string (relative_path, NOT TO_BE_NULL);
  gt_expect_string (relative_path, TO_EQUAL, "foobar");
  g_free (relative_path);
}

static void
test_get_relative_path_descendant (Fixture      *fixture,
                                   gconstpointer unused)
{
  GFile *child1 = g_file_get_child (fixture->file, "foobar");
  GFile *child2 = g_file_get_child (child1, "baz");
  g_object_unref (child1);
  char *relative_path = g_file_get_relative_path (fixture->file, child2);
  g_object_unref (child2);
  gt_expect_string (relative_path, NOT TO_BE_NULL);
  gt_expect_string (relative_path, TO_EQUAL, "foobar/baz");
  g_free (relative_path);
}

static void
test_get_relative_path_parent (Fixture      *fixture,
                               gconstpointer unused)
{
  g_file_set_display_name (fixture->file, "foobar", NULL, NULL);
  GFile *parent = g_file_get_parent (fixture->file);
  char *relative_path = g_file_get_relative_path (parent, fixture->file);
  g_object_unref (parent);
  gt_expect_string (relative_path, NOT TO_BE_NULL);
  gt_expect_string (relative_path, TO_EQUAL, "foobar");
  g_free (relative_path);
}

static void
test_get_relative_path_ancestor (Fixture      *fixture,
                                 gconstpointer unused)
{
  g_file_set_display_name (fixture->file, "foobar", NULL, NULL);
  GFile *parent1 = g_file_get_parent (fixture->file);
  g_file_set_display_name (parent1, "baz", NULL, NULL);
  GFile *parent2 = g_file_get_parent (parent1);
  g_object_unref (parent1);
  char *relative_path = g_file_get_relative_path (parent2, fixture->file);
  g_object_unref (parent2);
  gt_expect_string (relative_path, NOT TO_BE_NULL);
  gt_expect_string (relative_path, TO_EQUAL, "baz/foobar");
  g_free (relative_path);
}

static void
test_resolve_relative_path_same_file (Fixture      *fixture,
                                      gconstpointer unused)
{
  GFile *file = g_file_resolve_relative_path (fixture->file, ".");
  gt_expect_file (fixture->file, TO_EQUAL, file);
  g_object_unref (file);
}

static void
test_resolve_relative_path_parent (Fixture      *fixture,
                                   gconstpointer unused)
{
  GFile *file = g_file_resolve_relative_path (fixture->file, "..");
  GFile *parent = g_file_get_parent (fixture->file);
  if (file != NULL && parent != NULL)
    gt_expect_file (file, TO_EQUAL, parent);
  if (file != NULL)
    g_object_unref (file);
  if (parent != NULL)
    g_object_unref (parent);
}

static void
test_resolve_relative_path_child (Fixture      *fixture,
                                  gconstpointer unused)
{
  GFile *file = g_file_resolve_relative_path (fixture->file, "foobar");
  GFile *child = g_file_get_child (fixture->file, "foobar");
  gt_expect_file (file, TO_EQUAL, child);
  g_object_unref (file);
  g_object_unref (child);
}

static void
test_is_native (Fixture      *fixture,
                gconstpointer unused)
{
  g_file_is_native (fixture->file);
}

static void
test_has_uri_scheme (Fixture      *fixture,
                     gconstpointer unused)
{
  g_file_has_uri_scheme (fixture->file, "file");
}

static void
test_get_uri_scheme (Fixture      *fixture,
                     gconstpointer unused)
{
  char *scheme = g_file_get_uri_scheme (fixture->file);
  gt_expect_string (scheme, NOT TO_BE_NULL);
  g_free (scheme);
}

GENERATE_IO_METHOD_TESTS (read);
GENERATE_IO_METHOD_TESTS (append_to, G_FILE_CREATE_NONE);
GENERATE_IO_METHOD_TESTS (create, G_FILE_CREATE_NONE);
GENERATE_IO_METHOD_TESTS (replace, NULL, FALSE, G_FILE_CREATE_NONE);
GENERATE_IO_METHOD_TESTS (query_info, "*", G_FILE_QUERY_INFO_NONE);

static void
test_query_exists (Fixture      *fixture,
                   gconstpointer unused)
{
  g_file_query_exists (fixture->file, NULL);
}

static void
test_query_file_type (Fixture      *fixture,
                      gconstpointer unused)
{
  g_file_query_file_type (fixture->file, G_FILE_QUERY_INFO_NONE, NULL);
}

GENERATE_IO_METHOD_TESTS (query_filesystem_info, "*");
GENERATE_IO_METHOD_TEST_SYNC (query_default_handler);

static void
measure_disk_usage_progress_callback (gboolean reporting,
                                      guint64  current_size,
                                      guint64  num_dirs,
                                      guint64  num_files,
                                      Fixture *fixture)
{
  fixture->measure_disk_usage_progress_callback_called = TRUE;
  if (reporting)
    {
      gt_expect_uint (current_size, TO_EQUAL, 0);
      gt_expect_uint (num_dirs, TO_EQUAL, 0);
      gt_expect_uint (num_files, TO_EQUAL, 0);
    }
}

static void
test_measure_disk_usage (Fixture      *fixture,
                         gconstpointer unused)
{
  GError *error = NULL;
  gboolean success = g_file_measure_disk_usage (fixture->file, G_FILE_MEASURE_NONE, NULL,
                                                (GFileMeasureProgressCallback)measure_disk_usage_progress_callback, fixture,
                                                NULL, NULL, NULL, /* out params */
                                                &error);
  assert_success_or_free_error (success, error);
  if (success)
    gt_expect_bool (fixture->measure_disk_usage_progress_callback_called, TO_BE_TRUE);
}

static void
on_async_measure_disk_usage_assert (GObject      *source,
                                    GAsyncResult *res,
                                    Fixture      *fixture)
{
  GError *error = NULL;
  gboolean success = g_file_measure_disk_usage_finish (fixture->file, res,
                                                       NULL, NULL, NULL, /* out params */
                                                       &error);
  assert_success_or_free_error (success, error);
  if (success)
    gt_expect_bool (fixture->measure_disk_usage_progress_callback_called, TO_BE_TRUE);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_measure_disk_usage_async (Fixture      *fixture,
                               gconstpointer unused)
{
  g_file_measure_disk_usage_async (fixture->file, G_FILE_MEASURE_NONE, G_PRIORITY_DEFAULT, NULL,
                                   (GFileMeasureProgressCallback)measure_disk_usage_progress_callback, fixture,
                                   (GAsyncReadyCallback)on_async_measure_disk_usage_assert, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
on_async_measure_disk_usage_assert_cancelled (GObject      *source,
                                              GAsyncResult *res,
                                              Fixture      *fixture)
{
  GError *error = NULL;
  gboolean success = g_file_measure_disk_usage_finish (fixture->file, res,
                                                       NULL, NULL, NULL, /* out params */
                                                       &error);
  gt_expect_bool (success, TO_BE_FALSE);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_error_free (error);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_measure_disk_usage_cancel (Fixture      *fixture,
                                gconstpointer unused)
{
  g_file_measure_disk_usage_async (fixture->file, G_FILE_MEASURE_NONE, G_PRIORITY_DEFAULT, fixture->cancellable,
                                   NULL, NULL, /* progress callback */
                                   (GAsyncReadyCallback)on_async_measure_disk_usage_assert_cancelled, fixture);
  g_cancellable_cancel (fixture->cancellable);
  g_main_loop_run (fixture->mainloop);
}

GENERATE_IO_METHOD_TESTS (find_enclosing_mount);
GENERATE_IO_METHOD_TESTS (enumerate_children, "*", G_FILE_QUERY_INFO_NONE);
GENERATE_IO_METHOD_TESTS (set_display_name, "foobar");
GENERATE_IO_METHOD_TESTS_BOOL (delete);
GENERATE_IO_METHOD_TESTS_BOOL (trash);
GENERATE_COPY_METHOD_TEST_SYNC (copy);

static void
on_async_copy_assert (GObject      *source,
                      GAsyncResult *res,
                      Fixture      *fixture)
{
  GError *error = NULL;
  gboolean success = g_file_copy_finish (fixture->file, res, &error);
  assert_success_or_free_error (success, error);
  if (success)
    {
      gt_expect_bool (fixture->copy_progress_callback_called, TO_BE_TRUE);
      gt_expect_bool (fixture->copy_progress_callback_called_complete, TO_BE_TRUE);
    }
  g_main_loop_quit (fixture->mainloop);
}

static void
test_copy_async (Fixture      *fixture,
                 gconstpointer unused)
{
  GFile *file2 = G_FILE (gt_mock_file_new ());
  g_file_copy_async (fixture->file, file2, G_FILE_COPY_NONE, G_PRIORITY_DEFAULT, NULL,
                     (GFileProgressCallback)copy_progress_callback, fixture,
                     (GAsyncReadyCallback)on_async_copy_assert, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
on_async_copy_assert_cancelled (GObject      *source,
                                GAsyncResult *res,
                                Fixture      *fixture)
{
  GError *error = NULL;
  gboolean success = g_file_copy_finish (fixture->file, res, &error);
  gt_expect_bool (success, TO_BE_FALSE);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_error_free (error);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_copy_cancel (Fixture      *fixture,
                  gconstpointer unused)
{
  GFile *file2 = G_FILE (gt_mock_file_new ());
  g_file_copy_async (fixture->file, file2, G_FILE_COPY_NONE, G_PRIORITY_DEFAULT, fixture->cancellable,
                     NULL, NULL, /* progress callback */
                     (GAsyncReadyCallback)on_async_copy_assert_cancelled, fixture);
  g_cancellable_cancel (fixture->cancellable);
  g_main_loop_run (fixture->mainloop);
}

GENERATE_COPY_METHOD_TEST_SYNC (move);
GENERATE_IO_METHOD_TESTS_BOOL (make_directory);
GENERATE_IO_METHOD_TEST_SYNC_BOOL (make_directory_with_parents);
GENERATE_IO_METHOD_TEST_SYNC_BOOL (make_symbolic_link, "foobar");
GENERATE_ATTRIBUTE_LIST_METHOD_TEST_SYNC (query_settable_attributes);
GENERATE_ATTRIBUTE_LIST_METHOD_TEST_SYNC (query_writable_namespaces);
GENERATE_IO_METHOD_TEST_SYNC_BOOL (set_attribute, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_ATTRIBUTE_TYPE_STRING, "foobar", G_FILE_QUERY_INFO_NONE);

static void
test_set_attributes_from_info (Fixture      *fixture,
                               gconstpointer unused)
{
  GError *error = NULL;
  GFileInfo *info = g_file_info_new ();
  gboolean success = g_file_set_attributes_from_info (fixture->file, info, G_FILE_QUERY_INFO_NONE, NULL, &error);
  g_object_unref (info);
  assert_success_or_free_error (success, error);
}

static void
on_async_set_attributes_assert (GObject      *source,
                                GAsyncResult *res,
                                gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  GFileInfo *info = NULL;
  gboolean success = g_file_set_attributes_finish (fixture->file, res, &info, &error);
  assert_success_or_free_error (success, error);
  /* TODO: assert that no fields have G_FILE_ATTRIBUTE_STATUS_ERROR using
  g_file_info_get_attribute_status()? */
  g_object_unref (info);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_set_attributes_async (Fixture      *fixture,
                           gconstpointer unused)
{
  GFileInfo *info = g_file_info_new ();
  g_file_set_attributes_async (fixture->file, info, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, NULL,
                               on_async_set_attributes_assert, fixture);
  g_object_unref (info);
  g_main_loop_run (fixture->mainloop);
}

static void
on_async_set_attributes_assert_cancelled (GObject      *source,
                                          GAsyncResult *res,
                                          gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  GFileInfo *info = NULL;
  gboolean success = g_file_set_attributes_finish (fixture->file, res, &info, &error);
  gt_expect_bool (success, TO_BE_FALSE);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_error_free (error);
  g_object_unref (info);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_set_attributes_cancel (Fixture      *fixture,
                            gconstpointer unused)
{
  GFileInfo *info = g_file_info_new ();
  g_file_set_attributes_async (fixture->file, info, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, fixture->cancellable,
                               on_async_set_attributes_assert_cancelled, fixture);
  g_cancellable_cancel (fixture->cancellable);
  g_object_unref (info);
  g_main_loop_run (fixture->mainloop);
}

/* TODO: try setting attribute of each type? */

static void
test_mount_mountable (Fixture      *fixture,
                      gconstpointer unused)
{
  fixture->finish_func = (FileFinishFunc)g_file_mount_mountable_finish;
  g_file_mount_mountable (fixture->file, G_MOUNT_MOUNT_NONE, NULL, NULL,
                          on_async_assert_object, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
test_mount_mountable_cancel (Fixture      *fixture,
                             gconstpointer unused)
{
  fixture->finish_func = (FileFinishFunc)g_file_mount_mountable_finish;
  /* mount methods don't necessarily start the main loop before checking for
  cancellation */
  g_cancellable_cancel (fixture->cancellable);
  g_file_mount_mountable (fixture->file, G_MOUNT_MOUNT_NONE, NULL, fixture->cancellable,
                          on_async_assert_cancelled, fixture);
  g_main_loop_run (fixture->mainloop);
}

GENERATE_MOUNT_METHOD_TESTS (unmount_mountable_with_operation, G_MOUNT_UNMOUNT_NONE);
GENERATE_MOUNT_METHOD_TESTS (eject_mountable_with_operation, G_MOUNT_UNMOUNT_NONE);
GENERATE_MOUNT_METHOD_TESTS (start_mountable, G_DRIVE_START_NONE);
GENERATE_MOUNT_METHOD_TESTS (stop_mountable, G_MOUNT_UNMOUNT_NONE);

static void
test_poll_mountable (Fixture      *fixture,
                     gconstpointer unused)
{
  fixture->finish_func = (FileFinishFunc)g_file_poll_mountable_finish;
  g_file_poll_mountable (fixture->file, NULL,
                         on_async_assert_bool, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
test_poll_mountable_cancel (Fixture      *fixture,
                            gconstpointer unused)
{
  fixture->finish_func = (FileFinishFunc)g_file_poll_mountable_finish;
  /* mount methods don't necessarily start the main loop before checking for
  cancellation */
  g_cancellable_cancel (fixture->cancellable);
  g_file_poll_mountable (fixture->file, fixture->cancellable,
                         on_async_assert_bool_cancelled, fixture);
  g_main_loop_run (fixture->mainloop);
}

GENERATE_MOUNT_METHOD_TESTS (mount_enclosing_volume, G_MOUNT_MOUNT_NONE);
GENERATE_IO_METHOD_TEST_SYNC (monitor_directory, G_FILE_MONITOR_NONE);
GENERATE_IO_METHOD_TEST_SYNC (monitor_file, G_FILE_MONITOR_NONE);
GENERATE_IO_METHOD_TEST_SYNC (monitor, G_FILE_MONITOR_NONE);

static void
test_load_contents (Fixture      *fixture,
                    gconstpointer unused)
{
  char *contents = NULL;
  gsize length = 0;
  char *etag = NULL;
  GError *error = NULL;
  gboolean success = g_file_load_contents (fixture->file, NULL,
                                           &contents, &length, &etag,
                                           &error);
  assert_success_or_free_error (success, error);
  if (success)
    {
      gt_expect_string (contents, NOT TO_BE_NULL);
      gt_expect_int (length, NOT TO_BE_LESS_THAN, strlen (contents));
      gt_expect_string (etag, NOT TO_BE_NULL);
      g_free (contents);
      g_free (etag);
    }
}

static void
on_async_load_contents_assert (GObject      *source,
                               GAsyncResult *res,
                               gpointer     user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  char *contents = NULL;
  gsize length = 0;
  char *etag = NULL;
  GError *error = NULL;
  gboolean success = g_file_load_contents_finish (fixture->file, res,
                                                  &contents, &length, &etag,
                                                  &error);
  assert_success_or_free_error (success, error);
  if (success)
    {
      gt_expect_string (contents, NOT TO_BE_NULL);
      gt_expect_int (length, NOT TO_BE_LESS_THAN, strlen (contents));
      gt_expect_string (etag, NOT TO_BE_NULL);
      g_free (contents);
      g_free (etag);
    }
  g_main_loop_quit (fixture->mainloop);
}

static void
test_load_contents_async (Fixture      *fixture,
                          gconstpointer unused)
{
  g_file_load_contents_async (fixture->file, NULL,
                              on_async_load_contents_assert, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
on_async_load_contents_assert_cancelled (GObject      *source,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  char *contents = NULL;
  gboolean success = g_file_load_contents_finish (fixture->file, res,
                                                  &contents, NULL, NULL, /* out params */
                                                  &error);
  g_free (contents);
  gt_expect_bool (success, TO_BE_FALSE);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_error_free (error);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_load_contents_cancel (Fixture      *fixture,
                           gconstpointer unused)
{
  g_file_load_contents_async (fixture->file, fixture->cancellable,
                              on_async_load_contents_assert_cancelled, fixture);
  g_cancellable_cancel (fixture->cancellable);
  g_main_loop_run (fixture->mainloop);
}

static gboolean
on_load_partial_contents_read_all (const char *file_contents,
                                   goffset     file_size,
                                   gpointer    user_data)
{
  gt_expect_string (file_contents, NOT TO_BE_NULL);
  gt_expect_int (file_size, NOT TO_BE_LESS_THAN, strlen (file_contents));
  return TRUE;
}

static void
test_load_partial_contents_async_read_all (Fixture      *fixture,
                                           gconstpointer unused)
{
  g_file_load_partial_contents_async (fixture->file, NULL,
                                      on_load_partial_contents_read_all,
                                      on_async_load_contents_assert, fixture);
  g_main_loop_run (fixture->mainloop);
}

static gboolean
on_load_partial_contents_read_some (const char *file_contents,
                                    goffset     file_size,
                                    gpointer    user_data)
{
  gt_expect_string (file_contents, NOT TO_BE_NULL);
  gt_expect_int (file_size, NOT TO_BE_LESS_THAN, strlen (file_contents));
  return FALSE;
}

static void
test_load_partial_contents_async_read_some (Fixture      *fixture,
                                            gconstpointer unused)
{
  g_file_load_partial_contents_async (fixture->file, NULL,
                                      on_load_partial_contents_read_some,
                                      on_async_load_contents_assert, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
test_load_partial_contents_async_cancel (Fixture      *fixture,
                                         gconstpointer unused)
{
  g_file_load_partial_contents_async (fixture->file, fixture->cancellable,
                                      NULL, /* read more callback */
                                      on_async_load_contents_assert_cancelled, fixture);
  g_cancellable_cancel (fixture->cancellable);
  g_main_loop_run (fixture->mainloop);
}

static void
test_replace_contents (Fixture      *fixture,
                       gconstpointer unused)
{
  char *etag = NULL;
  GError *error = NULL;
  gboolean success = g_file_replace_contents (fixture->file,
                                              "foobar", strlen ("foobar"),
                                              NULL, /* etag */ FALSE, /* make backup */ G_FILE_CREATE_NONE, &etag, NULL, &error);
  assert_success_or_free_error (success, error);
  if (success)
    {
      gt_expect_string (etag, NOT TO_BE_NULL);
      g_free (etag);
    }
}

static void
on_async_replace_contents_assert (GObject      *source,
                                  GAsyncResult *res,
                                  gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  char *etag = NULL;
  GError *error = NULL;
  gboolean success = g_file_replace_contents_finish (fixture->file, res,
                                                     &etag, &error);
  assert_success_or_free_error (success, error);
  if (success)
    {
      gt_expect_string (etag, NOT TO_BE_NULL);
      g_free (etag);
    }
  g_main_loop_quit (fixture->mainloop);
}

static void
test_replace_contents_async (Fixture      *fixture,
                             gconstpointer unused)
{
  g_file_replace_contents_async (fixture->file, "foobar", strlen ("foobar"),
                                 NULL, /* etag */ FALSE, /* make backup */
                                 G_FILE_CREATE_NONE, NULL,
                                 on_async_replace_contents_assert, fixture);
  g_main_loop_run (fixture->mainloop);
}

static void
on_async_replace_contents_assert_cancelled (GObject      *source,
                                            GAsyncResult *res,
                                            gpointer      user_data)
{
  Fixture *fixture = (Fixture *)user_data;
  GError *error = NULL;
  gboolean success = g_file_replace_contents_finish (fixture->file, res,
                                                     NULL, /* out param */
                                                     &error);
  gt_expect_bool (success, TO_BE_FALSE);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_error_free (error);
  g_main_loop_quit (fixture->mainloop);
}

static void
test_replace_contents_cancel (Fixture      *fixture,
                              gconstpointer unused)
{
  g_file_replace_contents_async (fixture->file, "foobar", strlen ("foobar"),
                                 NULL, /* etag */ FALSE, /* make backup */
                                 G_FILE_CREATE_NONE, fixture->cancellable,
                                 on_async_replace_contents_assert_cancelled, fixture);
  g_cancellable_cancel (fixture->cancellable);
  g_main_loop_run (fixture->mainloop);
}

static void
test_replace_contents_bytes_async (Fixture      *fixture,
                                   gconstpointer unused)
{
  GBytes *new_contents = g_bytes_new ("foobar", strlen ("foobar"));
  g_file_replace_contents_bytes_async (fixture->file, new_contents,
                                       NULL, /* etag */ FALSE, /* make backup */
                                       G_FILE_CREATE_NONE, NULL,
                                       on_async_replace_contents_assert, fixture);
  g_bytes_unref (new_contents);
  g_main_loop_run (fixture->mainloop);
}

static void
test_replace_contents_bytes_async_cancel (Fixture      *fixture,
                                          gconstpointer unused)
{
  GBytes *new_contents = g_bytes_new ("foobar", strlen ("foobar"));
  g_file_replace_contents_bytes_async (fixture->file, new_contents,
                                       NULL, /* etag */ FALSE, /* make backup */
                                       G_FILE_CREATE_NONE, fixture->cancellable,
                                       on_async_replace_contents_assert_cancelled, fixture);
  g_bytes_unref (new_contents);
  g_cancellable_cancel (fixture->cancellable);
  g_main_loop_run (fixture->mainloop);
}

static void
test_copy_attributes (Fixture      *fixture,
                      gconstpointer unused)
{
  GError *error = NULL;
  GFile *file2 = G_FILE (gt_mock_file_new ());
  gboolean success = g_file_copy_attributes (fixture->file, file2, G_FILE_COPY_NONE,
                                             NULL, &error);
  assert_success_or_free_error (success, error);
  g_object_unref (file2);
}

GENERATE_IO_METHOD_TESTS (create_readwrite, G_FILE_CREATE_NONE);
GENERATE_IO_METHOD_TESTS (open_readwrite);
GENERATE_IO_METHOD_TESTS (replace_readwrite, NULL, FALSE, G_FILE_CREATE_NONE);

static void
test_supports_thread_contexts (Fixture      *fixture,
                               gconstpointer unused)
{
  g_file_supports_thread_contexts (fixture->file);
}

static void
test_new_for_uri (Fixture      *fixture,
                  gconstpointer unused)
{
  char *uri = g_file_get_uri (fixture->file);
  GFile *new_file = g_file_new_for_uri (uri);
  g_free (uri);
  gt_expect_file (fixture->file, TO_EQUAL, new_file);
  g_object_unref (new_file);
}

static void
test_parse_name (Fixture      *fixture,
                 gconstpointer unused)
{
  char *parse_name = g_file_get_parse_name (fixture->file);
  GFile *new_file = g_file_parse_name (parse_name);
  g_free (parse_name);
  gt_expect_file (fixture->file, TO_EQUAL, new_file);
  g_object_unref (new_file);
}

static void
test_child_parent (Fixture      *fixture,
                   gconstpointer unused)
{
  GFile *child = g_file_get_child (fixture->file, "foobar");
  GFile *new_file = g_file_get_parent (child);
  g_object_unref (child);
  gt_expect_file (fixture->file, TO_EQUAL, new_file);
  g_object_unref (new_file);
}

static void
test_file_exists_if_created_as_such (void)
{
  GFile *file = G_FILE (g_object_new (GT_TYPE_MOCK_FILE,
                                      "exists", TRUE,
                                      NULL));
  gt_expect_file (file, TO_EXIST);
  g_object_unref (file);
}

static void
test_file_does_not_exist_if_not_created_as_such (Fixture      *fixture,
                                                 gconstpointer unused)
{
  gt_expect_file (fixture->file, NOT TO_EXIST);
}

static void
test_file_create (Fixture      *fixture,
                  gconstpointer unused)
{
  GError *error = NULL;
  GFileOutputStream *stream = g_file_create (fixture->file, G_FILE_CREATE_NONE,
                                             NULL, &error);
  gt_expect_error (error, TO_BE_CLEAR);
  gt_expect_object (stream, NOT TO_BE_NULL);

  int length = strlen (SAMPLE_UTF8_CONTENTS);
  ssize_t written = g_output_stream_write (G_OUTPUT_STREAM (stream),
                                           SAMPLE_UTF8_CONTENTS, length,
                                           NULL, &error);
  g_object_unref (stream);
  gt_expect_error (error, TO_BE_CLEAR);
  gt_expect_int (written, TO_EQUAL, length);

  const char *contents = gt_mock_file_get_contents_utf8 (GT_MOCK_FILE (fixture->file));
  gt_expect_string (contents, TO_EQUAL, SAMPLE_UTF8_CONTENTS);
}

static void
test_file_create_fails_if_file_exists (void)
{
  GFile *file = G_FILE (g_object_new (GT_TYPE_MOCK_FILE,
                                      "exists", TRUE,
                                      NULL));

  GError *error = NULL;
  GFileOutputStream *stream = g_file_create (file, G_FILE_CREATE_NONE, NULL,
                                             &error);
  gt_expect_object (stream, TO_BE_NULL);
  gt_expect_error (error, TO_MATCH, G_IO_ERROR, G_IO_ERROR_EXISTS);

  g_error_free (error);

  g_object_unref (file);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

#define ADD_MOCK_FILE_TEST(path, test_func) \
  g_test_add ((path), Fixture, NULL, setup, (test_func), teardown)

  /* Ensure GtMockFile adheres to the minimum API contract of GFile */
  ADD_MOCK_FILE_TEST ("/api/dup", test_dup);
  ADD_MOCK_FILE_TEST ("/api/hash", test_hash);
  ADD_MOCK_FILE_TEST ("/api/equal/yes", test_equal_yes);
  ADD_MOCK_FILE_TEST ("/api/equal/no", test_equal_no);
  ADD_MOCK_FILE_TEST ("/api/get-basename", test_get_basename);
  ADD_MOCK_FILE_TEST ("/api/get-path", test_get_path);
  ADD_MOCK_FILE_TEST ("/api/get-uri", test_get_uri);
  ADD_MOCK_FILE_TEST ("/api/get-parse-name", test_get_parse_name);
  ADD_MOCK_FILE_TEST ("/api/get-parent", test_get_parent);
  ADD_MOCK_FILE_TEST ("/api/has-parent/any", test_has_any_parent);
  ADD_MOCK_FILE_TEST ("/api/has-parent/no", test_has_parent_no);
  ADD_MOCK_FILE_TEST ("/api/get-child", test_get_child);
  ADD_MOCK_FILE_TEST ("/api/get-child-for-display-name", test_get_child_for_display_name);
  ADD_MOCK_FILE_TEST ("/api/has-prefix/not-related", test_has_prefix_not_related);
  ADD_MOCK_FILE_TEST ("/api/has-prefix/same-file", test_has_prefix_same_file);
  ADD_MOCK_FILE_TEST ("/api/has-prefix/child", test_has_prefix_child);
  ADD_MOCK_FILE_TEST ("/api/has-prefix/descendant", test_has_prefix_descendant);
  ADD_MOCK_FILE_TEST ("/api/has-prefix/parent", test_has_prefix_parent);
  ADD_MOCK_FILE_TEST ("/api/has-prefix/ancestor", test_has_prefix_ancestor);
  ADD_MOCK_FILE_TEST ("/api/get-relative-path/not-related", test_get_relative_path_not_related);
  ADD_MOCK_FILE_TEST ("/api/get-relative-path/same-file", test_get_relative_path_same_file);
  ADD_MOCK_FILE_TEST ("/api/get-relative-path/child", test_get_relative_path_child);
  ADD_MOCK_FILE_TEST ("/api/get-relative-path/descendant", test_get_relative_path_descendant);
  ADD_MOCK_FILE_TEST ("/api/get-relative-path/parent", test_get_relative_path_parent);
  ADD_MOCK_FILE_TEST ("/api/get-relative-path/ancestor", test_get_relative_path_ancestor);
  ADD_MOCK_FILE_TEST ("/api/resolve-relative-path/same-file", test_resolve_relative_path_same_file);
  ADD_MOCK_FILE_TEST ("/api/resolve-relative-path/parent", test_resolve_relative_path_parent);
  ADD_MOCK_FILE_TEST ("/api/resolve-relative-path/child", test_resolve_relative_path_child);
  ADD_MOCK_FILE_TEST ("/api/is-native", test_is_native);
  ADD_MOCK_FILE_TEST ("/api/has-uri-scheme", test_has_uri_scheme);
  ADD_MOCK_FILE_TEST ("/api/get-uri-scheme", test_get_uri_scheme);
  ADD_MOCK_FILE_TEST ("/api/read", test_read);
  ADD_MOCK_FILE_TEST ("/api/read-async", test_read_async);
  ADD_MOCK_FILE_TEST ("/api/read-cancel", test_read_cancel);
  ADD_MOCK_FILE_TEST ("/api/append-to", test_append_to);
  ADD_MOCK_FILE_TEST ("/api/create", test_create);
  ADD_MOCK_FILE_TEST ("/api/replace", test_replace);
  ADD_MOCK_FILE_TEST ("/api/append-to-async", test_append_to_async);
  ADD_MOCK_FILE_TEST ("/api/append-to-cancel", test_append_to_cancel);
  ADD_MOCK_FILE_TEST ("/api/create-async", test_create_async);
  ADD_MOCK_FILE_TEST ("/api/create-cancel", test_create_cancel);
  ADD_MOCK_FILE_TEST ("/api/replace-async", test_replace_async);
  ADD_MOCK_FILE_TEST ("/api/replace-cancel", test_replace_cancel);
  ADD_MOCK_FILE_TEST ("/api/query-info", test_query_info);
  ADD_MOCK_FILE_TEST ("/api/query-info-async", test_query_info_async);
  ADD_MOCK_FILE_TEST ("/api/query-info-cancel", test_query_info_cancel);
  ADD_MOCK_FILE_TEST ("/api/query-exists", test_query_exists);
  ADD_MOCK_FILE_TEST ("/api/query-file-type", test_query_file_type);
  ADD_MOCK_FILE_TEST ("/api/query-filesystem-info", test_query_filesystem_info);
  ADD_MOCK_FILE_TEST ("/api/query-filesystem-info-async", test_query_filesystem_info_async);
  ADD_MOCK_FILE_TEST ("/api/query-filesystem-info-cancel", test_query_filesystem_info_cancel);
  ADD_MOCK_FILE_TEST ("/api/query-default-handler", test_query_default_handler);
  ADD_MOCK_FILE_TEST ("/api/measure-disk-usage", test_measure_disk_usage);
  ADD_MOCK_FILE_TEST ("/api/measure-disk-usage-async", test_measure_disk_usage_async);
  ADD_MOCK_FILE_TEST ("/api/measure-disk-usage-cancel", test_measure_disk_usage_cancel);
  ADD_MOCK_FILE_TEST ("/api/find-enclosing-mount", test_find_enclosing_mount);
  ADD_MOCK_FILE_TEST ("/api/find-enclosing-mount-async", test_find_enclosing_mount_async);
  ADD_MOCK_FILE_TEST ("/api/find-enclosing-mount-cancel", test_find_enclosing_mount_cancel);
  ADD_MOCK_FILE_TEST ("/api/enumerate-children", test_enumerate_children);
  ADD_MOCK_FILE_TEST ("/api/enumerate-children-async", test_enumerate_children_async);
  ADD_MOCK_FILE_TEST ("/api/enumerate-children-cancel", test_enumerate_children_cancel);
  ADD_MOCK_FILE_TEST ("/api/set-display-name", test_set_display_name);
  ADD_MOCK_FILE_TEST ("/api/set-display-name-async", test_set_display_name_async);
  ADD_MOCK_FILE_TEST ("/api/set-display-name-cancel", test_set_display_name_cancel);
  ADD_MOCK_FILE_TEST ("/api/delete", test_delete);
  ADD_MOCK_FILE_TEST ("/api/delete-async", test_delete_async);
  ADD_MOCK_FILE_TEST ("/api/delete-cancel", test_delete_cancel);
  ADD_MOCK_FILE_TEST ("/api/trash", test_trash);
  ADD_MOCK_FILE_TEST ("/api/trash-async", test_trash_async);
  ADD_MOCK_FILE_TEST ("/api/trash-cancel", test_trash_cancel);
  ADD_MOCK_FILE_TEST ("/api/copy", test_copy);
  ADD_MOCK_FILE_TEST ("/api/copy-async", test_copy_async);
  ADD_MOCK_FILE_TEST ("/api/copy-cancel", test_copy_cancel);
  ADD_MOCK_FILE_TEST ("/api/move", test_move);
  ADD_MOCK_FILE_TEST ("/api/make-directory", test_make_directory);
  ADD_MOCK_FILE_TEST ("/api/make-directory-async", test_make_directory_async);
  ADD_MOCK_FILE_TEST ("/api/make-directory-cancel", test_make_directory_cancel);
  ADD_MOCK_FILE_TEST ("/api/make-directory-with-parents", test_make_directory_with_parents);
  ADD_MOCK_FILE_TEST ("/api/make-symbolic-link", test_make_symbolic_link);
  ADD_MOCK_FILE_TEST ("/api/query-settable-attributes", test_query_settable_attributes);
  ADD_MOCK_FILE_TEST ("/api/query-writable-namespaces", test_query_writable_namespaces);
  ADD_MOCK_FILE_TEST ("/api/set-attribute", test_set_attribute);
  ADD_MOCK_FILE_TEST ("/api/set-attributes-from-info", test_set_attributes_from_info);
  ADD_MOCK_FILE_TEST ("/api/set-attributes-async", test_set_attributes_async);
  ADD_MOCK_FILE_TEST ("/api/set-attributes-cancel", test_set_attributes_cancel);
  ADD_MOCK_FILE_TEST ("/api/mount-mountable", test_mount_mountable);
  ADD_MOCK_FILE_TEST ("/api/mount-mountable-cancel", test_mount_mountable_cancel);
  ADD_MOCK_FILE_TEST ("/api/unmount-mountable-with-operation", test_unmount_mountable_with_operation);
  ADD_MOCK_FILE_TEST ("/api/unmount-mountable-with-operation-cancel", test_unmount_mountable_with_operation_cancel);
  ADD_MOCK_FILE_TEST ("/api/eject-mountable-with-operation", test_eject_mountable_with_operation);
  ADD_MOCK_FILE_TEST ("/api/eject-mountable-with-operation-cancel", test_eject_mountable_with_operation_cancel);
  ADD_MOCK_FILE_TEST ("/api/start-mountable", test_start_mountable);
  ADD_MOCK_FILE_TEST ("/api/start-mountable-cancel", test_start_mountable_cancel);
  ADD_MOCK_FILE_TEST ("/api/stop-mountable", test_stop_mountable);
  ADD_MOCK_FILE_TEST ("/api/stop-mountable-cancel", test_stop_mountable_cancel);
  ADD_MOCK_FILE_TEST ("/api/poll-mountable", test_poll_mountable);
  ADD_MOCK_FILE_TEST ("/api/poll-mountable-cancel", test_poll_mountable_cancel);
  ADD_MOCK_FILE_TEST ("/api/mount-enclosing-volume", test_mount_enclosing_volume);
  ADD_MOCK_FILE_TEST ("/api/mount-enclosing-volume-cancel", test_mount_enclosing_volume_cancel);
  ADD_MOCK_FILE_TEST ("/api/monitor-directory", test_monitor_directory);
  ADD_MOCK_FILE_TEST ("/api/monitor-file", test_monitor_file);
  ADD_MOCK_FILE_TEST ("/api/monitor", test_monitor);
  ADD_MOCK_FILE_TEST ("/api/load-contents", test_load_contents);
  ADD_MOCK_FILE_TEST ("/api/load-contents-async", test_load_contents_async);
  ADD_MOCK_FILE_TEST ("/api/load-contents-cancel", test_load_contents_cancel);
  ADD_MOCK_FILE_TEST ("/api/load-partial-contents-async/read-all", test_load_partial_contents_async_read_all);
  ADD_MOCK_FILE_TEST ("/api/load-partial-contents-async/read-some", test_load_partial_contents_async_read_some);
  ADD_MOCK_FILE_TEST ("/api/load-partial-contents-async/cancel", test_load_partial_contents_async_cancel);
  ADD_MOCK_FILE_TEST ("/api/replace-contents", test_replace_contents);
  ADD_MOCK_FILE_TEST ("/api/replace-contents-async", test_replace_contents_async);
  ADD_MOCK_FILE_TEST ("/api/replace-contents-cancel", test_replace_contents_cancel);
  ADD_MOCK_FILE_TEST ("/api/replace-contents-bytes-async", test_replace_contents_bytes_async);
  ADD_MOCK_FILE_TEST ("/api/replace-contents-bytes-async-cancel", test_replace_contents_bytes_async_cancel);
  ADD_MOCK_FILE_TEST ("/api/copy-attributes", test_copy_attributes);
  ADD_MOCK_FILE_TEST ("/api/create-readwrite", test_create_readwrite);
  ADD_MOCK_FILE_TEST ("/api/create-readwrite-async", test_create_readwrite_async);
  ADD_MOCK_FILE_TEST ("/api/create-readwrite-cancel", test_create_readwrite_cancel);
  ADD_MOCK_FILE_TEST ("/api/open-readwrite", test_open_readwrite);
  ADD_MOCK_FILE_TEST ("/api/open-readwrite-async", test_open_readwrite_async);
  ADD_MOCK_FILE_TEST ("/api/open-readwrite-cancel", test_open_readwrite_cancel);
  ADD_MOCK_FILE_TEST ("/api/replace-readwrite", test_replace_readwrite);
  ADD_MOCK_FILE_TEST ("/api/replace-readwrite-async", test_replace_readwrite_async);
  ADD_MOCK_FILE_TEST ("/api/replace-readwrite-cancel", test_replace_readwrite_cancel);
  ADD_MOCK_FILE_TEST ("/api/supports-thread-contexts", test_supports_thread_contexts);

  /* Round-trip with various methods of creating a GtMockFile */
  ADD_MOCK_FILE_TEST ("/round-trip/new-for-uri", test_new_for_uri);
  ADD_MOCK_FILE_TEST ("/round-trip/parse-name", test_parse_name);
  ADD_MOCK_FILE_TEST ("/round-trip/child-parent", test_child_parent);

  /* Tests for supported GFile functionality */
  g_test_add_func ("/file/exists-if-created-as-such",
                   test_file_exists_if_created_as_such);
  ADD_MOCK_FILE_TEST ("/file/does-not-exist-if-not-created-as-such",
                      test_file_does_not_exist_if_not_created_as_such);
  ADD_MOCK_FILE_TEST ("/file/create", test_file_create);
  g_test_add_func ("/file/create-fails-if-file-exists",
                   test_file_create_fails_if_file_exists);

#undef ADD_MOCK_FILE_TEST

  return g_test_run ();
}