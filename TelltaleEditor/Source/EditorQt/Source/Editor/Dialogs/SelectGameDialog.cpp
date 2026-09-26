#include <Editor/Dialogs/SelectGameDialog.hpp>
#include <Editor/AppSettings.hpp>

// Engine / meta headers — adjust paths to your project.
#include <Meta/Meta.hpp>          // Meta::GetInternalState()

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
    , m_gameSnapshot(new GameSnapshot{})
{
    setWindowTitle(tr("Select Game"));
    resize(640, 320);

    m_exeEdit = new QLineEdit;
    m_exeBrowse = new QPushButton(tr("Browse..."));

    auto* exeRow = new QHBoxLayout;
    exeRow->addWidget(m_exeEdit, 1);
    exeRow->addWidget(m_exeBrowse);

    m_gameCombo = new QComboBox;
    m_platformCombo = new QComboBox;
    m_vendorCombo = new QComboBox;

    m_statusLabel = new QLabel(tr("Pick a game executable to begin."));
    m_statusLabel->setWordWrap(true);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto* form = new QFormLayout;
    form->addRow(tr("Executable:"), exeRow);
    form->addRow(tr("Game:"), m_gameCombo);
    form->addRow(tr("Platform:"), m_platformCombo);
    form->addRow(tr("Vendor:"), m_vendorCombo);

    auto* root = new QVBoxLayout;
    root->addLayout(form);
    root->addWidget(m_statusLabel, 1);
    root->addWidget(m_buttons);
    setLayout(root);

    // Populate combos from the engine's registered games.
    populateCombos();

    // Restore last-used executable.
    auto& s = AppSettings::Get();
    const QString lastExe = s.GetExecutablePath();
    if (!lastExe.isEmpty()) {
        m_exeEdit->setText(QDir::toNativeSeparators(lastExe));
        m_exeEdit->setToolTip(QDir::toNativeSeparators(lastExe));

        // Try to auto-detect game/platform/vendor from the restored path.
        detectGameFromPath(lastExe);
    }

    // Connections
    connect(m_exeBrowse, &QPushButton::clicked,
        this, &SelectGameDialog::browseForExecutable);
    connect(m_exeEdit, &QLineEdit::textChanged,
        this, &SelectGameDialog::onExecutableChanged);
    connect(m_gameCombo, &QComboBox::currentIndexChanged,
        this, &SelectGameDialog::onGameChanged);
    connect(m_platformCombo, &QComboBox::currentIndexChanged,
        this, &SelectGameDialog::updateAcceptState);
    connect(m_vendorCombo, &QComboBox::currentTextChanged,
        this, &SelectGameDialog::updateAcceptState);
    connect(m_buttons, &QDialogButtonBox::accepted,
        this, &SelectGameDialog::onAccept);
    connect(m_buttons, &QDialogButtonBox::rejected,
        this, &SelectGameDialog::reject);

    restoreFromSettings();

    updateAcceptState();
}

SelectGameDialog::~SelectGameDialog()
{
    delete m_gameSnapshot;
}

// ---------------------------------------------------------------------------
// Populate from Meta::GetInternalState().Games
// ---------------------------------------------------------------------------
void SelectGameDialog::populateCombos()
{
    m_gameCombo->clear();
    m_platformCombo->clear();
    m_vendorCombo->clear();

    m_gameCombo->addItem(tr("(select a game)"), QString());

    auto& games = Meta::GetInternalState().Games;

    for (const auto& g : games) {
        const QString id = QString::fromStdString(g.ID);
        if (id.isEmpty())
            continue;
        m_gameCombo->addItem(id, id);
    }

    // Placeholder rows until a game is chosen.
    m_platformCombo->addItem(tr("(select a game first)"), QString());
    m_vendorCombo->addItem(tr("(none)"), QString());
}

void SelectGameDialog::onGameChanged(int /*index*/)
{
    const QString gameId = m_gameCombo->currentData().toString();

    m_platformCombo->blockSignals(true);
    m_vendorCombo->blockSignals(true);
    m_platformCombo->clear();
    m_vendorCombo->clear();

    if (gameId.isEmpty()) {
        m_platformCombo->addItem(tr("(select a game first)"), QString());
        m_vendorCombo->addItem(tr("(none)"), QString());
        m_platformCombo->blockSignals(false);
        m_vendorCombo->blockSignals(false);
        updateAcceptState();
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
        m_platformCombo->addItem(tr("(unknown)"), QString());
        m_vendorCombo->addItem(tr("(none)"), QString());
        m_platformCombo->blockSignals(false);
        m_vendorCombo->blockSignals(false);
        updateAcceptState();
        return;
    }

    // --- Platforms ---
    for (const auto& p : reg->ValidPlatforms) {
        const QString platform = QString::fromStdString(p);
        if (!platform.isEmpty())
            m_platformCombo->addItem(platform, platform);
    }
    if (m_platformCombo->count() == 0)
        m_platformCombo->addItem(tr("(unknown)"), QString());

    // --- Vendors ---
    // Always allow the empty/none entry.
    m_vendorCombo->addItem(tr("(none)"), QString());

    for (const auto& v : reg->ValidVendors) {
        const QString vendor = QString::fromStdString(v);
        if (!vendor.isEmpty())
            m_vendorCombo->addItem(vendor, vendor);
    }

    // If ValidVendors is empty, vendor is irrelevant — disable the combo.
    m_vendorCombo->setEnabled(!reg->ValidVendors.empty());
    m_vendorCombo->setCurrentIndex(0);

    m_platformCombo->blockSignals(false);
    m_vendorCombo->blockSignals(false);

    updateAcceptState();
}

