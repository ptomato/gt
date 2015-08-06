/*
 * Copyright 2009 Jan Hudec <bulb@ucw.cz>
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

/* This began as a mostly straightforward C port of Valadate's tests for its
wait mechanism */

#include <glib.h>

#include "gt.h"

/* GtChanger object that the tests will operate upon. Since it's a throwaway
object, we try to keep the boilerplate to a minimum */

typedef struct
{
  GObject parent;
  unsigned count;
} GtChanger;

typedef struct
{
  GObjectClass parent_class;
} GtChangerClass;

typedef struct
{
  unsigned timer;
} GtChangerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GtChanger, gt_changer, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_COUNT
};

static void
gt_changer_init (GtChanger *self)
{
}

static gboolean
tick (GtChanger *self)
{
  self->count++;
  g_object_notify (G_OBJECT (self), "count");
  return G_SOURCE_CONTINUE;
}

static void
gt_changer_stop (GtChanger *self)
{
  GtChangerPrivate *priv = gt_changer_get_instance_private (self);
  if (priv->timer != 0)
    g_source_remove (priv->timer);
  priv->timer = 0;
}

static void
gt_changer_start (GtChanger *self)
{
  GtChangerPrivate *priv = gt_changer_get_instance_private (self);
  gt_changer_stop (self);
  priv->timer = g_timeout_add (100, (GSourceFunc) tick, self);
}

static gboolean
increment_later (GTask *task)
{
  GtChanger *self = g_task_get_source_object (task);
  GtChangerPrivate *priv = gt_changer_get_instance_private (self);
  self->count++;
  g_object_notify (G_OBJECT (self), "count");
  g_task_return_boolean (task, TRUE);
  priv->timer = 0;  /* will be removed by the return value of this function */
  return G_SOURCE_REMOVE;
}

static void
on_cancelled (GCancellable *cancellable,
              GTask        *task)
{
  gt_changer_stop ((GtChanger *) g_task_get_source_object (task));
  g_task_return_error_if_cancelled (task);
}

static void
gt_changer_increment (GtChanger          *self,
                      GCancellable       *cancellable,
                      GAsyncReadyCallback callback,
                      gpointer            data)
{
  GtChangerPrivate *priv = gt_changer_get_instance_private (self);
  gt_changer_stop (self);
  GTask *task = g_task_new (self, cancellable, callback, data);
  priv->timer = g_timeout_add (100, (GSourceFunc) increment_later, task);

  if (cancellable != NULL)
    g_cancellable_connect (cancellable, G_CALLBACK (on_cancelled), task, NULL);
}

