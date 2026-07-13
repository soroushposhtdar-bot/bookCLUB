#pragma once

#include <QObject>
#include <QFuture>
#include <functional>

namespace bookclub::server {

class ThreadPool : public QObject {
    Q_OBJECT
public:
    explicit ThreadPool(QObject* parent = nullptr);
    ~ThreadPool() override = default;

    void setMaxThreadCount(int count);
    int maxThreadCount() const;
    void enqueue(const std::function<void()>& task);
    void clear();

signals:
    void taskStarted();
    void taskFinished();
    void poolOverloaded(bool overloaded);
};

} // namespace bookclub::server
