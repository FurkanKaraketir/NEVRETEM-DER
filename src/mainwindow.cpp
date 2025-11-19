#include "mainwindow.h"
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QSettings>
#include <QFile>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QComboBox>
#include <QSpinBox>
#include <QToolButton>
#include <QFrame>
#include <QGridLayout>
#include <QFileDialog>
#include <QDateTime>
#include <algorithm>
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxcellrange.h"
#include "statisticsdialog.h"

using namespace QXlsx;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_firestoreService(new FirestoreService(this))
    , m_storageService(new FirebaseStorageService(this))
    , m_authService(nullptr)
    , m_pendingPhotoDialog(nullptr)
{
    // Set window icon
    QStringList iconPaths = {
        ":/logo.jpg",
        "src/logo.jpg",
        "logo.jpg", 
        QApplication::applicationDirPath() + "/logo.jpg",
        QApplication::applicationDirPath() + "/src/logo.jpg",
        QApplication::applicationDirPath() + "/../../src/logo.jpg"
    };
    
    QIcon windowIcon;
    bool iconLoaded = false;
    
    for (const QString& path : iconPaths) {
        QPixmap iconPixmap(path);
        if (!iconPixmap.isNull()) {
            windowIcon = QIcon(iconPixmap);
            iconLoaded = true;
            qCDebug(dataLog) << "Window icon loaded from:" << path;
            break;
        }
    }
    
    if (iconLoaded) {
        setWindowIcon(windowIcon);
    } else {
        qCWarning(dataLog) << "Could not load window icon from any of the paths";
    }
    
    setupUI();
    setupFilterUI();
    setupMenuBar();
    setupStatusBar();
    setupFirestore();
    setupStorage();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setAuthService(FirebaseAuthService* authService)
{
    qCInfo(dataLog) << "=== Setting authentication service ===";
    m_authService = authService;
    
    if (m_authService) {
        qCInfo(dataLog) << "Auth service set for user:" << m_authService->getUserEmail();
        
        // Update window title with user info
        setWindowTitle(QString("NEVRETEM-DER MBS - %1").arg(m_authService->getUserEmail()));
        
        // Set auth token in Firestore service
        qCDebug(dataLog) << "Setting auth token in Firestore service";
        m_firestoreService->setAuthToken(m_authService->getIdToken());
        
        // Set auth token in Storage service
        qCDebug(dataLog) << "Setting auth token in Storage service";
        m_storageService->setAuthToken(m_authService->getIdToken());
        
        // Connect to token refresh signal to update services
        connect(m_authService, &FirebaseAuthService::tokenRefreshed, [this]() {
            qCDebug(dataLog) << "Auth token refreshed, updating services";
            m_firestoreService->setAuthToken(m_authService->getIdToken());
            m_storageService->setAuthToken(m_authService->getIdToken());
        });
        
        // Load students after authentication is set
        qCInfo(dataLog) << "Triggering initial student data load";
        onRefreshStudents();
    } else {
        qCWarning(dataLog) << "Auth service is null";
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    
    // Header section with logo
    QWidget* headerWidget = new QWidget();
    headerWidget->setObjectName("headerWidget");
    headerWidget->setFixedHeight(80);
    
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    
    // Logo in header
    QLabel* headerLogoLabel = new QLabel();
    headerLogoLabel->setObjectName("headerLogoLabel");
    QStringList logoPaths = {
        "src/logo.jpg",
        "logo.jpg", 
        ":/logo.jpg",
        QApplication::applicationDirPath() + "/logo.jpg",
        QApplication::applicationDirPath() + "/src/logo.jpg",
        QApplication::applicationDirPath() + "/../../src/logo.jpg"
    };
    
    QPixmap headerLogoPixmap;
    bool headerLogoLoaded = false;
    
    for (const QString& path : logoPaths) {
        headerLogoPixmap.load(path);
        if (!headerLogoPixmap.isNull()) {
            headerLogoLoaded = true;
            break;
        }
    }
    
    if (headerLogoLoaded) {
        headerLogoPixmap = headerLogoPixmap.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        headerLogoLabel->setPixmap(headerLogoPixmap);
    } else {
        headerLogoLabel->setText("NEVRETEM-DER");
    }
    
    headerLogoLabel->setFixedSize(60, 60);
    headerLogoLabel->setAlignment(Qt::AlignCenter);
    
    // Header title
    QLabel* headerTitleLabel = new QLabel("NEVRETEM-DER MBS - Mezun Bilgi Sistemi");
    headerTitleLabel->setObjectName("headerTitleLabel");
    headerTitleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    
    headerLayout->addWidget(headerLogoLabel);
    headerLayout->addWidget(headerTitleLabel);
    headerLayout->addStretch();
    
    mainLayout->addWidget(headerWidget);
    
    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_mainSplitter);
    
    // Left panel - Student list
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    
    // Search section container
    QWidget* searchSectionWidget = new QWidget();
    searchSectionWidget->setObjectName("searchSection");
    m_searchLayout = new QHBoxLayout(searchSectionWidget);
    m_searchLayout->setSpacing(12);
    m_searchLayout->setContentsMargins(8, 8, 8, 8);
    
    // Search components
    QLabel* searchLabel = new QLabel("Ara:");
    searchLabel->setObjectName("searchLabel");
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Mezun ara...");
    
    m_refreshButton = new QPushButton("Yenile");
    m_refreshButton->setObjectName("refreshButton");
    m_refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    
    // Filter toggle button
    m_filterToggleButton = new QToolButton();
    m_filterToggleButton->setObjectName("filterToggleButton");
    m_filterToggleButton->setText("Filtreler");
    m_filterToggleButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_filterToggleButton->setCheckable(true);
    m_filterToggleButton->setToolTip("Gelişmiş filtreleri göster/gizle");
    m_filterToggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    m_searchLayout->addWidget(searchLabel);
    m_searchLayout->addWidget(m_searchEdit, 1); // Give search edit more space
    m_searchLayout->addWidget(m_filterToggleButton);
    m_searchLayout->addWidget(m_refreshButton);
    m_leftLayout->addWidget(searchSectionWidget);
    
    // Students table
    m_studentsTable = new QTableWidget();
    m_studentsTable->setColumnCount(9);
    QStringList headers = {"Fotoğraf", "Ad", "E-posta", "Alan", "Okul", "Lise Mezuniyet Yılı", "Numara", "Üniversite Mezun Durumu", "Açıklama"};
    m_studentsTable->setHorizontalHeaderLabels(headers);
    m_studentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_studentsTable->setAlternatingRowColors(true);
    m_studentsTable->setSortingEnabled(true);
    m_studentsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Set default row height to accommodate photos better
    m_studentsTable->verticalHeader()->setDefaultSectionSize(80);
    m_studentsTable->verticalHeader()->setMinimumSectionSize(80);
    
    // Adjust column widths
    QHeaderView* header = m_studentsTable->horizontalHeader();
    header->setStretchLastSection(true);
    header->resizeSection(0, 90);  // Photo (increased for better visibility)
    header->resizeSection(1, 150); // Name
    header->resizeSection(2, 200); // Email
    header->resizeSection(3, 200); // Field
    header->resizeSection(4, 150); // School
    header->resizeSection(5, 120); // Lise Mezuniyet Yılı (increased width)
    header->resizeSection(6, 100); // Number
    header->resizeSection(7, 160); // Üniversite Mezun Durumu (increased width for longer text)
    
    m_leftLayout->addWidget(m_studentsTable);
    
    // Button layout
    m_buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("Mezun Ekle");
    m_editButton = new QPushButton("Mezun Düzenle");
    m_deleteButton = new QPushButton("Mezun Sil");
    
    m_addButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    m_editButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_deleteButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_editButton);
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addStretch();
    m_leftLayout->addLayout(m_buttonLayout);
    
    // Right panel - Student details
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    
    m_detailsGroup = new QGroupBox("Mezun Detayları");
    m_detailsLayout = new QFormLayout(m_detailsGroup);
    
    m_nameLabel = new QLabel("-");
    m_emailLabel = new QLabel("-");
    m_descriptionLabel = new QLabel("-");
    m_fieldLabel = new QLabel("-");
    m_schoolLabel = new QLabel("-");
    m_numberLabel = new QLabel("-");
    m_yearLabel = new QLabel("-");
    m_graduationLabel = new QLabel("-");
    m_photoLabel = new QLabel("-");
    
    m_nameLabel->setWordWrap(true);
    m_emailLabel->setWordWrap(true);
    m_descriptionLabel->setWordWrap(true);
    m_fieldLabel->setWordWrap(true);
    m_schoolLabel->setWordWrap(true);
    
    m_detailsLayout->addRow("Ad:", m_nameLabel);
    m_detailsLayout->addRow("E-posta:", m_emailLabel);
    m_detailsLayout->addRow("Açıklama:", m_descriptionLabel);
    m_detailsLayout->addRow("Alan:", m_fieldLabel);
    m_detailsLayout->addRow("Okul:", m_schoolLabel);
    m_detailsLayout->addRow("Numara:", m_numberLabel);
    m_detailsLayout->addRow("Lise Mezuniyet Yılı:", m_yearLabel);
    m_detailsLayout->addRow("Üniversite Mezun Durumu:", m_graduationLabel);
    m_detailsLayout->addRow("Fotoğraf URL:", m_photoLabel);
    
    // Add actual photo display
    m_detailsPhotoLabel = new QLabel();
    m_detailsPhotoLabel->setProperty("class", "detailsPhotoLabelEmpty");
    m_detailsPhotoLabel->setFixedSize(200, 200);
    m_detailsPhotoLabel->setAlignment(Qt::AlignCenter);
    m_detailsPhotoLabel->setScaledContents(true);
    m_detailsPhotoLabel->setText("Fotoğraf Yok");
    m_detailsLayout->addRow("Fotoğraf:", m_detailsPhotoLabel);
    
    m_rightLayout->addWidget(m_detailsGroup);
    m_rightLayout->addStretch();
    
    // Add panels to splitter
    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_rightPanel);
    m_mainSplitter->setSizes({600, 300});
    
    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshStudents);
    connect(m_filterToggleButton, &QToolButton::toggled, this, &MainWindow::onToggleFilters);
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::onAddStudent);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::onEditStudent);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteStudent);
    connect(m_studentsTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onTableItemDoubleClicked);
    connect(m_studentsTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onTableContextMenu);
    connect(m_studentsTable, &QTableWidget::itemSelectionChanged, [this]() {
        bool hasSelection = m_studentsTable->currentRow() >= 0;
        m_editButton->setEnabled(hasSelection);
        m_deleteButton->setEnabled(hasSelection);
        
        if (hasSelection) {
            Student student = getStudentFromRow(m_studentsTable->currentRow());
            updateStudentDetails(student);
        } else {
            clearStudentDetails();
        }
    });
}

