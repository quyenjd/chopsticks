#ifndef THREAD_HPP_INCLUDED
#define THREAD_HPP_INCLUDED

#include <algorithm>
#include <condition_variable>
#include <conio.h>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <thread>
#include <vector>

namespace Thread
{
    template< typename T, typename Comparator = std::not_equal_to<T> >
    class Atomic
    {
    private:
        mutable std::mutex mutex;
        mutable std::condition_variable cv;
        T state;
        Comparator comp;

    public:
        typedef std::function<void(const T&)> access_fn;
        typedef std::function<void(T&)> mutator_fn;

        Atomic (Comparator _comp = std::not_equal_to<T>()): comp(_comp) {}
        Atomic (const T& _state, Comparator _comp = std::not_equal_to<T>()): state(_state), comp(_comp) {}
        Atomic (T&& _state, Comparator _comp = std::not_equal_to<T>()): state(std::move(_state)), comp(_comp) {}

        // non-copyable
        Atomic (const Atomic&) = delete;
        Atomic& operator= (const Atomic&) = delete;

        // movable
        Atomic (Atomic&&) = default;
        Atomic& operator= (Atomic&&) = default;

        void wait_until_cond (const T& state) const
        {
            if (comp(get(), state))
                return;

            std::unique_lock<std::mutex> safe(mutex);
            cv.wait(safe, [=]() bool {
                return comp(get(), state);
            });
        }

        void set (const T& state)
        {
            if (comp(get(), state))
            {
                std::unique_lock<std::mutex> safe(mutex);
                this->state = state;
                cv.notify_all();
            }
        }

        void set (T&& state)
        {
            if (comp(get(), state))
            {
                std::unique_lock<std::mutex> safe(mutex);
                this->state = std::move(state);
                cv.notify_all();
            }
        }

        T get() const
        {
            std::unique_lock<std::mutex> safe(mutex);
            return state;
        }

        void access (access_fn fn) const
        {
            std::unique_lock<std::mutex> safe(mutex);
            fn(state);
            cv.notify_all();
        }

        void mutate (mutator_fn fn)
        {
            std::unique_lock<std::mutex> safe(mutex);
            fn(state);
            cv.notify_all();
        }
    };

    class ThreadPool
    {
    private:
        typedef std::function<void()> Task;

        size_t _num_of_threads;
        std::vector<std::thread> threads;
        std::queue<Task> tasks;
        std::mutex mutex;
        std::condition_variable cv;
        Atomic<size_t> num_of_waiting_threads;
        Atomic<bool> terminated, paused;

        void check_terminated()
        {
            if (terminated.get())
                throw std::runtime_error("Thread pool has been terminated before");
        }

        // a wrapper that acquires and completes the tasks
        static void caller(ThreadPool *pool)
        {
            while (true)
            {
                // if terminated, end the loop/thread
                if (pool->terminated.get())
                    break;

                std::unique_lock<std::mutex> lock(pool->mutex, std::defer_lock);

                // if paused or out of tasks, wait
                if (pool->tasks.empty() || pool->paused.get())
                {
                    pool->num_of_waiting_threads.mutate([](size_t &old) { ++old; });
                    pool->cv.wait(lock, [=]() {
                        return pool->terminated.get() || !(pool->tasks.empty() || pool->paused.get());
                    });
                    pool->num_of_waiting_threads.mutate([](size_t &old) { --old; });
                }

                // if terminated once again, end the loop/thread
                if (pool->terminated.get())
                    break;

                // take next task
                auto task = std::move(pool->tasks.front());
                pool->tasks.pop();

                lock.unlock();

                task();
            }
        }

    public:
        ThreadPool (size_t __num_of_threads = 0): terminated(false)
        {
            _num_of_threads = __num_of_threads ? __num_of_threads : std::thread::hardware_concurrency();

            std::cout << "Starting " << _num_of_threads << " thread" << (_num_of_threads > 1 ? "s" : "") << "... ";

            threads.reserve(_num_of_threads);
            std::generate_n(std::back_inserter(threads), _num_of_threads, [this]() { return std::thread(caller, this); });

            std::cout << "Done" << std::endl
                      << "Press any key to continue... ";
            getch();
        }

        ~ThreadPool()
        {
            clear();

            // notify all threads to terminate
            terminated.set(true);
            cv.notify_all();

            // join all threads
            for (auto &thread: threads)
                if (thread.joinable())
                    thread.join();
        }

        // non-copyable
        ThreadPool (const ThreadPool&) = delete;
        ThreadPool& operator= (const ThreadPool&) = delete;

        // movable
        ThreadPool (ThreadPool&&) = default;
        ThreadPool& operator= (ThreadPool&&) = default;

        template< typename Func, typename... Args >
        auto add (Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>
        {
            check_terminated();

            typedef std::packaged_task<typename std::result_of<Func(Args...)>::type()> PackagedTask;

            auto task = std::make_shared<PackagedTask>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

            auto ret = task->get_future();
            {
                std::lock_guard<std::mutex> lock(mutex);
                tasks.emplace([task]() { (*task)(); });
            }

            // let one waiting thread know there is an available job
            cv.notify_one();

            return ret;
        }

        void terminate()
        {
            check_terminated();
            terminated.set(true);
            cv.notify_all();
        }

        bool pause()
        {
            check_terminated();
            return paused.get();
        }

        void pause (bool flag)
        {
            check_terminated();

            if (paused.get() != flag)
            {
                paused.set(flag);
                cv.notify_all();
            }
        }

        void wait()
        {
            check_terminated();

            // wait until all threads are waiting and all tasks are done
            while (num_of_waiting_threads.get() != _num_of_threads || num_of_waiting_tasks());
        }

        void clear()
        {
            check_terminated();

            std::lock_guard<std::mutex> lock(mutex);
            while (!tasks.empty())
                tasks.pop();
        }

        size_t num_of_threads()
        {
            return _num_of_threads;
        }

        size_t num_of_waiting_tasks()
        {
            check_terminated();

            std::lock_guard<std::mutex> lock(mutex);
            return tasks.size();
        }
    };
}

#endif // THREAD_HPP_INCLUDED
