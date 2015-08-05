/*
 * Copyright 2015 Philip Chimento <philip.chimento@gmail.com>
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

#include "gt.h"
#include "gt-expect.h"

void
test_bool_to_equal (void)
{
  gt_expect_bool (TRUE, TO_EQUAL, TRUE);
  gt_expect_bool (FALSE, TO_EQUAL, FALSE);
}

void
test_bool_not_to_equal (void)
{
  gt_expect_bool (TRUE, NOT TO_EQUAL, FALSE);
  gt_expect_bool (FALSE, NOT TO_EQUAL, TRUE);
}

void
test_bool_to_be_true (void)
{
  gt_expect_bool (TRUE, TO_BE_TRUE);
}

void
test_bool_not_to_be_true (void)
{
  gt_expect_bool (FALSE, NOT TO_BE_TRUE);
}

void
test_bool_to_be_false (void)
{
  gt_expect_bool (FALSE, TO_BE_FALSE);
}

void
test_bool_not_to_be_false (void)
{
  gt_expect_bool (TRUE, NOT TO_BE_FALSE);
}

void
test_int_to_equal (void)
{
  gt_expect_int (5, TO_EQUAL, 5);
}

void
test_int_not_to_equal (void)
{
  gt_expect_int (5, NOT TO_EQUAL, 3);
}

void
test_int_to_be_less_than (void)
{
  gt_expect_int (3, TO_BE_LESS_THAN, 5);
}

void
test_int_not_to_be_less_than (void)
{
  gt_expect_int (5, NOT TO_BE_LESS_THAN, 3);
  gt_expect_int (5, NOT TO_BE_LESS_THAN, 5);
}

void
test_uint_to_equal (void)
{
  gt_expect_uint (5u, TO_EQUAL, 5u);
}

void
test_uint_not_to_equal (void)
{
  gt_expect_uint (5u, NOT TO_EQUAL, 3u);
}

void
test_uint_to_be_less_than (void)
{
  gt_expect_uint (3u, TO_BE_LESS_THAN, 5u);
}

void
test_uint_not_to_be_less_than (void)
{
  gt_expect_uint (5u, NOT TO_BE_LESS_THAN, 3u);
  gt_expect_uint (5u, NOT TO_BE_LESS_THAN, 5u);
}

void
test_string_to_be_null (void)
{
  gt_expect_string (NULL, TO_BE_NULL);
}

void
test_string_not_to_be_null (void)
{
  gt_expect_string ("", NOT TO_BE_NULL);
  gt_expect_string ("foobar", NOT TO_BE_NULL);
}

void
test_string_to_equal (void)
{
  gt_expect_string (NULL, TO_EQUAL, NULL);
  gt_expect_string ("", TO_EQUAL, "");
  gt_expect_string ("foobar", TO_EQUAL, "foobar");
}

void
test_string_not_to_equal (void)
{
  gt_expect_string ("foo", NOT TO_EQUAL, "bar");
  gt_expect_string (NULL, NOT TO_EQUAL, "foobar");
  gt_expect_string (NULL, NOT TO_EQUAL, "");
  gt_expect_string ("", NOT TO_EQUAL, NULL);
  gt_expect_string ("", NOT TO_EQUAL, "foobar");
  gt_expect_string ("foobar", NOT TO_EQUAL, NULL);
  gt_expect_string ("foobar", NOT TO_EQUAL, "");
}

void
test_string_to_be_less_than (void)
{
  gt_expect_string ("bar", TO_BE_LESS_THAN, "foo");
  gt_expect_string (NULL, TO_BE_LESS_THAN, "foobar");
  gt_expect_string ("", TO_BE_LESS_THAN, "foobar");
  gt_expect_string (NULL, TO_BE_LESS_THAN, "");
}

void
test_string_not_to_be_less_than (void)
{
  gt_expect_string ("foo", NOT TO_BE_LESS_THAN, "bar");
  gt_expect_string ("foobar", NOT TO_BE_LESS_THAN, "foobar");
  gt_expect_string ("foobar", NOT TO_BE_LESS_THAN, NULL);
  gt_expect_string ("foobar", NOT TO_BE_LESS_THAN, "");
  gt_expect_string ("", NOT TO_BE_LESS_THAN, "");
  gt_expect_string ("", NOT TO_BE_LESS_THAN, NULL);
  gt_expect_string (NULL, NOT TO_BE_LESS_THAN, NULL);
}

void
test_string_to_be_empty (void)
{
  gt_expect_string ("", TO_BE_EMPTY);
}

void
test_string_not_to_be_empty (void)
{
  gt_expect_string ("foobar", NOT TO_BE_EMPTY);
}

void
test_string_to_match (void)
{
  gt_expect_string ("foobar", TO_MATCH, "^fo+");
}

void
test_string_not_to_match (void)
{
  gt_expect_string ("foobar", NOT TO_MATCH, "barf");
}

void
test_error_to_be_clear (void)
{
  gt_expect_error (NULL, TO_BE_CLEAR);
}

void
test_error_not_to_be_clear (void)
{
  GError *error = g_error_new_literal (G_SPAWN_ERROR, G_SPAWN_ERROR_FORK, "e");
  gt_expect_error (error, NOT TO_BE_CLEAR);
  g_error_free (error);
}

void
test_error_to_match (void)
{
  GError *error = g_error_new_literal (G_SPAWN_ERROR, G_SPAWN_ERROR_FORK, "e");
  gt_expect_error (error, TO_MATCH, G_SPAWN_ERROR, G_SPAWN_ERROR_FORK);
  g_error_free (error);
}

void
test_error_not_to_match (void)
{
  GError *error = g_error_new_literal (G_SPAWN_ERROR, G_SPAWN_ERROR_FORK, "e");
  gt_expect_error (error, NOT TO_MATCH, G_SPAWN_ERROR, G_SPAWN_ERROR_READ);
  gt_expect_error (error, NOT TO_MATCH, G_FILE_ERROR, G_FILE_ERROR_EXIST);
  g_error_free (error);
  gt_expect_error (NULL, NOT TO_MATCH, G_SPAWN_ERROR, G_SPAWN_ERROR_FORK);
}

void
test_object_to_be_null (void)
{
  gt_expect_object (NULL, TO_BE_NULL);
}

void
test_object_not_to_be_null (void)
{
  GObject *object = g_object_new (G_TYPE_OBJECT, NULL);
  gt_expect_object (object, NOT TO_BE_NULL);
  g_object_unref (object);
}

void
test_object_to_be_a (void)
{
  GObject *object = g_object_new (G_TYPE_OBJECT, NULL);
  gt_expect_object (object, TO_BE_A, G_TYPE_OBJECT);
  g_object_unref (object);
}

void
test_object_not_to_be_a (void)
{
  GObject *object = g_object_new (G_TYPE_OBJECT, NULL);
  gt_expect_object (object, NOT TO_BE_A, G_TYPE_INITIALLY_UNOWNED);
  g_object_unref (object);
  gt_expect_object (NULL, NOT TO_BE_A, G_TYPE_OBJECT);
}

void
test_file_to_equal (void)
{
  GFile *file = g_file_new_for_path ("a");
  gt_expect_file (file, TO_EQUAL, file);
  g_object_unref (file);
}

void
test_file_not_to_equal (void)
{
  GFile *file1 = g_file_new_for_path ("a");
  GFile *file2 = g_file_new_for_path ("b");
  gt_expect_file (file1, NOT TO_EQUAL, file2);
  g_object_unref (file1);
  g_object_unref (file2);
}

void
test_file_to_be_null (void)
{
  gt_expect_file (NULL, TO_BE_NULL);
}

void
test_file_not_to_be_null (void)
{
  GFile *file = g_file_new_for_path ("a");
  gt_expect_file (file, NOT TO_BE_NULL);
  g_object_unref (file);
}

void
test_file_to_have_parent (void)
{
  GFile *file1 = g_file_new_for_path ("a");
  GFile *file2 = g_file_get_child (file1, "b");
  gt_expect_file (file2, TO_HAVE_PARENT, file1);
  gt_expect_file (file2, TO_HAVE_PARENT, NULL);
  g_object_unref (file1);
  g_object_unref (file2);
}

void
test_file_not_to_have_parent (void)
{
  GFile *file1 = g_file_new_for_path ("/");
  GFile *file2 = g_file_get_child (file1, "b");
  gt_expect_file (file1, NOT TO_HAVE_PARENT, file2);
  gt_expect_file (file1, NOT TO_HAVE_PARENT, NULL);
  g_object_unref (file1);
  g_object_unref (file2);
}

void
test_file_to_have_prefix (void)
{
  GFile *file1 = g_file_new_for_path ("a");
  GFile *file2 = g_file_get_child (file1, "b");
  gt_expect_file (file2, TO_HAVE_PREFIX, file1);
  g_object_unref (file1);
  g_object_unref (file2);
}

void
test_file_not_to_have_prefix (void)
{
  GFile *file1 = g_file_new_for_path ("a");
  GFile *file2 = g_file_new_for_path ("b");
  gt_expect_file (file1, NOT TO_HAVE_PREFIX, file2);
  g_object_unref (file1);
  g_object_unref (file2);
}

void
test_file_to_exist (void)
{
  GFile *file = G_FILE (gt_mock_file_new ());
  gt_expect_file (file, TO_EXIST);
  g_object_unref (file);
}

void
test_file_not_to_exist (void)
{
  GFile *file = G_FILE (g_object_new (GT_TYPE_MOCK_FILE,
                                      "exists", FALSE,
                                      NULL));
  gt_expect_file (file, NOT TO_EXIST);
  g_object_unref (file);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/bool/to-equal", test_bool_to_equal);
  g_test_add_func ("/bool/not-to-equal", test_bool_not_to_equal);
  g_test_add_func ("/bool/to-be-true", test_bool_to_be_true);
  g_test_add_func ("/bool/not-to-be-true", test_bool_not_to_be_true);
  g_test_add_func ("/bool/to-be-false", test_bool_to_be_false);
  g_test_add_func ("/bool/not-to-be-false", test_bool_not_to_be_false);

  g_test_add_func ("/int/to-equal", test_int_to_equal);
  g_test_add_func ("/int/not-to-equal", test_int_not_to_equal);
  g_test_add_func ("/int/to-be-less-than", test_int_to_be_less_than);
  g_test_add_func ("/int/not-to-be-less-than", test_int_not_to_be_less_than);

  g_test_add_func ("/uint/to-equal", test_uint_to_equal);
  g_test_add_func ("/uint/not-to-equal", test_uint_not_to_equal);
  g_test_add_func ("/uint/to-be-less-than", test_uint_to_be_less_than);
  g_test_add_func ("/uint/not-to-be-less-than", test_uint_not_to_be_less_than);

  g_test_add_func ("/string/to-be-null", test_string_to_be_null);
  g_test_add_func ("/string/not-to-be-null", test_string_not_to_be_null);
  g_test_add_func ("/string/to-equal", test_string_to_equal);
  g_test_add_func ("/string/not-to-equal", test_string_not_to_equal);
  g_test_add_func ("/string/to-be-less-than", test_string_to_be_less_than);
  g_test_add_func ("/string/not-to-be-less-than",
                   test_string_not_to_be_less_than);
  g_test_add_func ("/string/to-be-empty", test_string_to_be_empty);
  g_test_add_func ("/string/not-to-be-empty", test_string_not_to_be_empty);
  g_test_add_func ("/string/to-match", test_string_to_match);
  g_test_add_func ("/string/not-to-match", test_string_not_to_match);

  g_test_add_func ("/error/to-be-clear", test_error_to_be_clear);
  g_test_add_func ("/error/not-to-be-clear", test_error_not_to_be_clear);
  g_test_add_func ("/error/to-match", test_error_to_match);
  g_test_add_func ("/error/not-to-match", test_error_not_to_match);

  g_test_add_func ("/object/to-be-null", test_object_to_be_null);
  g_test_add_func ("/object/not-to-be-null", test_object_not_to_be_null);
  g_test_add_func ("/object/to-be-a", test_object_to_be_a);
  g_test_add_func ("/object/not-to-be-a", test_object_not_to_be_a);

  g_test_add_func ("/file/to-equal", test_file_to_equal);
  g_test_add_func ("/file/not-to-equal", test_file_not_to_equal);
  g_test_add_func ("/file/to-be-null", test_file_to_be_null);
  g_test_add_func ("/file/not-to-be-null", test_file_not_to_be_null);
  g_test_add_func ("/file/to-have-parent", test_file_to_have_parent);
  g_test_add_func ("/file/not-to-have-parent", test_file_not_to_have_parent);
  g_test_add_func ("/file/to-have-prefix", test_file_to_have_prefix);
  g_test_add_func ("/file/not-to-have-prefix", test_file_not_to_have_prefix);
  g_test_add_func ("/file/to-exist", test_file_to_exist);
  g_test_add_func ("/file/not-to-exist", test_file_not_to_exist);

  return g_test_run ();
}
