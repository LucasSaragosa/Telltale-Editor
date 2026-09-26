#pragma once

#include <TelltaleEditor.hpp>

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>

extern Ptr<ResourceRegistry> GetEditorResourceRegistry();

class MainWindow : public QMainWindow {
	Q_OBJECT

public:

	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	inline Ptr<ResourceRegistry> GetResourceRegistry()
	{
		return _Registry;
	}

private:
	void _SetupMenus();
	void _SetupToolBar();
	void _SetupCentralWidget();

	QWidget* _RenderSurface;

	QPushButton* _DbgButton;
	QLabel* _TitleLabel;
	QPushButton* _ChoicesButton;

	TelltaleEditor* _Context = nullptr;
	Ptr<ResourceRegistry> _Registry;

};