// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QFuture>
#include <QList>
#include <QtConcurrent>

namespace flipman::sdk::core {

/**
 * @class DispatchGroup
 * @brief Groups asynchronous tasks and waits for completion.
 */
class FLIPMANSDK_EXPORT DispatchGroup {
public:
    /**
     * @brief Constructs an empty DispatchGroup.
     */
    DispatchGroup() = default;

    /**
     * @brief Destroys the DispatchGroup.
     *
     * Calls wait() before destruction.
     */
    ~DispatchGroup() { wait(); }

    /**
     * @brief Schedules a task for asynchronous execution.
     *
     * @tparam Func Callable type.
     * @param fn Function or lambda to execute.
     */
    template<typename Func> void async(Func&& fn) { futures.append(QtConcurrent::run(std::forward<Func>(fn))); }

    /**
     * @brief Blocks until all scheduled tasks complete.
     */
    void wait()
    {
        for (auto& f : futures)
            f.waitForFinished();
    }

private:
    QList<QFuture<void>> futures;
};

}  // namespace flipman::sdk::core
