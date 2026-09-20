#include <MainWindow.hpp>
#include <QApplication>
#include <QFont>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. Window Setup
    setWindowTitle("Telltale Editor v0.0.1a - [Project Name] (English, PC)");
    resize(1280, 720);
    setWindowIcon(QIcon(":/icons/app.png"));   // <-- runtime icon

    // 2. Setup the Menus (File, Edit, Game, etc.)
    setupMenus();

    // 3. Setup the Toolbar (Icons below the menu)
    setupToolBar();

    // 4. Setup the Central Widget (The dark area + bottom controls)
    setupCentralWidget();

    // Optional: Add a status bar
    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenus()
{
    // Create the Menu Bar
    QMenuBar* menuBar = this->menuBar();

    // Add the menus as seen in the screenshot
    menuBar->addMenu("Game");
    menuBar->addMenu("File");
    menuBar->addMenu("Editor");
    menuBar->addMenu("Window");
    menuBar->addMenu("Scripts");
    menuBar->addMenu("Scene");
    menuBar->addMenu("Properties");
    menuBar->addMenu("Choreography");
    menuBar->addMenu("Audio");
    menuBar->addMenu("Input");
    menuBar->addMenu("Dialog");
    menuBar->addMenu("Rules");
    menuBar->addMenu("Style");
    menuBar->addMenu("Vfx");
    menuBar->addMenu("Selected");
    menuBar->addMenu("Minecraft"); // Just for fun, as seen in the image
}

void MainWindow::setupToolBar()
{
    // Create a toolbar
    QToolBar* toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(true); // Usually fixed in these editors
    toolBar->setIconSize(QSize(24, 24));

    // In a real app, you would load actual icons from resources.
    // Here I'm using standard Qt icons as placeholders.
    
    // "New" icon (simulated)
    toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), "New");
    
    // "Save" icon
    toolBar->addAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save");
    
    // "Undo" icon
    toolBar->addAction(style()->standardIcon(QStyle::SP_ArrowBack), "Undo");
    
    // "Redo" icon
    toolBar->addAction(style()->standardIcon(QStyle::SP_ArrowForward), "Redo");

    toolBar->addSeparator();

    // Placeholder icons for the specific editor tools (the green bug, etc.)
    toolBar->addAction(style()->standardIcon(QStyle::SP_ComputerIcon), "Tool 1");
    toolBar->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Tool 2");
    toolBar->addAction(style()->standardIcon(QStyle::SP_DirIcon), "Tool 3");
    
    toolBar->addSeparator();
    
    // Audio icon placeholder
    toolBar->addAction(style()->standardIcon(QStyle::SP_MediaVolume), "Audio");
}

void MainWindow::setupCentralWidget()
{
    // The main container for the center area
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- THE RENDERER RHI AREA ---
    // This is where your RHI (DirectX/Vulkan/OpenGL) surface goes.
    // For now, we make a black widget to simulate the dark render area.
    m_renderSurface = new QWidget(central);
    m_renderSurface->setStyleSheet("background-color: #1e1e1e;"); // Dark grey/black
    m_renderSurface->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- BOTTOM UI OVERLAY (From your screenshot) ---
    // We create a horizontal layout to place the Debug Text, Title, and Choices button
    QWidget* bottomBar = new QWidget(central);
    bottomBar->setFixedHeight(60); // Fixed height for the bottom strip
    bottomBar->setStyleSheet("background-color: #2b2b2b; border-top: 1px solid #444;");
    
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 10, 20, 10);

    // 1. Debug Text Button
    m_debugButton = new QPushButton("Debug Text", bottomBar);
    m_debugButton->setFixedSize(120, 30);
    m_debugButton->setStyleSheet("background-color: #555; color: white; border: 1px solid #777;");

    // 2. Episode Title Label (Center)
    m_titleLabel = new QLabel("Episode 106: A Portal to Mystery", bottomBar);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #dddddd;");

    // 3. Choices Button
    m_choicesButton = new QPushButton("Choices", bottomBar);
    m_choicesButton->setFixedSize(120, 30);
    m_choicesButton->setStyleSheet("background-color: #555; color: white; border: 1px solid #777;");

    // Add widgets to bottom layout
    // We use stretch factors to push the label to the center
    bottomLayout->addWidget(m_debugButton);
    bottomLayout->addStretch(1); 
    bottomLayout->addWidget(m_titleLabel);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(m_choicesButton);

    // Add the render surface and bottom bar to the main layout
    mainLayout->addWidget(m_renderSurface, 1); // 1 = takes up all remaining space
    mainLayout->addWidget(bottomBar);

    setCentralWidget(central);
}