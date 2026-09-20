#include <Editor/Dialogs/SelectGameDialog.hpp>

#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>
#include <QMessageBox>
#include <QStandardPaths>

// PLACEHOLDER - THIS IS ONLY A CONCEPT

SelectGameDialog::SelectGameDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle("Select Game");
	resize(640, 320);

	// TODO: Add translations
	m_folderEdit = new QLineEdit(this);
	m_folderBrowse = new QPushButton("Browse...", this);
	m_folderBrowse->setToolTip("Select the installation folder which contains the executable file.");

	auto* folderRow = new QHBoxLayout;
	folderRow->addWidget(m_folderEdit, 1);
	folderRow->addWidget(m_folderBrowse);

	m_exeEdit = new QLineEdit(this);
	m_exeBrowse = new QPushButton("Browse...", this);

	auto* exeRow = new QHBoxLayout;
	exeRow->addWidget(m_exeEdit, 1);
	exeRow->addWidget(m_exeBrowse);

	m_gameCombo = new QComboBox(this);
	m_gameCombo->addItem("(unknown)", QString());

	m_platformCombo = new QComboBox(this);
	m_platformCombo->addItem("(unknown)", QString());

	m_snapshotCombo = new QComboBox(this);
	m_snapshotCombo->addItem("No snapshot", QString());

	m_statusLabel = new QLabel("Pick a game folder to begin.", this);
	m_statusLabel->setWordWrap(true);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

	auto* form = new QFormLayout;
	form->addRow("Install folder:", folderRow);
	form->addRow("Executable:", exeRow);
	form->addRow(tr("Game:"), m_gameCombo);
	form->addRow(tr("Platform:"), m_platformCombo);
	form->addRow(tr("Snapshot:"), m_snapshotCombo);

	auto* root = new QVBoxLayout(this);
	root->addLayout(form);
	root->addWidget(m_statusLabel, 1);
	root->addWidget(m_buttons);

	// Signals
	connect(m_folderBrowse, &QPushButton::clicked, this, &SelectGameDialog::browseForFolder);
	connect(m_exeBrowse, &QPushButton::clicked, this, &SelectGameDialog::browseForExecutable);
	connect(m_folderEdit, &QLineEdit::textChanged, this, &SelectGameDialog::onFolderChanged);
	connect(m_exeEdit, &QLineEdit::textChanged, this, &SelectGameDialog::onExecutableChanged);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &SelectGameDialog::onAccept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &SelectGameDialog::reject);
	connect(m_gameCombo, &QComboBox::currentIndexChanged, this, &SelectGameDialog::updateAcceptState);
	connect(m_platformCombo, &QComboBox::currentIndexChanged, this, &SelectGameDialog::updateAcceptState);
	connect(m_buildCombo, &QComboBox::currentTextChanged, this, &SelectGameDialog::updateAcceptState);

	updateAcceptState();
}

void SelectGameDialog::browseForFolder()
{
	const QString start = m_folderEdit->text().isEmpty()
		? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
		: m_folderEdit->text();

	const QString dir = QFileDialog::getExistingDirectory(
		this, "Select game install folder", start,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!dir.isEmpty())
		m_folderEdit->setText(QDir::toNativeSeparators(dir));
}

void SelectGameDialog::browseForExecutable()
{
	const QString start = m_folderEdit->text().isEmpty()
		? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
		: m_folderEdit->text();

	const QString exe = QFileDialog::getOpenFileName(
		this, "Select game executable", start,
		"Executables (*.exe);;All files (*)");

	if (!exe.isEmpty())
		m_exeEdit->setText(QDir::toNativeSeparators(exe));
}

void SelectGameDialog::onFolderChanged(const QString& path)
{
	detectGame(path);
	refreshSnapshots(path);
	updateAcceptState();
}

void SelectGameDialog::onExecutableChanged(const QString& path)
{
	updateAcceptState();
}

void SelectGameDialog::detectGame(const QString& folder)
{

	updateAcceptState();
}

void SelectGameDialog::refreshSnapshots(const QString& folder)
{
}

void SelectGameDialog::updateAcceptState()
{

}

void SelectGameDialog::onAccept()
{
	accept();
}