static gboolean
gt_changer_increment_finish (GtChanger    *self,
                             GAsyncResult *result,
                             GError      **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
gt_changer_get_property (GObject    *object,
                         unsigned    id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  switch (id)
    {
    case PROP_COUNT:
      g_value_set_uint (value, ((GtChanger *) object)->count);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
gt_changer_set_property (GObject      *object,
                         unsigned      id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (id)
    {
    case PROP_COUNT:
      {
        unsigned new_value = g_value_get_uint (value);
        if (((GtChanger *) object)->count == new_value)
          break;
        ((GtChanger *) object)->count = new_value;
        g_object_notify (object, "count");
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
gt_changer_dispose (GObject *object) {
  gt_changer_stop ((GtChanger *) object);
  G_OBJECT_CLASS (gt_changer_parent_class)->dispose (object);
}

static void
gt_changer_class_init (GtChangerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = gt_changer_get_property;
  object_class->set_property = gt_changer_set_property;
  object_class->dispose = gt_changer_dispose;

  g_object_class_install_property (object_class, PROP_COUNT,
                                   g_param_spec_uint ("count", "", "",
                                                      0, G_MAXUINT32, 0,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/* Test fixture */

typedef struct
{
  GtChanger *changer;
} Fixture;

static void
setup (Fixture      *fixture,
       gconstpointer unused)
{
  fixture->changer = g_object_new (gt_changer_get_type (), NULL);
}

static void
teardown (Fixture      *fixture,
          gconstpointer unused)
{
  g_object_unref (fixture->changer);
}

/* Tests */

static void
test_wait_signal_normal (Fixture      *fixture,
                         gconstpointer unused)
{
  gt_changer_start (fixture->changer);
  g_assert_true (gt_wait_for_signal (500, G_OBJECT (fixture->changer),
                                     "notify::count", NULL, NULL));
  g_assert_cmpuint (fixture->changer->count, ==, 1);
}

static void
set_count_to_5 (GtChanger *changer)
{
  g_object_set (changer, "count", 5, NULL);
}

static void
test_wait_signal_immediate (Fixture      *fixture,
                            gconstpointer unused)
{
  g_assert_true (gt_wait_for_signal (200, G_OBJECT (fixture->changer),
                                     "notify::count", (GtBlock) set_count_to_5,
                                     fixture->changer));
  g_assert_cmpuint (fixture->changer->count, ==, 5);
}

static void
test_wait_signal_fail (Fixture      *fixture,
                       gconstpointer unused)
{
  g_assert_false (gt_wait_for_signal (200, G_OBJECT (fixture->changer),
                                      "notify::count", NULL, NULL));
  g_assert_cmpuint (fixture->changer->count, ==, 0);
}

static gboolean
count_is_2 (GtChanger *changer)
{
  return (changer->count == 2);
}

static void
test_wait_condition_normal (Fixture      *fixture,
                            gconstpointer unused)
{
  gt_changer_start (fixture->changer);
  g_assert_true (gt_wait_for_condition (300, G_OBJECT (fixture->changer),
                                        "notify::count",
                                        (GtPredicate) count_is_2,
                                        fixture->changer, NULL, NULL, NULL));
  g_assert_cmpuint (fixture->changer->count, ==, 2);
}

static gboolean
return_true (void)
{
  return TRUE;
}

static void
test_wait_condition_immediate (Fixture      *fixture,
                               gconstpointer unused)
{
  g_assert_true (gt_wait_for_condition (200, G_OBJECT (fixture->changer),
                                        "notify::count",
                                        (GtPredicate) return_true, NULL, NULL,
                                        NULL, NULL));
  g_assert_cmpuint (fixture->changer->count, ==, 0);
}

static void
test_wait_condition_fail (Fixture      *fixture,
                          gconstpointer unused)
{
  gt_changer_start (fixture->changer);
  g_assert_false (gt_wait_for_condition (150, G_OBJECT (fixture->changer),
                                         "notify::count",
                                         (GtPredicate) count_is_2,
                                         fixture->changer, NULL, NULL, NULL));
  g_assert_cmpuint (fixture->changer->count, ==, 1);
}

static void
start_changer_async (GAsyncReadyCallback callback,
                     gpointer            callback_data,
                     GtChanger          *changer)
{
  gt_changer_increment (changer, NULL, callback, callback_data);
}

static void
finish_changer_async (GAsyncResult *result,
                      Fixture      *fixture)
{
  g_assert_true (gt_changer_increment_finish (fixture->changer, result, NULL));
}

static void
test_wait_async_normal (Fixture      *fixture,
                        gconstpointer unused)
{
  g_assert_true (gt_wait_for_async (150, (GtAsyncBegin) start_changer_async,
                                    fixture->changer,
                                    (GtAsyncFinish) finish_changer_async,
                                    fixture));
  g_assert_cmpuint (fixture->changer->count, ==, 1);
}

static void
test_wait_async_fail (Fixture      *fixture,
                      gconstpointer unused)
{
  g_assert_false (gt_wait_for_async (10, (GtAsyncBegin) start_changer_async,
                                     fixture->changer,
                                     (GtAsyncFinish) finish_changer_async,
                                     fixture));
  // FIXME If it times out, the async function may run to completion when main
  // loop is entered again later.
  gt_changer_stop (fixture->changer);

  g_assert_cmpuint (fixture->changer->count, ==, 0);
}

static void
start_changer_cancellable_async (GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            callback_data,
                                 GtChanger          *changer)
{
  gt_changer_increment (changer, cancellable, callback, callback_data);
}

static void
test_wait_cancellable_async_normal (Fixture      *fixture,
                                    gconstpointer unused)
{
  g_assert_true (gt_wait_for_cancellable_async (150,
                                                (GtCancellableAsyncBegin) start_changer_cancellable_async,
                                                fixture->changer,
                                                (GtAsyncFinish) finish_changer_async,
                                                fixture));
  g_assert_cmpuint (fixture->changer->count, ==, 1);
}

static void
test_wait_cancellable_async_fail (Fixture      *fixture,
                                  gconstpointer unused)
{
  g_assert_false (gt_wait_for_cancellable_async (10,
                                                 (GtCancellableAsyncBegin) start_changer_cancellable_async,
                                                 fixture->changer,
                                                 (GtAsyncFinish) finish_changer_async,
                                                 fixture));
  g_assert_cmpuint (fixture->changer->count, ==, 0);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

#define ADD_WAIT_TEST(path, test_func) \
  g_test_add ((path), Fixture, NULL, setup, (test_func), teardown)

  ADD_WAIT_TEST ("/wait/signal/normal", test_wait_signal_normal);
  ADD_WAIT_TEST ("/wait/signal/immediate", test_wait_signal_immediate);
  ADD_WAIT_TEST ("/wait/signal/fail", test_wait_signal_fail);
  ADD_WAIT_TEST ("/wait/condition/normal", test_wait_condition_normal);
  ADD_WAIT_TEST ("/wait/condition/immediate", test_wait_condition_immediate);
  ADD_WAIT_TEST ("/wait/condition/fail", test_wait_condition_fail);
  ADD_WAIT_TEST ("/wait/async/normal", test_wait_async_normal);
  ADD_WAIT_TEST ("/wait/async/fail", test_wait_async_fail);
  ADD_WAIT_TEST ("/wait/async/cancellable-normal", test_wait_cancellable_async_normal);
  ADD_WAIT_TEST ("/wait/async/cancellable-fail", test_wait_cancellable_async_fail);

#undef ADD_WAIT_TEST

  return g_test_run ();
}
