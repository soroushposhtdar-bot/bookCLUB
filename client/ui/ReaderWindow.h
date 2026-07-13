#pragma once

#include <QWidget>

namespace bookclub::client {

class ReaderWindow : public QWidget {
    Q_OBJECT
public:
    explicit ReaderWindow(QWidget* parent = nullptr);
    ~ReaderWindow() override = default;

signals:
    void pageNavigationRequested(int page);
    void zoomRequested(double zoom);
    void closeReaderRequested();

private:
    void setupUi();
};

} // namespace bookclub::client
