#pragma once

#include <Editor/MainWindow.hpp>
#include <QDialog>

class ResourceLocationsWindow : public QDialog
{
    Q_OBJECT
public:

    explicit ResourceLocationsWindow(QWidget* parent = nullptr);
    ~ResourceLocationsWindow() override;

private slots:


private:

    QLabel* _StatusLabel = nullptr;
};