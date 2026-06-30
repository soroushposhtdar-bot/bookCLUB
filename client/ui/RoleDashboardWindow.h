#pragma once

#include <QWidget>

namespace bookclub::client {

class RoleDashboardWindow : public QWidget {
    Q_OBJECT
public:
    explicit RoleDashboardWindow(QWidget* parent = nullptr);
    ~RoleDashboardWindow() override = default;

signals:
    void sectionChanged(const QString& sectionName);

private:
    void setupUi();
};

} // namespace bookclub::client
