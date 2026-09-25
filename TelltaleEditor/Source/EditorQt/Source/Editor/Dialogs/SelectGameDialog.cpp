#include <Editor/Dialogs/SelectGameDialog.hpp>
#include <Editor/AppSettings.hpp>

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
	m_exeEdit = new QLineEdit;
	m_exeBrowse = new QPushButton("Browse...");

	auto* exeRow = new QHBoxLayout;
	exeRow->addWidget(m_exeEdit, 1);
	exeRow->addWidget(m_exeBrowse);

	m_gameCombo = new QComboBox;
	m_gameCombo->addItem("(unknown)", QString());

	m_platformCombo = new QComboBox;
	m_platformCombo->addItem("(unknown)", QString());

	m_vendorCombo = new QComboBox;
	m_vendorCombo->addItem("No vendor", QString());

	m_statusLabel = new QLabel("Pick a game folder to begin.");
	m_statusLabel->setWordWrap(true);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false); // This must be controlled by checking for valid game paths, exes and snapshots.

	auto* form = new QFormLayout;
	
	form->addRow("Executable:", exeRow);
	form->addRow(tr("Game:"), m_gameCombo);
	form->addRow(tr("Platform:"), m_platformCombo);
	form->addRow(tr("Vendor:"), m_vendorCombo);

	auto* root = new QVBoxLayout;
	root->addLayout(form);
	root->addWidget(m_statusLabel, 1);
	root->addWidget(m_buttons);

	this->setLayout(root);

	// Signals
	connect(m_exeBrowse, &QPushButton::clicked, this, &SelectGameDialog::browseForExecutable);
	connect(m_exeEdit, &QLineEdit::textChanged, this, &SelectGameDialog::onExecutableChanged);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &SelectGameDialog::onAccept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &SelectGameDialog::reject);
	connect(m_gameCombo, &QComboBox::currentIndexChanged, this, &SelectGameDialog::updateAcceptState);
	connect(m_platformCombo, &QComboBox::currentIndexChanged, this, &SelectGameDialog::updateAcceptState);
	connect(m_vendorCombo, &QComboBox::currentTextChanged, this, &SelectGameDialog::updateAcceptState);

	auto& s = AppSettings::Get();

	m_exeEdit->setText(s.GetExecutablePath());
	m_exeEdit->setToolTip(s.GetExecutablePath());

	if (auto* lbl = qobject_cast<QLabel*>(form->labelForField(exeRow)))
		lbl->setToolTip(tr("Select the game's executable file."));

	updateAcceptState();
}

void SelectGameDialog::browseForExecutable()
{
	const QString start = m_exeEdit->text().isEmpty()
		? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
		: m_exeEdit->text();

	const QString exe = QFileDialog::getOpenFileName(
		this, "Select game executable", start,
		"Executables (*.exe);;All files (*)");

	if (!exe.isEmpty()) {
		m_exeEdit->setText(QDir::toNativeSeparators(exe));
		m_exeEdit->setToolTip(QDir::toNativeSeparators(exe));
	}
	
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
	auto& s = AppSettings::Get();
	s.SetExecutablePath(m_exeBrowse->text());



	accept();
}