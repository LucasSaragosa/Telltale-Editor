#include <Editor/Dialogs/SelectGameDialog.hpp>
#include <Editor/AppSettings.hpp>

#include <Meta/Meta.hpp>

#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>
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
#include <QMessageBox>
#include <QStandardPaths>

SelectGameDialog::SelectGameDialog(QWidget* parent)
    : QDialog(parent)
    , _ActiveSnapshot(new GameSnapshot{})
{
    setWindowTitle(tr("Select Game"));
    resize(640, 320);

    _ExeEditor = new QLineEdit;
    _ExeBrowse = new QPushButton(tr("Browse..."));

    auto* exeRow = new QHBoxLayout;
    exeRow->addWidget(_ExeEditor, 1);
    exeRow->addWidget(_ExeBrowse);

    _GameComboBox = new QComboBox;
    _PlatformCombo = new QComboBox;
    _VendorCombo = new QComboBox;

    _StatusLabel = new QLabel(tr("Pick a game executable to begin."));
    _StatusLabel->setWordWrap(true);

    _Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    _Buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto* form = new QFormLayout;
    form->addRow(tr("Executable:"), exeRow);
    form->addRow(tr("Game:"), _GameComboBox);
    form->addRow(tr("Platform:"), _PlatformCombo);
    form->addRow(tr("Vendor:"), _VendorCombo);

    auto* root = new QVBoxLayout;
    root->addLayout(form);
    root->addWidget(_StatusLabel, 1);
    root->addWidget(_Buttons);
    setLayout(root);

    // Populate combos from the engine's registered games.
    _PopulateGameCombos();

    // Restore last-used executable.
    auto& s = AppSettings::Get();
    const QString lastExe = s.GetExecutablePath();
    if (!lastExe.isEmpty()) {
        _ExeEditor->setText(QDir::toNativeSeparators(lastExe));
        _ExeEditor->setToolTip(QDir::toNativeSeparators(lastExe));

        // Try to auto-detect game/platform/vendor from the restored path.
        _DetectGameFromPath(lastExe);
    }

    // Connections
    connect(_ExeBrowse, &QPushButton::clicked,
        this, &SelectGameDialog::_BrowseForExecutable);
    connect(_ExeEditor, &QLineEdit::textChanged,
        this, &SelectGameDialog::_OnChangeExecutable);
    connect(_GameComboBox, &QComboBox::currentIndexChanged,
        this, &SelectGameDialog::_OnGameChange);
    connect(_PlatformCombo, &QComboBox::currentIndexChanged,
        this, &SelectGameDialog::_UpdateAcceptState);
    connect(_VendorCombo, &QComboBox::currentTextChanged,
        this, &SelectGameDialog::_UpdateAcceptState);
    connect(_Buttons, &QDialogButtonBox::accepted,
        this, &SelectGameDialog::_OnAccept);
    connect(_Buttons, &QDialogButtonBox::rejected,
        this, &SelectGameDialog::reject);

    _RestoreFromSettings();

    _UpdateAcceptState();
}

SelectGameDialog::~SelectGameDialog()
{
    delete _ActiveSnapshot;
}

// ---------------------------------------------------------------------------
// Populate from Meta::GetInternalState().Games
// ---------------------------------------------------------------------------
void SelectGameDialog::_PopulateGameCombos()
{
    _GameComboBox->clear();
    _PlatformCombo->clear();
    _VendorCombo->clear();

    _GameComboBox->addItem(tr("(select a game)"), QString());

    auto& games = Meta::GetInternalState().Games;

    for (const auto& g : games) {
        const QString id = QString::fromStdString(g.ID);
        if (id.isEmpty())
            continue;
        _GameComboBox->addItem(id, id);
    }

    // Placeholder rows until a game is chosen.
    _PlatformCombo->addItem(tr("(select a game first)"), QString());
    _VendorCombo->addItem(tr("(none)"), QString());
}

void SelectGameDialog::_OnGameChange(int /*index*/)
{
    const QString gameId = _GameComboBox->currentData().toString();

    _PlatformCombo->blockSignals(true);
    _VendorCombo->blockSignals(true);
    _PlatformCombo->clear();
    _VendorCombo->clear();

    if (gameId.isEmpty()) {
        _PlatformCombo->addItem(tr("(select a game first)"), QString());
        _VendorCombo->addItem(tr("(none)"), QString());
        _PlatformCombo->blockSignals(false);
        _VendorCombo->blockSignals(false);
        _UpdateAcceptState();
        return;
    }

    // Find the RegGame for this ID.
    auto& games = Meta::GetInternalState().Games;
    const Meta::RegGame* reg = nullptr;
    for (const auto& g : games) {
        if (QString::fromStdString(g.ID) == gameId) {
            reg = &g;
            break;
        }
    }

    if (!reg) {
        _PlatformCombo->addItem(tr("(unknown)"), QString());
        _VendorCombo->addItem(tr("(none)"), QString());
        _PlatformCombo->blockSignals(false);
        _VendorCombo->blockSignals(false);
        _UpdateAcceptState();
        return;
    }

    // --- Platforms ---
    for (const auto& p : reg->ValidPlatforms) {
        const QString platform = QString::fromStdString(p);
        if (!platform.isEmpty())
            _PlatformCombo->addItem(platform, platform);
    }
    if (_PlatformCombo->count() == 0)
        _PlatformCombo->addItem(tr("(unknown)"), QString());

    // --- Vendors ---
    // Always allow the empty/none entry.
    _VendorCombo->addItem(tr("(none)"), QString());

    for (const auto& v : reg->ValidVendors) {
        const QString vendor = QString::fromStdString(v);
        if (!vendor.isEmpty())
            _VendorCombo->addItem(vendor, vendor);
    }

    // If ValidVendors is empty, vendor is irrelevant — disable the combo.
    _VendorCombo->setEnabled(!reg->ValidVendors.empty());
    _VendorCombo->setCurrentIndex(0);

    _PlatformCombo->blockSignals(false);
    _VendorCombo->blockSignals(false);

    _UpdateAcceptState();
}

