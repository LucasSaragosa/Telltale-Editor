#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>

#include <Application.hpp>

class MainWindow : public QMainWindow 
{

	Q_OBJECT

public:

	MainWindow(Application& app, QWidget* parent = nullptr);
	~MainWindow();

private:

	void _SetupMenus();
	void _SetupToolBar();
	void _SetupCentralWidget();

	QWidget* _RenderSurface;
	QPushButton* _DbgButton;
	QLabel* _TitleLabel;
	QPushButton* _ChoicesButton;

	Application& _Application;

};