void SelectGameDialog::browseForExecutable()
{
    const QString start = m_exeEdit->text().isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        : m_exeEdit->text();

    const QString exe = QFileDialog::getOpenFileName(
        this, tr("Select game executable"), start,
        tr("Executables (*.exe);;All files (*)"));

    if (exe.isEmpty())
        return;

    const QString native = QDir::toNativeSeparators(exe);
    m_exeEdit->setText(native);
    m_exeEdit->setToolTip(native);
    detectGameFromPath(native);
}

void SelectGameDialog::detectGameFromPath(const QString& exePath)
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

    updateAcceptState();
}

void SelectGameDialog::onExecutableChanged(const QString& path)
{
    m_exeEdit->setToolTip(path);
    updateAcceptState();
}

bool SelectGameDialog::validateInputs(QString* outError) const
{
    const QString exePath = m_exeEdit->text().trimmed();

    if (exePath.isEmpty()) {
        if (outError) *outError = tr("Please select a game executable.");
        return false;
    }

    const QFileInfo fi(exePath);
    if (!fi.exists() || !fi.isFile()) {
        if (outError) *outError = tr("The executable does not exist:\n%1").arg(exePath);
        return false;
    }

    if (m_gameCombo->currentData().toString().isEmpty()) {
        if (outError) *outError = tr("Please select a game.");
        return false;
    }

    if (m_platformCombo->currentData().toString().isEmpty()) {
        if (outError) *outError = tr("Please select a platform.");
        return false;
    }

    return true;
}

void SelectGameDialog::updateAcceptState()
{
    QString err;
    const bool ok = validateInputs(&err);

    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);

    if (ok) {
        m_statusLabel->setText(tr("Ready. Click OK to open the game."));
    }
    else {
        m_statusLabel->setText(err);
    }
}

void SelectGameDialog::onAccept()
{
    QString err;
    if (!validateInputs(&err)) {
        QMessageBox::warning(this, tr("Invalid Selection"), err);
        return;
    }

    m_gameSnapshot->ID = m_gameCombo->currentData().toString().toStdString();
    m_gameSnapshot->Platform = m_platformCombo->currentData().toString().toStdString();
    m_gameSnapshot->Vendor = m_vendorCombo->currentData().toString().toStdString();

    auto& s = AppSettings::Get();
    s.SetExecutablePath(m_exeEdit->text());      
    s.SetGameFolder(QFileInfo(m_exeEdit->text()).absolutePath());
    s.SetSelectedGameId(QString::fromStdString(m_gameSnapshot->ID));
    s.SetSelectedPlatform(QString::fromStdString(m_gameSnapshot->Platform));
    s.Sync();

    accept();
}

void SelectGameDialog::restoreFromSettings()
{
    auto& s = AppSettings::Get();

    // --- Executable ---
    const QString lastExe = s.GetExecutablePath();
    if (!lastExe.isEmpty()) {
        const QString native = QDir::toNativeSeparators(lastExe);
        m_exeEdit->setText(native);
        m_exeEdit->setToolTip(native);
    }

    // --- Game ---
    // Select the saved game by data (the ID), not by index — indices
    // can shift if the registry changes between runs.
    const QString savedGame = s.GetSelectedGameId();
    if (!savedGame.isEmpty()) {
        const int idx = m_gameCombo->findData(savedGame);
        if (idx >= 0) {
            // Setting the index fires onGameChanged(), which repopulates
            // platform + vendor for this game.
            m_gameCombo->setCurrentIndex(idx);
        }
    }

    // --- Platform ---
    const QString savedPlatform = s.GetSelectedPlatform();
    if (!savedPlatform.isEmpty()) {
        const int idx = m_platformCombo->findData(savedPlatform);
        if (idx >= 0)
            m_platformCombo->setCurrentIndex(idx);
    }

    // --- Vendor ---
    const QString savedVendor = s.GetSelectedVendor();
    if (!savedVendor.isEmpty()) {
        const int idx = m_vendorCombo->findData(savedVendor);
        if (idx >= 0)
            m_vendorCombo->setCurrentIndex(idx);
    }
    else {
        // Empty saved vendor == "(none)". Make sure that's selected.
        m_vendorCombo->setCurrentIndex(0);
    }

    // If the exe was restored but no game was saved, try detection.
    if (savedGame.isEmpty() && !lastExe.isEmpty())
        detectGameFromPath(lastExe);
}