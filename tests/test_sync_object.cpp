/**
 * @file test_sync_object.cpp
 * @brief Unit tests for the tools::sync_object class.
 *
 * @author Laurent Lardinois and Copilot
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

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "tools/sync_object.hpp"

TEST(SyncObjectTest, WaitForSignalUnblocksAfterSignal)
{
    tools::sync_object sync;
    std::atomic_bool signaled { false };

    std::thread waiter(
        [&sync, &signaled]()
        {
            sync.wait_for_signal();
            signaled.store(true);
        });

    // give the waiter time to block before signaling
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(signaled.load());

    sync.signal();
    waiter.join();

    EXPECT_TRUE(signaled.load());
}

TEST(SyncObjectTest, WaitForSignalWithTimeoutExpires)
{
    tools::sync_object sync;

    const auto start = std::chrono::steady_clock::now();
    sync.wait_for_signal(std::chrono::milliseconds(50));
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_GE(elapsed, std::chrono::milliseconds(40));
}

TEST(SyncObjectTest, WaitForSignalWithTimeoutReturnsEarlyWhenSignaled)
{
    tools::sync_object sync;
    std::atomic_bool signaled { false };

    std::thread waiter(
        [&sync, &signaled]()
        {
            sync.wait_for_signal(std::chrono::seconds(5));
            signaled.store(true);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sync.signal();
    waiter.join();

    EXPECT_TRUE(signaled.load());
}

// TODO: unlike a typical multi-waiter broadcast, signal_all() here only reliably
// releases one auto-reset waiter per call; do not assume it wakes every blocked thread.
TEST(SyncObjectTest, SignalAllWakesWaiter)
{
    // wait_for_signal auto-resets the flag, so a single signal_all() reliably
    // releases only one blocked waiter; verify that basic contract here.
    tools::sync_object sync;
    std::atomic_bool signaled { false };

    std::thread waiter(
        [&sync, &signaled]()
        {
            sync.wait_for_signal();
            signaled.store(true);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sync.signal_all();
    waiter.join();

    EXPECT_TRUE(signaled.load());
}
