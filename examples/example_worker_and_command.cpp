/**
 * @file example_worker_and_command.cpp
 * @brief Runs queued command and worker task examples.
 *
 * @author Laurent Lardinois
 * @date September 2026
 */

//-----------------------------------------------------------------------------//
// C++ Publish/Subscribe Pattern - Spare time development for fun              //
// (c) 2025-2026 Laurent Lardinois https://be.linkedin.com/in/laurentlardinois //
//                                                                             //
// https://github.com/type-one/PublishSubscribe                                //
//                                                                             //
// MIT License                                                                 //
//                                                                             //
// This software is provided 'as-is', without any express or implied           //
// warranty.In no event will the authors be held liable for any damages        //
// arising from the use of this software.                                      //
//                                                                             //
// Permission is granted to anyone to use this software for any purpose,       //
// including commercial applications, and to alter itand redistribute it       //
// freely, subject to the following restrictions :                             //
//                                                                             //
// 1. The origin of this software must not be misrepresented; you must not     //
// claim that you wrote the original software.If you use this software         //
// in a product, an acknowledgment in the product documentation would be       //
// appreciated but is not required.                                            //
// 2. Altered source versions must be plainly marked as such, and must not be  //
// misrepresented as being the original software.                              //
// 3. This notice may not be removed or altered from any source distribution.  //
//-----------------------------------------------------------------------------//

#include "example_common.hpp"