void SelectGameDialog::_BrowseForExecutable()
{
    const QString start = _ExeEditor->text().isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        : _ExeEditor->text();

    const QString exe = QFileDialog::getOpenFileName(
        this, tr("Select game executable"), start,
        tr("Executables (*.exe);;All files (*)"));

    if (exe.isEmpty())
        return;

    const QString native = QDir::toNativeSeparators(exe);
    _ExeEditor->setText(native);
    _ExeEditor->setToolTip(native);
    _DetectGameFromPath(native);
}

void SelectGameDialog::_DetectGameFromPath(const QString& exePath)
{
    if (exePath.isEmpty())
        return;

    const QFileInfo fi(exePath);
    const QString exeName = fi.fileName();  

    auto& games = Meta::GetInternalState().Games;

    for (const auto& g : games) {
        // const QString expected = QString::fromStdString(g.ExecutableName);
        // if (exeName.compare(expected, Qt::CaseInsensitive) != 0)
        //     continue;

        Q_UNUSED(exeName);
        Q_UNUSED(g);
        break;
    }

    _UpdateAcceptState();
}

void SelectGameDialog::_OnChangeExecutable(const QString& path)
{
    _ExeEditor->setToolTip(path);
    _UpdateAcceptState();
}

bool SelectGameDialog::_ValidateInputs(QString* outError) const
{
    const QString exePath = _ExeEditor->text().trimmed();

    if (exePath.isEmpty()) {
        if (outError) *outError = tr("Please select a game executable.");
        return false;
    }

    const QFileInfo fi(exePath);
    if (!fi.exists() || !fi.isFile()) {
        if (outError) *outError = tr("The executable does not exist:\n%1").arg(exePath);
        return false;
    }

    if (_GameComboBox->currentData().toString().isEmpty()) {
        if (outError) *outError = tr("Please select a game.");
        return false;
    }

    if (_PlatformCombo->currentData().toString().isEmpty()) {
        if (outError) *outError = tr("Please select a platform.");
        return false;
    }

    return true;
}

void SelectGameDialog::_UpdateAcceptState()
{
    QString err;
    const bool ok = _ValidateInputs(&err);

    _Buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);

    if (ok) {
        _StatusLabel->setText(tr("Ready. Click OK to open the game."));
    }
    else {
        _StatusLabel->setText(err);
    }
}

void SelectGameDialog::_OnAccept()
{
    QString err;
    if (!_ValidateInputs(&err)) {
        QMessageBox::warning(this, tr("Invalid Selection"), err);
        return;
    }

    _ActiveSnapshot->ID = _GameComboBox->currentData().toString().toStdString();
    _ActiveSnapshot->Platform = _PlatformCombo->currentData().toString().toStdString();
    _ActiveSnapshot->Vendor = _VendorCombo->currentData().toString().toStdString();

    auto& s = AppSettings::Get();
    s.SetExecutablePath(_ExeEditor->text());      
    s.SetGameFolder(QFileInfo(_ExeEditor->text()).absolutePath());
    s.SetSelectedGameId(QString::fromStdString(_ActiveSnapshot->ID));
    s.SetSelectedPlatform(QString::fromStdString(_ActiveSnapshot->Platform));
    s.Sync();

    accept();
}

void SelectGameDialog::_RestoreFromSettings()
{
    auto& s = AppSettings::Get();

    // --- Executable ---
    const QString lastExe = s.GetExecutablePath();
    if (!lastExe.isEmpty()) 
    {
        const QString native = QDir::toNativeSeparators(lastExe);
        _ExeEditor->setText(native);
        _ExeEditor->setToolTip(native);
    }

    // --- Game ---
    // Select the saved game by data (the ID), not by index — indices
    // can shift if the registry changes between runs.
    const QString savedGame = s.GetSelectedGameId();
    if (!savedGame.isEmpty()) 
    {
        const int idx = _GameComboBox->findData(savedGame);
        if (idx >= 0) 
        {
            // Setting the index fires onGameChanged(), which repopulates
            // platform + vendor for this game.
            _GameComboBox->setCurrentIndex(idx);
        }
    }

    // --- Platform ---
    const QString savedPlatform = s.GetSelectedPlatform();
    if (!savedPlatform.isEmpty()) 
    {
        const int idx = _PlatformCombo->findData(savedPlatform);
        if (idx >= 0)
            _PlatformCombo->setCurrentIndex(idx);
    }

    // --- Vendor ---
    const QString savedVendor = s.GetSelectedVendor();
    if (!savedVendor.isEmpty()) 
    {
        const int idx = _VendorCombo->findData(savedVendor);
        if (idx >= 0)
            _VendorCombo->setCurrentIndex(idx);
    }
    else {
        // Empty saved vendor == "(none)". Make sure that's selected.
        _VendorCombo->setCurrentIndex(0);
    }

    // If the exe was restored but no game was saved, try detection.
    if (savedGame.isEmpty() && !lastExe.isEmpty())
        _DetectGameFromPath(lastExe);
}