void MainWindow::setupFilterUI()
{
    // Create filter frame
    m_filterFrame = new QFrame();
    m_filterFrame->setObjectName("filterFrame");
    m_filterFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_filterFrame->setVisible(false); // Initially hidden
    
    m_filterLayout = new QGridLayout(m_filterFrame);
    m_filterLayout->setSpacing(15);
    m_filterLayout->setContentsMargins(20, 20, 20, 20);
    
    // Row 0: Name and Email filters
    QLabel* nameLabel = new QLabel("Ad:");
    nameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_filterLayout->addWidget(nameLabel, 0, 0);
    m_nameFilterEdit = new QLineEdit();
    m_nameFilterEdit->setPlaceholderText("Ada göre filtrele...");
    m_filterLayout->addWidget(m_nameFilterEdit, 0, 1);
    
    QLabel* emailLabel = new QLabel("E-posta:");
    emailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_filterLayout->addWidget(emailLabel, 0, 2);
    m_emailFilterEdit = new QLineEdit();
    m_emailFilterEdit->setPlaceholderText("E-postaya göre filtrele...");
    m_filterLayout->addWidget(m_emailFilterEdit, 0, 3);
    
    // Row 1: Field and School filters
    QLabel* fieldLabel = new QLabel("Alan:");
    fieldLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_filterLayout->addWidget(fieldLabel, 1, 0);
    m_fieldFilterCombo = new QComboBox();
    m_fieldFilterCombo->addItem("Tümü", "");
    m_fieldFilterCombo->setEditable(false);
    m_filterLayout->addWidget(m_fieldFilterCombo, 1, 1);
    
    QLabel* schoolLabel = new QLabel("Okul:");
    schoolLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_filterLayout->addWidget(schoolLabel, 1, 2);
    m_schoolFilterCombo = new QComboBox();
    m_schoolFilterCombo->addItem("Tümü", "");
    m_schoolFilterCombo->setEditable(false);
    m_filterLayout->addWidget(m_schoolFilterCombo, 1, 3);
    
    // Row 2: Year range and graduation status
    QLabel* yearLabel = new QLabel("Mezuniyet Yılı:");
    yearLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_filterLayout->addWidget(yearLabel, 2, 0);
    
    QWidget* yearRangeWidget = new QWidget();
    QHBoxLayout* yearLayout = new QHBoxLayout(yearRangeWidget);
    yearLayout->setContentsMargins(0, 0, 0, 0);
    yearLayout->setSpacing(8);
    
    m_yearFromSpinBox = new QSpinBox();
    m_yearFromSpinBox->setRange(1990, 2100);
    m_yearFromSpinBox->setValue(2000);
    m_yearFromSpinBox->setSpecialValueText("Başlangıç");
    m_yearFromSpinBox->setMinimum(0);
    
    QLabel* dashLabel = new QLabel("-");
    dashLabel->setObjectName("yearDashLabel");
    dashLabel->setAlignment(Qt::AlignCenter);
    
    m_yearToSpinBox = new QSpinBox();
    m_yearToSpinBox->setRange(1990, 2100);
    m_yearToSpinBox->setValue(2100);
    m_yearToSpinBox->setSpecialValueText("Bitiş");
    m_yearToSpinBox->setMaximum(9999);
    
    yearLayout->addWidget(m_yearFromSpinBox);
    yearLayout->addWidget(dashLabel);
    yearLayout->addWidget(m_yearToSpinBox);
    
    m_filterLayout->addWidget(yearRangeWidget, 2, 1);
    
    QLabel* graduationLabel = new QLabel("Üniversite Mezun Durumu:");
    graduationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_filterLayout->addWidget(graduationLabel, 2, 2);
    m_graduationFilterCombo = new QComboBox();
    m_graduationFilterCombo->addItem("Tümü", -1);
    m_graduationFilterCombo->addItem("Mezun", 1);
    m_graduationFilterCombo->addItem("Aktif (Devam Ediyor)", 0);
    m_graduationFilterCombo->addItem("Üniversiteye Gitmedi", 2);
    m_filterLayout->addWidget(m_graduationFilterCombo, 2, 3);
    
    // Row 3: Clear filters button with enhanced styling
    m_clearFiltersButton = new QPushButton("🗑️ Filtreleri Temizle");
    m_clearFiltersButton->setObjectName("clearFiltersButton");
    m_clearFiltersButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    m_filterLayout->addWidget(m_clearFiltersButton, 3, 0, 1, 4);
    
    // Set column stretch to make the layout more balanced
    m_filterLayout->setColumnStretch(1, 1);
    m_filterLayout->setColumnStretch(3, 1);
    
    // Add filter frame to left layout (after search, before table)
    m_leftLayout->insertWidget(1, m_filterFrame);
    
    // Connect filter signals
    connect(m_nameFilterEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(m_emailFilterEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(m_fieldFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
    connect(m_schoolFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
    connect(m_graduationFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
    connect(m_yearFromSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onFilterChanged);
    connect(m_yearToSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onFilterChanged);
    connect(m_clearFiltersButton, &QPushButton::clicked, this, &MainWindow::onClearFilters);
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&Dosya");
    
    m_settingsAction = new QAction("&Ayarlar", this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    connect(m_settingsAction, &QAction::triggered, [this]() {
        bool ok;
        QString projectId = QInputDialog::getText(this, "Firestore Ayarları", 
                                                  "Proje ID:", QLineEdit::Normal, 
                                                  "", &ok);
        if (ok && !projectId.isEmpty()) {
            m_firestoreService->setProjectId(projectId);
            QString configPath = QApplication::applicationDirPath() + "/../../config.ini";
            if (!QFile::exists(configPath)) {
                configPath = "config.ini";
            }
            QSettings settings(configPath, QSettings::IniFormat);
            settings.setValue("firestore/projectId", projectId);
        }
    });
    fileMenu->addAction(m_settingsAction);
    
    fileMenu->addSeparator();
    
    // Excel import/export actions
    m_exportExcelAction = new QAction("Excel'e &Aktar", this);
    m_exportExcelAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(m_exportExcelAction, &QAction::triggered, this, &MainWindow::onExportToExcel);
    fileMenu->addAction(m_exportExcelAction);
    
    m_importExcelAction = new QAction("Excel'den &İçe Aktar", this);
    m_importExcelAction->setShortcut(QKeySequence("Ctrl+I"));
    connect(m_importExcelAction, &QAction::triggered, this, &MainWindow::onImportFromExcel);
    fileMenu->addAction(m_importExcelAction);
    
    fileMenu->addSeparator();
    
    m_signOutAction = new QAction("Çı&kış Yap", this);
    connect(m_signOutAction, &QAction::triggered, this, &MainWindow::onSignOut);
    fileMenu->addAction(m_signOutAction);
    
    fileMenu->addSeparator();
    
    m_exitAction = new QAction("Ç&ık", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(m_exitAction);
    
    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&Görünüm");
    
    m_statisticsAction = new QAction("&İstatistik Paneli", this);
    m_statisticsAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(m_statisticsAction, &QAction::triggered, this, &MainWindow::onShowStatistics);
    viewMenu->addAction(m_statisticsAction);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Yardım");
    
    m_aboutAction = new QAction("&Hakkında", this);
    connect(m_aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "NEVRETEM-DER MBS Hakkında", 
                          "NEVRETEM-DER MBS (Mezun Bilgi Sistemi) v1.0\n\nRecep Tayyip Erdoğan Anadolu İmam Hatip Lisesi\nMezunları ve Mensupları Derneği\n\nFirestore entegrasyonu ile mezun verilerini yönetmek için bir uygulamadır.\nGeliştirici İletişim Bilgileri: FURKAN KARAKETİR +90 551 145 09 68");
    });
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Hazır");
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_progressBar);
}

void MainWindow::setupFirestore()
{
    // Connect Firestore signals
    connect(m_firestoreService, &FirestoreService::studentsReceived, this, &MainWindow::onStudentsReceived);
    connect(m_firestoreService, &FirestoreService::studentAdded, this, &MainWindow::onStudentAdded);
    connect(m_firestoreService, &FirestoreService::studentUpdated, this, &MainWindow::onStudentUpdated);
    connect(m_firestoreService, &FirestoreService::studentDeleted, this, &MainWindow::onStudentDeleted);
    connect(m_firestoreService, &FirestoreService::errorOccurred, this, &MainWindow::onFirestoreError);
    
    // Load settings from config.ini file
    QString configPath = QApplication::applicationDirPath() + "/../../config.ini";
    if (!QFile::exists(configPath)) {
        configPath = "config.ini";
    }
    QSettings settings(configPath, QSettings::IniFormat);
    QString projectId = settings.value("firestore/projectId", "").toString();
    QString apiKey = settings.value("firestore/apiKey", "").toString();
    
    if (!projectId.isEmpty()) {
        m_firestoreService->setProjectId(projectId);
    }
    if (!apiKey.isEmpty()) {
        m_firestoreService->setApiKey(apiKey);
    }
}

void MainWindow::setupStorage()
{
    // Connect Storage service signals
    connect(m_storageService, &FirebaseStorageService::imageLoaded, this, &MainWindow::onImageLoaded);
    connect(m_storageService, &FirebaseStorageService::imageLoadFailed, this, &MainWindow::onImageLoadFailed);
    
    // Load settings from config.ini file
    QString configPath = QApplication::applicationDirPath() + "/../../config.ini";
    if (!QFile::exists(configPath)) {
        configPath = "config.ini";
    }
    QSettings settings(configPath, QSettings::IniFormat);
    QString projectId = settings.value("firestore/projectId", "").toString();
    QString apiKey = settings.value("firestore/apiKey", "").toString();
    
    if (!projectId.isEmpty()) {
        m_storageService->setProjectId(projectId);
    }
    if (!apiKey.isEmpty()) {
        m_storageService->setApiKey(apiKey);
    }
}

void MainWindow::onAddStudent()
{
    StudentDialog* dialog = new StudentDialog(this);
    dialog->setStorageService(m_storageService);
    if (dialog->exec() == QDialog::Accepted) {
        Student student = dialog->getStudent();
        
        // Store the dialog pointer for deferred photo upload
        m_pendingPhotoDialog = dialog;
        
        showLoadingState(true);
        m_firestoreService->addStudent(student);
    } else {
        // Clean up dialog if cancelled
        dialog->deleteLater();
    }
}

void MainWindow::onEditStudent()
{
    int currentRow = m_studentsTable->currentRow();
    if (currentRow < 0) return;
    
    Student student = getStudentFromRow(currentRow);
    StudentDialog dialog(student, this);
    dialog.setStorageService(m_storageService);
    if (dialog.exec() == QDialog::Accepted) {
        Student updatedStudent = dialog.getStudent();
        showLoadingState(true);
        m_firestoreService->updateStudent(updatedStudent);
    }
}

void MainWindow::onDeleteStudent()
{
    int currentRow = m_studentsTable->currentRow();
    if (currentRow < 0) return;
    
    Student student = getStudentFromRow(currentRow);
    int ret = QMessageBox::question(this, "Mezun Sil", 
                                   QString("Mezun '%1' silmek istediğinizden emin misiniz?").arg(student.getName()),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        showLoadingState(true);
        
        // Delete associated photo first if it exists
        if (!student.getPhotoURL().isEmpty() && m_storageService) {
            // Generate the expected storage path based on student ID
            QString studentId = student.getId();
            // Try common image extensions
            QStringList extensions = {"jpg", "jpeg", "png", "gif", "bmp"};
            for (const QString& ext : extensions) {
                QString storagePath = QString("student_photos/%1.%2").arg(studentId, ext);
                m_storageService->deleteFile(storagePath);
            }
        }
        
        m_firestoreService->deleteStudent(student.getId());
    }
}

void MainWindow::onRefreshStudents()
{
    qCInfo(dataLog) << "=== Refreshing student data ===";
    qCDebug(dataLog) << "Current student count:" << m_allStudents.size();
    qCDebug(dataLog) << "Current filtered count:" << m_filteredStudents.size();
    
    showLoadingState(true);
    qCInfo(dataLog) << "Requesting all students from Firestore";
    m_firestoreService->getAllStudents();
}

void MainWindow::onSearchTextChanged()
{
    QString searchText = m_searchEdit->text();
    qCDebug(dataLog) << "Search text changed to:" << (searchText.isEmpty() ? "(empty)" : searchText);
    filterStudents();
}

void MainWindow::onToggleFilters()
{
    bool visible = m_filterToggleButton->isChecked();
    m_filterFrame->setVisible(visible);
    
    if (visible) {
        m_filterToggleButton->setText("Filtreleri Gizle");
        // Populate filter dropdowns with unique values from current data
        populateFilterDropdowns();
    } else {
        m_filterToggleButton->setText("Filtreler");
    }
    
    qCDebug(dataLog) << "Filter panel visibility changed to:" << visible;
}

void MainWindow::onFilterChanged()
{
    qCDebug(dataLog) << "Filter criteria changed, applying filters";
    filterStudents();
}

void MainWindow::onClearFilters()
{
    qCDebug(dataLog) << "Clearing all filter criteria";
    
    // Clear text filters
    m_nameFilterEdit->clear();
    m_emailFilterEdit->clear();
    
    // Reset combos
    m_fieldFilterCombo->setCurrentIndex(0);
    m_schoolFilterCombo->setCurrentIndex(0);
    m_graduationFilterCombo->setCurrentIndex(0);
    
    // Reset spinboxes
    m_yearFromSpinBox->setValue(0); // Special value "Başlangıç"
    m_yearToSpinBox->setValue(9999); // Special value "Bitiş"
}

void MainWindow::onShowStatistics()
{
    StatisticsDialog dialog(m_allStudents, this);
    dialog.exec();
}


void MainWindow::onTableItemDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0) {
        onEditStudent();
    }
}

void MainWindow::onTableContextMenu(const QPoint& pos)
{
    QMenu contextMenu(this);
    
    QAction* editAction = contextMenu.addAction("Düzenle");
    QAction* deleteAction = contextMenu.addAction("Sil");
    contextMenu.addSeparator();
    QAction* refreshAction = contextMenu.addAction("Yenile");
    
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditStudent);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteStudent);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshStudents);
    
    bool hasSelection = m_studentsTable->currentRow() >= 0;
    editAction->setEnabled(hasSelection);
    deleteAction->setEnabled(hasSelection);
    
    contextMenu.exec(m_studentsTable->mapToGlobal(pos));
}

