#pragma once

#include <Application.hpp>

#include <QDialog>
#include <QLabel>

class ArchiveBrowserWindow : public QDialog
{
    Q_OBJECT
public:

    explicit ArchiveBrowserWindow(Application& app, QWidget* parent = nullptr);
    ~ArchiveBrowserWindow() override;

private slots:


private:

    QLabel* _StatusLabel = nullptr;

    Application& _App;

};