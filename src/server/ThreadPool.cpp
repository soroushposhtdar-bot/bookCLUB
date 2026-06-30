// src/server/ThreadPool.cpp
#include "src/server/ThreadPool.h"
#include "common/Utils/Logger.h"

#include <QRunnable>

namespace bookclub::server {

// --- Task Wrapper for std::function ---
class FunctionTask : public QRunnable {
public:
    explicit FunctionTask(std::function<void()> task)
        : m_task(task)
    {}

    void run() override {
        if (m_task) {
            m_task();
        }
    }

private:
    std::function<void()> m_task;
};

// --- Singleton ---
ThreadPool& ThreadPool::instance() {
    static ThreadPool instance;
    return instance;
}

ThreadPool::ThreadPool(QObject* parent)
    : QObject(parent)
{
    // Default to number of processor cores
    int cores = QThread::idealThreadCount();
    if (cores > 0) {
        m_pool.setMaxThreadCount(cores * 2);
    } else {
        m_pool.setMaxThreadCount(4);
    }
    m_pool.setExpiryTimeout(30000); // 30 seconds

    LOG_INFO("ThreadPool initialized with " + QString::number(m_pool.maxThreadCount()) + " threads");
}

ThreadPool::~ThreadPool()
{
    waitForDone();
}

void ThreadPool::setMaxThreadCount(int count)
{
    QMutexLocker locker(&m_mutex);
    if (count > 0) {
        m_pool.setMaxThreadCount(count);
        LOG_INFO("ThreadPool max threads set to: " + QString::number(count));
    }
}

int ThreadPool::maxThreadCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_pool.maxThreadCount();
}

int ThreadPool::activeThreadCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_pool.activeThreadCount();
}

void ThreadPool::submitTask(std::function<void()> task)
{
    auto* runnable = new FunctionTask(task);
    submitTask(runnable);
}

void ThreadPool::submitTask(QRunnable* runnable)
{
    if (!runnable) return;

    QMutexLocker locker(&m_mutex);
    runnable->setAutoDelete(true);
    m_pool.start(runnable);
    emit taskStarted();
}

void ThreadPool::waitForDone()
{
    QMutexLocker locker(&m_mutex);
    m_pool.waitForDone();
    emit allTasksDone();
}

void ThreadPool::clear()
{
    QMutexLocker locker(&m_mutex);
    m_pool.clear();
}

} // namespace bookclub::server