void MainWindow::onStudentsReceived(const QList<Student>& students)
{
    qCInfo(dataLog) << "=== Received students from Firestore ===";
    qCInfo(dataLog) << "Received" << students.size() << "students";
    
    showLoadingState(false);
    
    // Log some details about the received students
    if (!students.isEmpty()) {
        qCDebug(dataLog) << "First student:" << students.first().getName() << "(" << students.first().getEmail() << ")";
        if (students.size() > 1) {
            qCDebug(dataLog) << "Last student:" << students.last().getName() << "(" << students.last().getEmail() << ")";
        }
    }
    
    // Count students by graduation status
    int graduatedCount = 0;
    int activeCount = 0;
    int noUniversityCount = 0;
    for (const Student& student : students) {
        if (student.getSchool() == "Üniversiteye gitmedi") {
            noUniversityCount++;
        } else if (student.getGraduation()) {
            graduatedCount++;
        } else {
            activeCount++;
        }
    }
    
    qCInfo(dataLog) << "Student breakdown - Active:" << activeCount << "Graduated:" << graduatedCount << "No University:" << noUniversityCount;
    
    m_allStudents = students;
    qCDebug(dataLog) << "Updated m_allStudents, size:" << m_allStudents.size();
    
    // Update filter dropdowns with new data
    if (m_filterFrame && m_filterFrame->isVisible()) {
        qCDebug(dataLog) << "Updating filter dropdowns with new student data";
        populateFilterDropdowns();
    }
    
    qCInfo(dataLog) << "Applying filters to student list";
    filterStudents();
    
    QString statusText = QString("%1 mezun yüklendi").arg(students.size());
    m_statusLabel->setText(statusText);
    qCInfo(dataLog) << "Status updated:" << statusText;
}

