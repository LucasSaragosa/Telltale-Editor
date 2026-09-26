#pragma once

#include <QDialog>
#include <QLabel>

class ArchiveBrowserWindow : public QDialog
{
    Q_OBJECT
public:

    explicit ArchiveBrowserWindow(QWidget* parent = nullptr);
    ~ArchiveBrowserWindow() override;

private slots:


private:

    QLabel* _StatusLabel = nullptr;
};