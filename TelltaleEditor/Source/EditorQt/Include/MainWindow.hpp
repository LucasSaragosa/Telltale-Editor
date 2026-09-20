#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupMenus();
    void setupToolBar();
    void setupCentralWidget();

    // The central rendering area
    QWidget* m_renderSurface;
    
    // Bottom UI elements from the screenshot
    QPushButton* m_debugButton;
    QLabel* m_titleLabel;
    QPushButton* m_choicesButton;
};