void MainWindow::onStudentAdded(const Student& student)
{
    qCInfo(dataLog) << "=== Student added successfully ===";
    qCInfo(dataLog) << "Added student:" << student.getName() << "(" << student.getEmail() << ")";
    qCDebug(dataLog) << "Student ID:" << student.getId();
    
    showLoadingState(false);
    
    qCDebug(dataLog) << "Current student count before adding:" << m_allStudents.size();
    m_allStudents.append(student);
    qCDebug(dataLog) << "Current student count after adding:" << m_allStudents.size();
    
    // Handle deferred photo upload if there's a pending dialog
    if (m_pendingPhotoDialog) {
        qCInfo(dataLog) << "Uploading deferred photo for student ID:" << student.getId();
        
        // Connect the completion signal
        connect(m_pendingPhotoDialog, &StudentDialog::deferredUploadCompleted,
                this, &MainWindow::onDeferredUploadCompleted, Qt::UniqueConnection);
        
        m_pendingPhotoDialog->uploadDeferredPhoto(student.getId());
        // Don't delete the dialog immediately - let it clean up after upload completes
        // The dialog will be cleaned up when the upload finishes or fails
    }
    
    qCInfo(dataLog) << "Refreshing filtered student list";
    filterStudents();
    
    m_statusLabel->setText("Mezun başarıyla eklendi");
    qCInfo(dataLog) << "Student addition process completed";
}

void MainWindow::onStudentUpdated(const Student& student)
{
    qCInfo(dataLog) << "=== Student updated successfully ===";
    qCInfo(dataLog) << "Updated student:" << student.getName() << "(" << student.getEmail() << ")";
    qCDebug(dataLog) << "Student ID:" << student.getId();
    
    showLoadingState(false);
    
    // Update the student in our list
    bool found = false;
    for (int i = 0; i < m_allStudents.size(); ++i) {
        if (m_allStudents[i].getId() == student.getId()) {
            qCDebug(dataLog) << "Found student at index" << i << "- updating";
            qCDebug(dataLog) << "Old name:" << m_allStudents[i].getName() << "New name:" << student.getName();
            m_allStudents[i] = student;
            found = true;
            break;
        }
    }
    
    if (!found) {
        qCWarning(dataLog) << "Could not find student with ID" << student.getId() << "in local list";
    } else {
        qCDebug(dataLog) << "Student updated in local list";
    }
    
    qCInfo(dataLog) << "Refreshing filtered student list";
    filterStudents();
    
    m_statusLabel->setText("Mezun başarıyla güncellendi");
    qCInfo(dataLog) << "Student update process completed";
}

void MainWindow::onStudentDeleted(const QString& studentId)
{
    qCInfo(dataLog) << "=== Student deleted successfully ===";
    qCInfo(dataLog) << "Deleted student ID:" << studentId;
    
    showLoadingState(false);
    
    // Remove the student from our list
    bool found = false;
    QString deletedStudentName;
    for (int i = 0; i < m_allStudents.size(); ++i) {
        if (m_allStudents[i].getId() == studentId) {
            deletedStudentName = m_allStudents[i].getName();
            qCDebug(dataLog) << "Found student at index" << i << "- removing:" << deletedStudentName;
            qCDebug(dataLog) << "Student count before removal:" << m_allStudents.size();
            m_allStudents.removeAt(i);
            qCDebug(dataLog) << "Student count after removal:" << m_allStudents.size();
            found = true;
            break;
        }
    }
    
    if (!found) {
        qCWarning(dataLog) << "Could not find student with ID" << studentId << "in local list";
    } else {
        qCInfo(dataLog) << "Successfully removed student:" << deletedStudentName;
    }
    
    qCInfo(dataLog) << "Refreshing filtered student list";
    filterStudents();
    
    qCDebug(dataLog) << "Clearing student details panel";
    clearStudentDetails();
    
    m_statusLabel->setText("Mezun başarıyla silindi");
    qCInfo(dataLog) << "Student deletion process completed";
}

