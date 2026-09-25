#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QDialogButtonBox;

class SelectGameDialog : public QDialog {
	Q_OBJECT

public:
	explicit SelectGameDialog(QWidget* parent = nullptr);


private slots:
	void browseForExecutable();
	void onFolderChanged(const QString& path);
	void onExecutableChanged(const QString& path);
	void onAccept();

private:
	void detectGame(const QString& folder);
	void refreshSnapshots(const QString& folder);
	void updateAcceptState();

	QLineEdit* m_folderEdit = nullptr;
	QPushButton* m_folderBrowse = nullptr;
	QLineEdit* m_exeEdit = nullptr;
	QPushButton* m_exeBrowse = nullptr;
	QLabel* m_statusLabel = nullptr;
	QDialogButtonBox* m_buttons = nullptr;

	QComboBox* m_gameCombo = nullptr;
	QComboBox* m_platformCombo = nullptr;
	QComboBox* m_vendorCombo = nullptr;
};