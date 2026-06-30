// src/server/ThreadPool.h
#pragma once

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <functional>
#include <QMutex>

namespace bookclub::server {

class ThreadPool : public QObject {
    Q_OBJECT
public:
    static ThreadPool& instance();

    // --- Configuration ---
    void setMaxThreadCount(int count);
    int maxThreadCount() const;
    int activeThreadCount() const;

    // --- Task Submission ---
    void submitTask(std::function<void()> task);
    void submitTask(QRunnable* runnable);

    // --- Control ---
    void waitForDone();
    void clear();

signals:
    void taskStarted();
    void taskFinished();
    void allTasksDone();

private:
    ThreadPool(QObject* parent = nullptr);
    ~ThreadPool() override;
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    QThreadPool m_pool;
    QMutex m_mutex;
};

} // namespace bookclub::server