void MainWindow::onFirestoreError(const QString& error)
{
    qCCritical(dataLog) << "=== Firestore error occurred ===";
    qCCritical(dataLog) << "Error message:" << error;
    
    showLoadingState(false);
    
    // Clean up pending photo dialog if there's an error
    if (m_pendingPhotoDialog) {
        qCWarning(dataLog) << "Cleaning up pending photo dialog due to error";
        m_pendingPhotoDialog->deleteLater();
        m_pendingPhotoDialog = nullptr;
    }
    
    qCWarning(dataLog) << "Showing error dialog to user";
    QMessageBox::critical(this, "Firestore Hatası", error);
    
    m_statusLabel->setText("Hata oluştu");
    qCInfo(dataLog) << "Error handling completed";
}

void MainWindow::populateTable(const QList<Student>& students)
{
    qCDebug(dataLog) << "=== Populating table ===";
    qCInfo(dataLog) << "Populating table with" << students.size() << "students";
    
    // Clear photo labels map to prevent crashes from accessing destroyed widgets
    if (!m_photoLabels.isEmpty()) {
        qCDebug(dataLog) << "Clearing" << m_photoLabels.size() << "photo label mappings";
        m_photoLabels.clear();
    }
    
    // Save column widths before clearing
    QList<int> columnWidths;
    QHeaderView* header = m_studentsTable->horizontalHeader();
    for (int i = 0; i < m_studentsTable->columnCount(); ++i) {
        columnWidths.append(header->sectionSize(i));
    }
    
    // Clear current selection to avoid issues
    m_studentsTable->clearSelection();
    
    // Temporarily disable sorting to prevent column width issues
    bool sortingWasEnabled = m_studentsTable->isSortingEnabled();
    m_studentsTable->setSortingEnabled(false);
    
    // Clear all existing rows first (including cell widgets)
    m_studentsTable->setRowCount(0);
    qCDebug(dataLog) << "Table cleared";
    
    m_studentsTable->setRowCount(students.size());
    qCDebug(dataLog) << "Table row count set to:" << students.size();
    
    for (int i = 0; i < students.size(); ++i) {
        const Student& student = students[i];
        
        if (i < 5 || i == students.size() - 1) { // Log first 5 and last student
            qCDebug(dataLog) << "Adding student" << (i + 1) << ":" << student.getName() << "(" << student.getEmail() << ")";
        } else if (i == 5) {
            qCDebug(dataLog) << "... (logging only first 5 and last student)";
        }
        
        // Photo column
        QLabel* photoLabel = new QLabel();
        photoLabel->setFixedSize(70, 70);
        photoLabel->setAlignment(Qt::AlignCenter);
        photoLabel->setScaledContents(true);
        
        if (!student.getPhotoURL().isEmpty()) {
            photoLabel->setProperty("class", "photoLabel");
            loadStudentPhoto(photoLabel, student.getPhotoURL());
        } else {
            photoLabel->setProperty("class", "photoLabelEmpty");
            photoLabel->setText("Fotoğraf Yok");
        }
        
        m_studentsTable->setCellWidget(i, 0, photoLabel);
        m_studentsTable->setItem(i, 1, new QTableWidgetItem(student.getName()));
        m_studentsTable->setItem(i, 2, new QTableWidgetItem(student.getEmail()));
        m_studentsTable->setItem(i, 3, new QTableWidgetItem(student.getField()));
        m_studentsTable->setItem(i, 4, new QTableWidgetItem(student.getSchool()));
        m_studentsTable->setItem(i, 5, new QTableWidgetItem(QString::number(student.getYear())));
        m_studentsTable->setItem(i, 6, new QTableWidgetItem(student.getNumber()));
        // Determine graduation status display
        QString graduationStatus;
        if (student.getSchool() == "Üniversiteye gitmedi") {
            graduationStatus = "Üniversiteye Gitmedi";
        } else if (student.getGraduation()) {
            graduationStatus = "Mezun";
        } else {
            graduationStatus = "Aktif";
        }
        m_studentsTable->setItem(i, 7, new QTableWidgetItem(graduationStatus));
        m_studentsTable->setItem(i, 8, new QTableWidgetItem(student.getDescription()));
        
        // Store student ID in the name item's data
        m_studentsTable->item(i, 1)->setData(Qt::UserRole, student.getId());
    }
    
    // Restore column widths
    for (int i = 0; i < qMin(columnWidths.size(), m_studentsTable->columnCount()); ++i) {
        header->resizeSection(i, columnWidths[i]);
    }
    
    // Re-enable sorting if it was enabled before
    if (sortingWasEnabled) {
        m_studentsTable->setSortingEnabled(true);
    }
    
    qCInfo(dataLog) << "Table population completed - rows:" << m_studentsTable->rowCount() << "columns:" << m_studentsTable->columnCount();
}

void MainWindow::updateStudentDetails(const Student& student)
{
    m_nameLabel->setText(student.getName());
    m_emailLabel->setText(QString("<a href=\"mailto:%1\">%1</a>").arg(student.getEmail()));
    m_emailLabel->setOpenExternalLinks(true);
    m_descriptionLabel->setText(student.getDescription());
    m_fieldLabel->setText(student.getField());
    m_schoolLabel->setText(student.getSchool());
    m_numberLabel->setText(student.getNumber());
    m_yearLabel->setText(QString::number(student.getYear()));
    // Determine graduation status display for details panel
    QString graduationStatus;
    if (student.getSchool() == "Üniversiteye gitmedi") {
        graduationStatus = "Üniversiteye Gitmedi";
    } else if (student.getGraduation()) {
        graduationStatus = "Mezun";
    } else {
        graduationStatus = "Aktif (Devam Ediyor)";
    }
    m_graduationLabel->setText(graduationStatus);
    m_photoLabel->setText(student.getPhotoURL().isEmpty() ? "-" : 
                         QString("<a href=\"%1\">View Photo</a>").arg(student.getPhotoURL()));
    m_photoLabel->setOpenExternalLinks(true);
    
    // Load photo in details panel
    if (!student.getPhotoURL().isEmpty()) {
        m_currentDetailsPhotoUrl = student.getPhotoURL();
        loadStudentDetailsPhoto(student.getPhotoURL());
    } else {
        m_currentDetailsPhotoUrl.clear();
        m_detailsPhotoLabel->clear();
        m_detailsPhotoLabel->setProperty("class", "detailsPhotoLabelEmpty");
        m_detailsPhotoLabel->style()->unpolish(m_detailsPhotoLabel);
        m_detailsPhotoLabel->style()->polish(m_detailsPhotoLabel);
        m_detailsPhotoLabel->setText("Fotoğraf Yok");
    }
}

void MainWindow::clearStudentDetails()
{
    m_nameLabel->setText("-");
    m_emailLabel->setText("-");
    m_descriptionLabel->setText("-");
    m_fieldLabel->setText("-");
    m_schoolLabel->setText("-");
    m_numberLabel->setText("-");
    m_yearLabel->setText("-");
    m_graduationLabel->setText("-");
    m_photoLabel->setText("-");
    
    // Clear details photo
    m_currentDetailsPhotoUrl.clear();
    m_detailsPhotoLabel->clear();
    m_detailsPhotoLabel->setProperty("class", "detailsPhotoLabelEmpty");
    m_detailsPhotoLabel->style()->unpolish(m_detailsPhotoLabel);
    m_detailsPhotoLabel->style()->polish(m_detailsPhotoLabel);
    m_detailsPhotoLabel->setText("Fotoğraf Yok");
}

