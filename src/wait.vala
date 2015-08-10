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

// The Wait mechanisms have been cheerfully stolen from Valadate

namespace Gt {
public delegate bool Predicate();
public delegate void Block();
public delegate void AsyncBegin(AsyncReadyCallback callback);
public delegate void CancellableAsyncBegin(Cancellable cancel,
    AsyncReadyCallback callback);
public delegate void AsyncFinish(AsyncResult result);

private class SignalWaiter {
    private MainContext context;
    public MainLoop loop;
    public bool succeeded = false;
    public unowned Predicate predicate;

    public SignalWaiter(MainContext context, Predicate predicate) {
        this.context = context;
        this.predicate = predicate;
        loop = new MainLoop(context, true);
    }

    public int callback() {
        if (predicate()) {
            succeeded = true;
            loop.quit();
        }
        return 0;
    }

    public bool abort() {
        loop.quit();
        return false;
    }
}

private static Source custom_context_add_timeout(MainContext context,
    uint interval, owned SourceFunc function, int priority = Priority.DEFAULT)
{
    var source = new TimeoutSource(interval);
    if (priority != Priority.DEFAULT)
        source.set_priority(priority);
    source.set_callback(function);
    source.attach(context);
    return source;
}

/**
 * Wait until a condition becomes true.
 *
 * Waits until a condition becomes true. The condition is checked at the
 * beginning and than each time emitter emits signal signame.
 * This can be used to check asynchronous functionality that uses signals to
 * signal completion when the first emission does not necessarily imply the
 * desired state was reached.
 *
 * @param timeout Maximum timeout to wait for the emission, in milliseconds.
 * @param emitter The object that will emit signal.
 * @param signame Name of the signal to wait for. May include detail (in the
 * format used by g_signal_connect).
 * @param predicate Function that will be called to test whether the waited-for
 * condition occured.
 * The wait will continue until this function returns true.
 * @param block Function that will start the asynchronous operation.
 * The function will register the signal if it's emitted synchronously from
 * block, while obviously it cannot notice if it is emitted before.
 * @return true if the condition became true, false otherwise.
 */
public bool wait_for_condition(int timeout, Object emitter, string signame,
    owned Predicate predicate, Block? block)
{
    var context = new MainContext();
    context.push_thread_default();

    var waiter = new SignalWaiter(context, predicate);
    var sh = Signal.connect_swapped(emitter, signame,
        (Callback) SignalWaiter.callback, waiter);

    if (block != null)
        block();

    // Check whether the condition is not true already
    waiter.callback();
    // Plan timeout
    var t1 = custom_context_add_timeout(context, timeout, waiter.abort);
    // Run the loop if it was not quit yet.
    if(waiter.loop.is_running())
        waiter.loop.run();

    SignalHandler.disconnect(emitter, sh);
    t1.destroy();

    context.pop_thread_default();
    return waiter.succeeded;
}

/**
 * Wait for signal to be emitted.
 *
 * Waits at most timeout for given signal to be emitted and return whether the
 * signal was emitted.
 * Runs main loop while waiting.
 * This can be used to test asynchronous functionality using signals to signal
 * completion.
 *
 * @param timeout Maximum timeout to wait for the emission, in milliseconds.
 * @param emitter The object that will emit signal.
 * @param signame Name of the signal to wait for. May include detail (in the
 * format used by g_signal_connect).
 * @param block Function that will start the asynchronous operation.
 * The function will register the signal if it's emitted synchronously from
 * block, while obviously it cannot notice if it is emitted before.
 * @return true if the signal was emitted, false otherwise.
 */
public bool wait_for_signal(int timeout, Object emitter, string signame,
    Block? block)
{
    bool condition = false;
    return wait_for_condition(timeout, emitter, signame, () => {
        if (condition)
            return true;
        condition = true;
        return false;
    }, block);
}

/**
 * Wait for an async operation to complete.
 *
 * Waits until a async function completes.
 *
 * @param timeout Maximum timeout to wait for completion, in milliseconds.
 * @param async_function The async function to call.
 * The signature corresponds to function declared as
 * {{{
 *     async void async_function()
 * }}}
 * in Vala.
 * @param async_finish The finish part of the async function.
 * It is assumed it will either assert any problems, or stash the result
 * somewhere.
 * @return true if the function completed and passed the check, false otherwise.
 */
public bool wait_for_async(int timeout, AsyncBegin async_function,
    AsyncFinish async_finish)
{
    var context = new MainContext();
    context.push_thread_default();
    var loop = new MainLoop(context, true);

    AsyncResult? result = null;
    // Plan the async function
    async_function((o, r) => {
        result = r;
        loop.quit();
    });
    // Plan timeout
    var t1 = custom_context_add_timeout(context, timeout, () => {
        loop.quit();
        return false;
    });
    // Run the loop if it was not quit yet.
    if (loop.is_running())
        loop.run();
    t1.destroy();

    // Check the outcome
    if (result == null) {
        context.pop_thread_default();
        return false;
    }
    async_finish(result);

    context.pop_thread_default();
    return true;
}

/**
 * Wait for cancellable async operation to complete.
 *
 * Calls an async function and waits until it completes, at most specified time.
 * If it does not complete in time, it cancels the operation and waits once more
 * the same timeout for the cancellation (it still fails if the cancellation
 * succeeds).
 *
 * @param timeout Maximum timeout to wait for completion, in milliseconds.
 * @param async_function The async function to call.
 * The signature corresponds to function declared as
 * {{{
 *     async void async_function(GLib.Cancellable cancel)
 * }}}
 * in Vala.
 * @param async_finish The finish part of the async function.
 * It is assumed it will either assert any problems, or stash the result
 * somewhere.
 * @return true if the function completed (without being cancelled) and passed
 * the check, false otherwise.
 */
public bool wait_for_cancellable_async(int timeout,
    CancellableAsyncBegin async_function, AsyncFinish async_finish)
{
    var context = new MainContext();
    context.push_thread_default();
    var loop = new MainLoop(context, true);

    AsyncResult? result = null;
    var cancel = new Cancellable();
    // Plan the async function
    async_function(cancel, (o, r) => {
        result = r;
        loop.quit();
    });
    // Plan timeouts
    var t1 = custom_context_add_timeout(context, timeout, () => {
        cancel.cancel();
        return false;
    });
    var t2 = custom_context_add_timeout(context, 2 * timeout, () => {
        loop.quit();
        return false;
    });
    // Run the loop if it was not quit yet.
    if(loop.is_running())
        loop.run();
    t2.destroy();
    t1.destroy();

    // Check the outcome
    if (result == null || cancel.is_cancelled()) {
        // Either the async wasn't called at all (result == null) or it was
        // cancelled. In either case the wait fails.
        context.pop_thread_default();
        return false;
    }
    async_finish(result);

    context.pop_thread_default();
    return true;
}
}  // namespace Gt
