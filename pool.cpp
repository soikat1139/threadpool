#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

class Pool {
private:
    int m_pool;
    vector<thread> threads;
    queue<function<void()>> tasks;
    condition_variable cv;
    mutex mtx;
    bool stop;

public:
    Pool(int num) : m_pool(num), stop(false) {
        for (int i = 0; i < m_pool; i++) {
            threads.emplace_back([this]() {
                while (true) {
                    function<void()> task;

                    {
                        unique_lock<mutex> lock(mtx);
                        cv.wait(lock, [this]() {
                            return !tasks.empty() || stop;
                        });

                        if (stop && tasks.empty())
                            return;

                        task = move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    ~Pool() {
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
        }

        cv.notify_all();

        for (thread &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(mtx);
            tasks.push(move(task));
        }
        cv.notify_one();
    }
};

int main() {
    Pool pool(4);

    for (int i = 0; i < 8; i++) {
        pool.enqueue([i]() {
            cout << "Task " << i << " is being processed by thread " << this_thread::get_id() << endl;
            this_thread::sleep_for(chrono::seconds(1));
        });
    }

    this_thread::sleep_for(chrono::seconds(10));

    return 0;
}