void MainWindow::populateFilterDropdowns()
{
    if (m_allStudents.isEmpty()) {
        qCDebug(dataLog) << "No students available to populate filter dropdowns";
        return;
    }
    
    qCDebug(dataLog) << "Populating filter dropdowns with unique values";
    
    // Collect unique values
    QSet<QString> uniqueFields;
    QSet<QString> uniqueSchools;
    
    for (const Student& student : m_allStudents) {
        if (!student.getField().isEmpty()) {
            uniqueFields.insert(student.getField());
        }
        if (!student.getSchool().isEmpty()) {
            uniqueSchools.insert(student.getSchool());
        }
    }
    
    // Populate field filter
    QString currentField = m_fieldFilterCombo->currentData().toString();
    m_fieldFilterCombo->clear();
    m_fieldFilterCombo->addItem("Tümü", "");
    
    QStringList sortedFields = uniqueFields.values();
    sortedFields.sort();
    for (const QString& field : sortedFields) {
        m_fieldFilterCombo->addItem(field, field);
    }
    
    // Restore previous selection if it still exists
    int fieldIndex = m_fieldFilterCombo->findData(currentField);
    if (fieldIndex >= 0) {
        m_fieldFilterCombo->setCurrentIndex(fieldIndex);
    }
    
    // Populate school filter
    QString currentSchool = m_schoolFilterCombo->currentData().toString();
    m_schoolFilterCombo->clear();
    m_schoolFilterCombo->addItem("Tümü", "");
    
    QStringList sortedSchools = uniqueSchools.values();
    sortedSchools.sort();
    for (const QString& school : sortedSchools) {
        m_schoolFilterCombo->addItem(school, school);
    }
    
    // Restore previous selection if it still exists
    int schoolIndex = m_schoolFilterCombo->findData(currentSchool);
    if (schoolIndex >= 0) {
        m_schoolFilterCombo->setCurrentIndex(schoolIndex);
    }
    
    qCDebug(dataLog) << "Filter dropdowns populated - Fields:" << uniqueFields.size() << "Schools:" << uniqueSchools.size();
}

void MainWindow::filterStudents()
{
    QString searchText = m_searchEdit->text().toLower();
    qCDebug(dataLog) << "=== Filtering students ===";
    qCDebug(dataLog) << "Search text:" << (searchText.isEmpty() ? "(empty)" : searchText);
    qCDebug(dataLog) << "Total students to filter:" << m_allStudents.size();
    
    // Get filter criteria
    QString nameFilter = m_nameFilterEdit ? m_nameFilterEdit->text().toLower() : "";
    QString emailFilter = m_emailFilterEdit ? m_emailFilterEdit->text().toLower() : "";
    QString fieldFilter = m_fieldFilterCombo ? m_fieldFilterCombo->currentData().toString() : "";
    QString schoolFilter = m_schoolFilterCombo ? m_schoolFilterCombo->currentData().toString() : "";
    int graduationFilter = m_graduationFilterCombo ? m_graduationFilterCombo->currentData().toInt() : -1;
    int yearFrom = m_yearFromSpinBox ? m_yearFromSpinBox->value() : 0;
    int yearTo = m_yearToSpinBox ? m_yearToSpinBox->value() : 9999;
    
    // Handle special values for year range
    if (yearFrom == 0) yearFrom = 1900; // Minimum reasonable year
    if (yearTo == 9999) yearTo = 2100;  // Maximum reasonable year
    
    qCDebug(dataLog) << "Filter criteria - Name:" << nameFilter << "Email:" << emailFilter 
                     << "Field:" << fieldFilter << "School:" << schoolFilter 
                     << "Graduation:" << graduationFilter << "Year range:" << yearFrom << "-" << yearTo;
    
    m_filteredStudents.clear();
    
    bool hasAnyFilter = !searchText.isEmpty() || !nameFilter.isEmpty() || !emailFilter.isEmpty() ||
                       !fieldFilter.isEmpty() || !schoolFilter.isEmpty() || graduationFilter != -1 ||
                       yearFrom > 1900 || yearTo < 2100;
    
    if (!hasAnyFilter) {
        qCDebug(dataLog) << "No filters applied - showing all students";
        m_filteredStudents = m_allStudents;
    } else {
        qCDebug(dataLog) << "Applying filters";
        int matchCount = 0;
        for (const Student& student : m_allStudents) {
            bool matches = true;
            
            // Apply search text filter (searches across multiple fields)
            if (!searchText.isEmpty()) {
                bool searchMatches = student.getName().toLower().contains(searchText) ||
                                   student.getEmail().toLower().contains(searchText) ||
                                   student.getField().toLower().contains(searchText) ||
                                   student.getSchool().toLower().contains(searchText) ||
                                   student.getDescription().toLower().contains(searchText);
                if (!searchMatches) {
                    matches = false;
                }
            }
            
            // Apply specific field filters
            if (matches && !nameFilter.isEmpty()) {
                if (!student.getName().toLower().contains(nameFilter)) {
                    matches = false;
                }
            }
            
            if (matches && !emailFilter.isEmpty()) {
                if (!student.getEmail().toLower().contains(emailFilter)) {
                    matches = false;
                }
            }
            
            if (matches && !fieldFilter.isEmpty()) {
                if (student.getField() != fieldFilter) {
                    matches = false;
                }
            }
            
            if (matches && !schoolFilter.isEmpty()) {
                if (student.getSchool() != schoolFilter) {
                    matches = false;
                }
            }
            
            if (matches && graduationFilter != -1) {
                bool isGraduated = student.getGraduation();
                QString school = student.getSchool();
                bool didNotAttendUniversity = (school == "Üniversiteye gitmedi");
                
                if (graduationFilter == 1) {
                    // Mezun - graduated from university
                    if (!isGraduated || didNotAttendUniversity) {
                        matches = false;
                    }
                } else if (graduationFilter == 0) {
                    // Aktif (Devam Ediyor) - currently studying
                    if (isGraduated || didNotAttendUniversity) {
                        matches = false;
                    }
                } else if (graduationFilter == 2) {
                    // Üniversiteye Gitmedi - didn't attend university
                    if (!didNotAttendUniversity) {
                        matches = false;
                    }
                }
            }
            
            if (matches) {
                int studentYear = student.getYear();
                // Only apply year range filter if student attended university
                bool didNotAttendUniversity = (student.getSchool() == "Üniversiteye gitmedi");
                if (!didNotAttendUniversity && (studentYear < yearFrom || studentYear > yearTo)) {
                    matches = false;
                }
            }
            
            if (matches) {
                m_filteredStudents.append(student);
                matchCount++;
            }
        }
        qCInfo(dataLog) << "Filters applied - found" << matchCount << "matches out of" << m_allStudents.size() << "students";
    }
    
    // Sort by lastUpdateTime in descending order (newest first)
    std::sort(m_filteredStudents.begin(), m_filteredStudents.end(), 
              [](const Student& a, const Student& b) {
                  return a.getLastUpdateTime() > b.getLastUpdateTime();
              });
    
    qCDebug(dataLog) << "Filtered students count:" << m_filteredStudents.size();
    qCDebug(dataLog) << "Students sorted by lastUpdateTime (newest first)";
    qCDebug(dataLog) << "Populating table with filtered results";
    populateTable(m_filteredStudents);
}

Student MainWindow::getStudentFromRow(int row) const
{
    if (row < 0 || row >= m_studentsTable->rowCount()) {
        return Student();
    }
    
    // Get the student ID from the table item (which moves with sorting)
    QTableWidgetItem* item = m_studentsTable->item(row, 1); // Name column at index 1
    if (!item) {
        return Student();
    }
    
    QString studentId = item->data(Qt::UserRole).toString();
    
    // Find the student in m_filteredStudents by ID
    for (const Student& student : m_filteredStudents) {
        if (student.getId() == studentId) {
            return student;
        }
    }
    
    return Student();
}

