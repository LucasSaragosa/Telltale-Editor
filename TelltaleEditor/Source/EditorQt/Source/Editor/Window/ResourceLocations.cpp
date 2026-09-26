#include <Editor/Window/ResourceLocations.hpp>

#include <QListWidget>
#include <QVBoxLayout>

#include <vector>

ResourceLocationsWindow::ResourceLocationsWindow(QWidget* parent): QDialog(parent)
{
    setWindowTitle("Resource Locations");

    auto* layout = new QVBoxLayout(this);
    auto* listWidget = new QListWidget(this);

    std::vector<String> allLocations{};
    GetEditorResourceRegistry()->GetResourceLocationNames(allLocations);

    for (const auto& location : allLocations)
    {
        listWidget->addItem(QString::fromStdString(location));
    }

    layout->addWidget(listWidget);

    resize(500, 400);
}

ResourceLocationsWindow::~ResourceLocationsWindow()
{}