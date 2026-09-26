#include <Editor/Window/ArchiveBrowser.hpp>

#include <QGridLayout>

ArchiveBrowserWindow::ArchiveBrowserWindow(QWidget* parent /*= nullptr*/)
{
    setWindowTitle("Archive Browser");

    _StatusLabel = new QLabel(tr("Pick a game executable to begin."));
   
    QGridLayout* layout = new QGridLayout;
    layout->addWidget(_StatusLabel);
    layout->addWidget(_StatusLabel);
    layout->addWidget(_StatusLabel);
    layout->addWidget(_StatusLabel);
    setLayout(layout);

}

ArchiveBrowserWindow::~ArchiveBrowserWindow()
{

}