int MainWindow::findStudentRow(const QString& studentId) const
{
    for (int i = 0; i < m_studentsTable->rowCount(); ++i) {
        QTableWidgetItem* item = m_studentsTable->item(i, 1); // Name column now at index 1
        if (item && item->data(Qt::UserRole).toString() == studentId) {
            return i;
        }
    }
    return -1;
}

void MainWindow::showLoadingState(bool loading)
{
    qCDebug(dataLog) << "Setting loading state:" << (loading ? "ON" : "OFF");
    m_progressBar->setVisible(loading);
    if (loading) {
        m_progressBar->setRange(0, 0); // Indeterminate progress
        m_statusLabel->setText("Yüklüyor...");
        qCDebug(dataLog) << "Loading indicator activated";
    } else {
        qCDebug(dataLog) << "Loading indicator deactivated";
    }
}

void MainWindow::loadStudentPhoto(QLabel* photoLabel, const QString& photoUrl)
{
    if (photoUrl.isEmpty() || !photoLabel) return;
    
    // Use the authenticated storage service to load the image
    if (m_storageService) {
        // Store the label reference for when the image loads
        m_photoLabels[photoUrl] = photoLabel;
        m_storageService->loadImage(photoUrl);
    } else {
        photoLabel->setProperty("class", "photoLabelEmpty");
        photoLabel->style()->unpolish(photoLabel);
        photoLabel->style()->polish(photoLabel);
        photoLabel->setText("Servis Yok");
    }
}

void MainWindow::loadStudentDetailsPhoto(const QString& photoUrl)
{
    if (photoUrl.isEmpty()) return;
    
    // Set loading state for details photo
    m_detailsPhotoLabel->setProperty("class", "detailsPhotoLabelEmpty");
    m_detailsPhotoLabel->style()->unpolish(m_detailsPhotoLabel);
    m_detailsPhotoLabel->style()->polish(m_detailsPhotoLabel);
    m_detailsPhotoLabel->setText("Yüklüyor...");
    
    // Use the authenticated storage service to load the image
    if (m_storageService) {
        // Use a special key to identify this as the details photo
        QString detailsKey = "DETAILS_" + photoUrl;
        m_photoLabels[detailsKey] = m_detailsPhotoLabel;
        m_storageService->loadImage(photoUrl);
    } else {
        m_detailsPhotoLabel->setText("Servis Yok");
    }
}

void MainWindow::onSignOut()
{
    int ret = QMessageBox::question(this, "Çıkış Yap", 
                                   "Çıkış yapmak istediğinizden emin misiniz?",
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (m_authService) {
            m_authService->signOut();
        }
        
        // Close the application - user will need to authenticate again on next startup
        QApplication::quit();
    }
}

void MainWindow::onDeferredUploadCompleted()
{
    qCInfo(dataLog) << "Deferred photo upload completed, clearing pending dialog pointer";
    m_pendingPhotoDialog = nullptr;
}

void MainWindow::onImageLoaded(const QString& imageUrl, const QByteArray& imageData)
{
    qCDebug(dataLog) << "Image loaded successfully for URL:" << imageUrl;
    qCDebug(dataLog) << "Image data size:" << imageData.size() << "bytes";
    
    // Check for details photo first (using special key)
    QString detailsKey = "DETAILS_" + imageUrl;
    if (m_photoLabels.contains(detailsKey)) {
        QLabel* photoLabel = m_photoLabels[detailsKey];
        if (photoLabel && imageUrl == m_currentDetailsPhotoUrl) {
            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                qCDebug(dataLog) << "Details pixmap created successfully, size:" << pixmap.size();
                photoLabel->setProperty("class", "detailsPhotoLabel");
                photoLabel->style()->unpolish(photoLabel);
                photoLabel->style()->polish(photoLabel);
                photoLabel->setPixmap(pixmap);
            } else {
                qCWarning(dataLog) << "Failed to create details pixmap from image data";
                photoLabel->setProperty("class", "detailsPhotoLabelEmpty");
                photoLabel->style()->unpolish(photoLabel);
                photoLabel->style()->polish(photoLabel);
                photoLabel->setText("Geçersiz Resim");
            }
        }
        // Clean up the mapping
        m_photoLabels.remove(detailsKey);
    }
    
    // Find the corresponding table label
    if (m_photoLabels.contains(imageUrl)) {
        QLabel* photoLabel = m_photoLabels[imageUrl];
        if (photoLabel) {
            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                qCDebug(dataLog) << "Table pixmap created successfully, size:" << pixmap.size();
                photoLabel->setProperty("class", "photoLabel");
                photoLabel->style()->unpolish(photoLabel);
                photoLabel->style()->polish(photoLabel);
                photoLabel->setPixmap(pixmap);
            } else {
                qCWarning(dataLog) << "Failed to create table pixmap from image data";
                photoLabel->setProperty("class", "photoLabelEmpty");
                photoLabel->style()->unpolish(photoLabel);
                photoLabel->style()->polish(photoLabel);
                photoLabel->setText("Geçersiz");
            }
        }
        // Clean up the mapping
        m_photoLabels.remove(imageUrl);
    }
    
    if (!m_photoLabels.contains(imageUrl) && !m_photoLabels.contains(detailsKey)) {
        qCWarning(dataLog) << "No photo label found for URL:" << imageUrl;
    }
}

void MainWindow::onImageLoadFailed(const QString& imageUrl, const QString& error)
{
    qCWarning(dataLog) << "Image load failed for URL:" << imageUrl << "Error:" << error;
    
    // Check for details photo first (using special key)
    QString detailsKey = "DETAILS_" + imageUrl;
    if (m_photoLabels.contains(detailsKey)) {
        QLabel* photoLabel = m_photoLabels[detailsKey];
        if (photoLabel) {
            photoLabel->setProperty("class", "detailsPhotoLabelEmpty");
            photoLabel->style()->unpolish(photoLabel);
            photoLabel->style()->polish(photoLabel);
            photoLabel->setText("Yükleme Başarısız");
        }
        // Clean up the mapping
        m_photoLabels.remove(detailsKey);
    }
    
    // Find the corresponding table label
    if (m_photoLabels.contains(imageUrl)) {
        QLabel* photoLabel = m_photoLabels[imageUrl];
        if (photoLabel) {
            photoLabel->setProperty("class", "photoLabelEmpty");
            photoLabel->style()->unpolish(photoLabel);
            photoLabel->style()->polish(photoLabel);
            photoLabel->setText("Başarısız");
        }
        // Clean up the mapping
        m_photoLabels.remove(imageUrl);
    }
}

