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
\
SelectGameDialog::SelectGameDialog(Application& app, QWidget* parent) : QDialog(parent), _Application(app)
{
    setWindowTitle(tr("Select Game"));
    resize(640, 320);


}

SelectGameDialog::~SelectGameDialog()
{

}