void test_queued_commands()
{
    std::cout << "-- queued commands --" << '\n';
    tools::sync_queue<std::function<void()>> commands_queue;

    commands_queue.emplace([]() { std::cout << "hello" << '\n'; });

    commands_queue.emplace([]() { std::cout << "world" << '\n'; });

    while (!commands_queue.empty())
    {
        auto call = commands_queue.front_pop();
        if (call.has_value())
        {
            (*call)();
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_ring_buffer_commands()
{
    std::cout << "-- ring buffer commands --" << '\n';
    tools::sync_ring_buffer<std::function<void()>, 128U> commands_queue;

    commands_queue.emplace([]() { std::cout << "hello" << '\n'; });

    commands_queue.emplace([]() { std::cout << "world" << '\n'; });

    while (!commands_queue.empty())
    {
        auto call = commands_queue.front_pop();
        if (call.has_value())
        {
            (*call)();
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

struct my_worker_task_context
{
    std::atomic<int> loop_counter = 0;
    tools::sync_queue<std::chrono::high_resolution_clock::time_point> time_points;
};

using my_worker_task = tools::worker_task<my_worker_task_context>;

void test_worker_tasks()
{
    std::cout << "-- worker tasks --" << '\n';

    auto context = std::make_shared<my_worker_task_context>();

    auto task1 = std::make_unique<my_worker_task>(context, "worker_1");
    auto task2 = std::make_unique<my_worker_task>(context, "worker_2");

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 1);
    std::array<std::unique_ptr<my_worker_task>, 2> tasks = { std::move(task1), std::move(task2) };

    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(100)); // 100 ms

    const auto start_timepoint = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 20; ++i)
    {
        auto idx = distribution(generator);

        tasks.at(idx)->delegate(
            [](const auto& context, const auto& task_name) -> void
            {
                std::cout << "job " << context->loop_counter.load() << " on worker task " << task_name.c_str()
                          << std::endl;
                context->loop_counter++;
                context->time_points.emplace(std::chrono::high_resolution_clock::now());
            });

        std::this_thread::yield();
    }

    // delegate_range (C++17): iterator-pair batch delegation
    std::vector<my_worker_task::call_back> batch_jobs;
    batch_jobs.reserve(4);
    for (int i = 0; i < 4; ++i)
    {
        batch_jobs.emplace_back(
            [](const auto& context, const auto& task_name) -> void
            {
                std::cout << "batch job " << context->loop_counter.load() << " on worker task " << task_name.c_str()
                          << std::endl;
                context->loop_counter++;
                context->time_points.emplace(std::chrono::high_resolution_clock::now());
            });
    }
    tasks[0]->delegate_range(batch_jobs.begin(), batch_jobs.end());

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // delegate_range (C++20): range overload with a container
    tasks[1]->delegate_range(batch_jobs);
#endif

    // sleep 2 sec
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));

    std::cout << "nb of periodic loops = " << context->loop_counter.load() << '\n';

    auto previous_timepoint = start_timepoint;
    while (!context->time_points.empty())
    {
        const auto measured_timepoint = context->time_points.front_pop();

        if (measured_timepoint.has_value())
        {
            const auto elapsed
                = std::chrono::duration_cast<std::chrono::microseconds>(*measured_timepoint - previous_timepoint);
            std::cout << "timepoint: " << elapsed.count() << " us" << '\n';
            previous_timepoint = *measured_timepoint;
        }
    }
}

void test_worker_tasks_async()
{
    std::cout << "-- worker tasks async --" << '\n';

    auto context = std::make_shared<my_worker_task_context>();
    auto task = std::make_unique<my_worker_task>(context, "worker_async");

    auto computation
        = task->delegate_async(
                  [](const std::shared_ptr<my_worker_task_context>& ctx, const std::string& task_name, int value)
                  {
                      ctx->loop_counter++;
                      std::cout << "compute on " << task_name << ", value=" << value << '\n';
                      return value * 2;
                  },
                  21)
              .then([](portable_concurrency::future<int> previous) { return previous.get() + 1; });

    const auto result = computation.get();
    std::cout << "async chained result = " << result << '\n';
    std::cout << "async jobs executed = " << context->loop_counter.load() << '\n';
}

void test_worker_tasks_async_fanout()
{
    std::cout << "-- worker tasks async fanout --" << '\n';

    auto context = std::make_shared<my_worker_task_context>();
    auto task = std::make_unique<my_worker_task>(context, "worker_async_fanout");

    std::vector<portable_concurrency::future<int>> jobs;
    jobs.reserve(5);

    for (int value = 1; value <= 5; ++value)
    {
        jobs.emplace_back(
            task->delegate_async(
                    [](const std::shared_ptr<my_worker_task_context>& ctx, const std::string& task_name, int v)
                    {
                        ctx->loop_counter++;
                        std::cout << "fanout compute on " << task_name << ", value=" << v << '\n';
                        return v * v;
                    },
                    value)
                .then(task->as_executor(),
                    [](portable_concurrency::future<int> previous) { return previous.get() + 10; }));
    }

    auto total_future = portable_concurrency::when_all(std::move(jobs))
                            .next(
                                [](std::vector<portable_concurrency::future<int>> results)
                                {
                                    int total = 0;
                                    for (auto& result_future : results)
                                    {
                                        total += result_future.get();
                                    }
                                    return total;
                                });

    const auto total = total_future.get();
    std::cout << "fanout/fanin total = " << total << '\n';
    std::cout << "fanout async jobs executed = " << context->loop_counter.load() << '\n';
}

void test_portable_concurrency_test_parity()
{
    std::cout << "-- portable_concurrency test parity --" << '\n';

    std::size_t passed = 0U;
    std::size_t failed = 0U;

    auto check = [&passed, &failed](bool condition, const char* name)
    {
        if (condition)
        {
            ++passed;
            std::cout << "  [PASS] " << name << '\n';
        }
        else
        {
            ++failed;
            std::cout << "  [FAIL] " << name << '\n';
        }
    };

    // when_any tuple: result index points to first fulfilled input.
    {
        auto p0 = portable_concurrency::make_promise<int>();
        auto p1 = portable_concurrency::make_promise<std::string>();
        auto sf0 = p0.second.share();

        auto any_future = portable_concurrency::when_any(sf0, std::move(p1.second));
        check(any_future.valid(), "when_any(tuple) returns valid future");
        check(!any_future.is_ready(), "when_any(tuple) not ready before fulfillment");

        p1.first.set_value("hello");
        auto any_result = any_future.get();

        check(any_result.index == 1U, "when_any(tuple) reports ready index");
        check(std::get<1>(any_result.futures).get() == "hello", "when_any(tuple) transports result");
    }

    // when_any vector/range: supports movable future sequences.
    {
        auto p0 = portable_concurrency::make_promise<int>();
        auto p1 = portable_concurrency::make_promise<int>();

        std::vector<portable_concurrency::future<int>> futures;
        futures.emplace_back(std::move(p0.second));
        futures.emplace_back(std::move(p1.second));

        auto any_future = portable_concurrency::when_any(futures.begin(), futures.end());
        p0.first.set_value(7);
        auto any_result = any_future.get();

        check(any_result.index == 0U, "when_any(vector) reports first completed index");
        check(any_result.futures[0].get() == 7, "when_any(vector) contains completed future");
    }

    // packaged_task unwrap: future<future<T>> collapses to future<T> and invalid inner future becomes broken_promise.
    {
        portable_concurrency::packaged_task<portable_concurrency::future<int>()> task_ok(
            []() { return portable_concurrency::make_ready_future(42); });
        auto future_ok = task_ok.get_future();
        task_ok();
        check(future_ok.get() == 42, "packaged_task unwraps ready future result");

        portable_concurrency::packaged_task<portable_concurrency::future<int>()> task_bad(
            []() { return portable_concurrency::future<int> {}; });
        auto future_bad = task_bad.get_future();
        task_bad();

        bool got_broken_promise = false;
        try
        {
            (void)future_bad.get();
        }
        catch (const std::future_error& err)
        {
            got_broken_promise = (err.code() == std::make_error_code(std::future_errc::broken_promise));
        }
        check(got_broken_promise, "packaged_task invalid inner future -> broken_promise");
    }

    // promise lifecycle: abandoning an unresolved promise must fail awaiting future with broken_promise.
    {
        auto promise_and_future = portable_concurrency::make_promise<int>();
        auto abandoned_future = std::move(promise_and_future.second);
        {
            auto abandoned_promise = std::move(promise_and_future.first);
        }

        bool got_broken_promise = false;
        try
        {
            (void)abandoned_future.get();
        }
        catch (const std::future_error& err)
        {
            got_broken_promise = (err.code() == std::make_error_code(std::future_errc::broken_promise));
        }
        check(got_broken_promise, "promise abandon propagates broken_promise");
    }

    // packaged_task lifecycle: destroying a task with outstanding future must abandon shared state.
    {
        portable_concurrency::future<int> pending;
        {
            portable_concurrency::packaged_task<int()> task([]() { return 5; });
            pending = task.get_future();
        }

        bool got_broken_promise = false;
        try
        {
            (void)pending.get();
        }
        catch (const std::future_error& err)
        {
            got_broken_promise = (err.code() == std::make_error_code(std::future_errc::broken_promise));
        }
        check(got_broken_promise, "packaged_task destructor abandons pending future");
    }

    // canceler callback: action executes only when cancellable promise is abandoned before completion.
    {
        bool cancel_called = false;
        {
            auto cancellable = portable_concurrency::make_promise<int>(
                portable_concurrency::canceler_arg, [&cancel_called]() { cancel_called = true; });
            auto awaiting_future = std::move(cancellable.second);
            (void)awaiting_future.valid();
        }
        check(cancel_called, "cancellable promise invokes cancel action on abandon");
    }

    // timed_waiter + latch: timeout first, then ready once work is released.
    {
        portable_concurrency::static_thread_pool pool(2U);
        portable_concurrency::latch gate(2);

        auto async_future = portable_concurrency::async(pool.executor(),
            [&gate]()
            {
                gate.count_down_and_wait();
                return 123;
            });

        portable_concurrency::timed_waiter waiter(async_future);
        const auto timeout_status = waiter.wait_for(std::chrono::milliseconds(1));
        check(timeout_status == portable_concurrency::future_status::timeout,
            "timed_waiter wait_for times out while task blocked");

        gate.count_down();
        const auto ready_status = waiter.wait_for(std::chrono::seconds(1));
        check(ready_status == portable_concurrency::future_status::ready,
            "timed_waiter wait_for becomes ready after latch release");
        check(async_future.get() == 123, "async future returns expected value after latch release");

        pool.wait();
    }

    // shared_future.then: source shared future stays valid and continuation receives result.
    {
        auto promise_and_future = portable_concurrency::make_promise<int>();
        auto shared = promise_and_future.second.share();

        auto continuation
            = shared.then([](const portable_concurrency::shared_future<int>& src) { return src.get() + 1; });

        check(shared.valid(), "shared_future.then keeps source future valid");
        promise_and_future.first.set_value(10);
        check(continuation.get() == 11, "shared_future.then continuation result");
    }

    std::cout << "  summary: passed=" << passed << " failed=" << failed << '\n';
}

#if defined(PC_HAS_COROUTINES)
portable_concurrency::future<int> worker_task_coro_job(
    my_worker_task* task, std::shared_ptr<my_worker_task_context> context, int value)
{
    std::cout << "coroutine started on thread " << std::this_thread::get_id() << '\n';

    // Hop to the worker thread before computing the result.
    co_await task->schedule();

    std::cout << "coroutine resumed on worker thread " << std::this_thread::get_id() << '\n';
    context->loop_counter++;
    co_return value * 3;
}

portable_concurrency::future<int> worker_task_mixed_coro_job(my_worker_task* task,
    std::shared_ptr<my_worker_task_context> context, portable_concurrency::future<int> async_value)
{
    std::cout << "mixed coroutine started on thread " << std::this_thread::get_id() << '\n';

    // Switch coroutine execution to the worker thread.
    co_await task->schedule();

    std::cout << "mixed coroutine resumed on worker thread " << std::this_thread::get_id() << '\n';

    // Await the delegate_async result from inside the coroutine flow.
    const auto async_result = co_await async_value;
    context->loop_counter++;
    co_return async_result + 7;
}

void test_worker_tasks_coroutine_schedule()
{
    std::cout << "-- worker tasks coroutine schedule --" << '\n';

    auto context = std::make_shared<my_worker_task_context>();
    auto task = std::make_unique<my_worker_task>(context, "worker_async_coro");

    auto result_future
        = worker_task_coro_job(task.get(), context, 14)
              .then(task->as_executor(), [](portable_concurrency::future<int> previous) { return previous.get() + 2; });

    const auto result = result_future.get();
    std::cout << "coroutine result = " << result << '\n';
    std::cout << "coroutine jobs executed = " << context->loop_counter.load() << '\n';
}

void test_worker_tasks_mixed_execution()
{
    std::cout << "-- worker tasks mixed execution --" << '\n';

    auto context = std::make_shared<my_worker_task_context>();
    auto task = std::make_unique<my_worker_task>(context, "worker_mixed");

    task->delegate(
        [](const std::shared_ptr<my_worker_task_context>& ctx, const std::string& task_name)
        {
            std::cout << "delegate on " << task_name << '\n';
            ctx->loop_counter++;
        });

    auto async_value = task->delegate_async(
        [](const std::shared_ptr<my_worker_task_context>& ctx, const std::string& task_name, int value)
        {
            std::cout << "delegate_async on " << task_name << ", value=" << value << '\n';
            ctx->loop_counter++;
            return value * 5;
        },
        6);

    auto mixed_result_future
        = worker_task_mixed_coro_job(task.get(), context, std::move(async_value))
              .then(task->as_executor(), [](portable_concurrency::future<int> previous) { return previous.get() + 1; });

    const auto mixed_result = mixed_result_future.get();
    std::cout << "mixed execution result = " << mixed_result << '\n';
    std::cout << "mixed execution jobs executed = " << context->loop_counter.load() << '\n';
}
#endif

void run_example_worker_and_command()
{
    test_queued_commands();
    test_ring_buffer_commands();
    test_worker_tasks();
    test_worker_tasks_async();
    test_worker_tasks_async_fanout();
    test_portable_concurrency_test_parity();
#if defined(PC_HAS_COROUTINES)
    test_worker_tasks_coroutine_schedule();
    test_worker_tasks_mixed_execution();
#endif
}