void MainWindow::onExportToExcel()
{
    // Get the list to export (filtered or all)
    const QList<Student>& studentsToExport = m_filteredStudents.isEmpty() ? m_allStudents : m_filteredStudents;
    
    if (studentsToExport.isEmpty()) {
        QMessageBox::information(this, "Excel'e Aktar", "Aktarılacak mezun verisi bulunamadı.");
        return;
    }
    
    // Get file path from user
    QString defaultFileName = QString("mezunlar_%1.xlsx")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss"));
    
    QString filePath = QFileDialog::getSaveFileName(this,
        "Excel'e Aktar",
        defaultFileName,
        "Excel Dosyaları (*.xlsx);;Tüm Dosyalar (*)");
    
    if (filePath.isEmpty()) {
        return; // User cancelled
    }
    
    // Create Excel document
    Document xlsx;
    
    // Create header format (bold, centered, with background color)
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setHorizontalAlignment(Format::AlignHCenter);
    headerFormat.setVerticalAlignment(Format::AlignVCenter);
    headerFormat.setPatternBackgroundColor(QColor(79, 129, 189)); // Blue background
    headerFormat.setFontColor(QColor(Qt::white));
    headerFormat.setBorderStyle(Format::BorderThin);
    
    // Write header row
    xlsx.write(1, 1, "Ad", headerFormat);
    xlsx.write(1, 2, "E-posta", headerFormat);
    xlsx.write(1, 3, "Açıklama", headerFormat);
    xlsx.write(1, 4, "Alan", headerFormat);
    xlsx.write(1, 5, "Okul", headerFormat);
    xlsx.write(1, 6, "Numara", headerFormat);
    xlsx.write(1, 7, "Lise Mezuniyet Yılı", headerFormat);
    xlsx.write(1, 8, "Üniversite Mezun Durumu", headerFormat);
    
    // Create cell format for data
    Format dataFormat;
    dataFormat.setBorderStyle(Format::BorderThin);
    dataFormat.setVerticalAlignment(Format::AlignVCenter);
    
    // Create format for centered columns (year, graduation)
    Format centerFormat;
    centerFormat.setBorderStyle(Format::BorderThin);
    centerFormat.setHorizontalAlignment(Format::AlignHCenter);
    centerFormat.setVerticalAlignment(Format::AlignVCenter);
    
    // Write student data
    int row = 2; // Start from row 2 (after header)
    for (const Student& student : studentsToExport) {
        QString graduationStatus = student.getGraduation() ? "Evet" : "Hayır";
        
        xlsx.write(row, 1, student.getName(), dataFormat);
        xlsx.write(row, 2, student.getEmail(), dataFormat);
        xlsx.write(row, 3, student.getDescription(), dataFormat);
        xlsx.write(row, 4, student.getField(), dataFormat);
        xlsx.write(row, 5, student.getSchool(), dataFormat);
        xlsx.write(row, 6, student.getNumber(), centerFormat);
        xlsx.write(row, 7, student.getYear(), centerFormat);
        xlsx.write(row, 8, graduationStatus, centerFormat);
        
        row++;
    }
    
    // Auto-fit columns
    xlsx.setColumnWidth(1, 20);  // Ad
    xlsx.setColumnWidth(2, 30);  // E-posta
    xlsx.setColumnWidth(3, 40);  // Açıklama
    xlsx.setColumnWidth(4, 20);  // Alan
    xlsx.setColumnWidth(5, 25);  // Okul
    xlsx.setColumnWidth(6, 15);  // Numara
    xlsx.setColumnWidth(7, 20);  // Lise Mezuniyet Yılı
    xlsx.setColumnWidth(8, 25);  // Üniversite Mezun Durumu
    
    // Save the document
    if (xlsx.saveAs(filePath)) {
        int exportedCount = studentsToExport.size();
        QMessageBox::information(this, "Başarılı", 
            QString("%1 mezun başarıyla Excel dosyasına aktarıldı.\n\nDosya: %2")
                .arg(exportedCount)
                .arg(filePath));
        
        qCInfo(dataLog) << "Exported" << exportedCount << "students to" << filePath;
    } else {
        QMessageBox::critical(this, "Hata", "Excel dosyası oluşturulamadı.");
        qCWarning(dataLog) << "Failed to save Excel file:" << filePath;
    }
}

void MainWindow::onImportFromExcel()
{
    // Ask user for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Excel'den İçe Aktar",
        "Excel dosyasından mezun verilerini içe aktarmak istiyor musunuz?\n\n"
        "Not: Fotoğraflar içe aktarılmayacak. Var olan kayıtlar güncellenmeyecek, yalnızca yeni kayıtlar eklenecektir.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Get file path from user
    QString filePath = QFileDialog::getOpenFileName(this,
        "Excel'den İçe Aktar",
        "",
        "Excel Dosyaları (*.xlsx);;Tüm Dosyalar (*)");
    
    if (filePath.isEmpty()) {
        return; // User cancelled
    }
    
    // Open Excel document
    Document xlsx(filePath);
    if (!xlsx.load()) {
        QMessageBox::critical(this, "Hata", "Excel dosyası açılamadı veya geçersiz format.");
        return;
    }
    
    int importedCount = 0;
    int errorCount = 0;
    QStringList errors;
    
    // Get the dimension of the data
    CellRange dimension = xlsx.dimension();
    int lastRow = dimension.lastRow();
    
    if (lastRow < 2) {
        QMessageBox::warning(this, "Hata", "Excel dosyası boş veya yalnızca başlık satırı içeriyor.");
        return;
    }
    
    // Read data starting from row 2 (skip header)
    for (int row = 2; row <= lastRow; row++) {
        // Read cell values using the read method (simpler and more reliable)
        QString name = xlsx.read(row, 1).toString().trimmed();
        QString email = xlsx.read(row, 2).toString().trimmed();
        QString description = xlsx.read(row, 3).toString().trimmed();
        QString field = xlsx.read(row, 4).toString().trimmed();
        QString school = xlsx.read(row, 5).toString().trimmed();
        QString number = xlsx.read(row, 6).toString().trimmed();
        
        // Skip completely empty rows
        if (name.isEmpty() && email.isEmpty() && field.isEmpty()) {
            continue;
        }
        
        // Validate required fields (only name is required)
        if (name.isEmpty()) {
            errors.append(QString("Satır %1: Ad zorunludur").arg(row));
            errorCount++;
            continue;
        }
        
        // Parse year
        QVariant yearValue = xlsx.read(row, 7);
        bool yearOk = false;
        int year = 0;
        
        if (yearValue.typeId() == QMetaType::Int || yearValue.typeId() == QMetaType::LongLong || 
            yearValue.typeId() == QMetaType::Double) {
            year = yearValue.toInt(&yearOk);
        } else if (!yearValue.toString().isEmpty()) {
            year = yearValue.toString().toInt(&yearOk);
        }
        
        if (!yearOk || year == 0) {
            errors.append(QString("Satır %1: Geçersiz yıl değeri").arg(row));
            errorCount++;
            continue;
        }
        
        // Parse graduation status
        QVariant graduationValue = xlsx.read(row, 8);
        bool graduation = false;
        
        if (graduationValue.isNull() || graduationValue.toString().isEmpty()) {
            errors.append(QString("Satır %1: Mezuniyet durumu zorunludur").arg(row));
            errorCount++;
            continue;
        }
        
        QString graduationStr = graduationValue.toString().toLower().trimmed();
        if (graduationStr == "evet" || graduationStr == "yes" || graduationStr == "true" || graduationStr == "1") {
            graduation = true;
        } else if (graduationStr == "hayır" || graduationStr == "no" || graduationStr == "false" || graduationStr == "0") {
            graduation = false;
        } else {
            errors.append(QString("Satır %1: Geçersiz mezuniyet durumu: %2 (Evet/Hayır bekleniyor)")
                .arg(row).arg(graduationStr));
            errorCount++;
            continue;
        }
        
        // Create student object (ID will be assigned by Firestore)
        Student student("", name, email, description, field, school, number, year, graduation, "");
        
        // Add student to Firestore
        m_firestoreService->addStudent(student);
        importedCount++;
    }
    
    // Show results
    QString message = QString("%1 mezun başarıyla içe aktarıldı.").arg(importedCount);
    if (errorCount > 0) {
        message += QString("\n\n%1 satırda hata oluştu:").arg(errorCount);
        if (errors.size() <= 10) {
            message += "\n" + errors.join("\n");
        } else {
            message += "\n" + errors.mid(0, 10).join("\n");
            message += QString("\n... ve %1 hata daha").arg(errors.size() - 10);
        }
    }
    
    if (importedCount > 0 && errorCount == 0) {
        QMessageBox::information(this, "Başarılı", message);
    } else if (importedCount > 0 && errorCount > 0) {
        QMessageBox::warning(this, "Kısmen Başarılı", message);
    } else {
        QMessageBox::critical(this, "Başarısız", message);
    }
    
    qCInfo(dataLog) << "Imported" << importedCount << "students from" << filePath 
                    << "with" << errorCount << "errors";
    
    // Refresh the table to show newly imported students
    if (importedCount > 0) {
        onRefreshStudents();
    }
}
