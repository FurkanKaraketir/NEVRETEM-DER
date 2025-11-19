#include "studentdialog.h"
#include <QUuid>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QUrl>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

StudentDialog::StudentDialog(QWidget *parent)
    : QDialog(parent)
    , m_isEditing(false)
    , m_storageService(nullptr)
    , m_isUploading(false)
{
    setupUI();
    setWindowTitle("Yeni Mezun Ekle");
}

StudentDialog::StudentDialog(const Student& student, QWidget *parent)
    : QDialog(parent)
    , m_student(student)
    , m_isEditing(true)
    , m_storageService(nullptr)
    , m_isUploading(false)
{
    setupUI();
    populateFields(student);
    setWindowTitle("Mezun Bilgilerini Düzenle");
}

StudentDialog::~StudentDialog()
{
    // Ensure all connections to storage service are properly disconnected
    if (m_storageService) {
        disconnect(m_storageService, nullptr, this, nullptr);
    }
}

void StudentDialog::setupUI()
{
    setModal(true);
    resize(800, 450);
    
    m_mainLayout = new QVBoxLayout(this);
    
    // Create horizontal layout for two-column design
    QHBoxLayout* contentLayout = new QHBoxLayout();
    
    // LEFT COLUMN - Personal Information with Photo
    QVBoxLayout* leftColumn = new QVBoxLayout();
    
    m_personalGroup = new QGroupBox("Kişisel Bilgiler");
    m_personalLayout = new QFormLayout(m_personalGroup);
    
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Tam adınızı girin");
    m_personalLayout->addRow("Ad*:", m_nameEdit);
    
    m_emailEdit = new QLineEdit();
    m_emailEdit->setPlaceholderText("ornek@email.com (isteğe bağlı)");
    // Email validation (only when not empty)
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    m_emailEdit->setValidator(new QRegularExpressionValidator(emailRegex, this));
    m_personalLayout->addRow("E-posta:", m_emailEdit);
    
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(60);
    m_descriptionEdit->setPlaceholderText("Kısa açıklama veya rol");
    m_personalLayout->addRow("Açıklama:", m_descriptionEdit);
    
    // Photo upload section
    m_photoLayout = new QHBoxLayout();
    m_selectPhotoButton = new QPushButton("Fotoğraf Seç");
    m_removePhotoButton = new QPushButton("Kaldır");
    m_removePhotoButton->setEnabled(false);
    
    m_selectPhotoButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    m_removePhotoButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    
    m_photoLayout->addWidget(m_selectPhotoButton);
    m_photoLayout->addWidget(m_removePhotoButton);
    m_photoLayout->addStretch();
    
    m_personalLayout->addRow("Fotoğraf:", m_photoLayout);
    
    // Photo preview - smaller size
    m_photoPreview = new QLabel();
    m_photoPreview->setFixedSize(120, 120);
    m_photoPreview->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
    m_photoPreview->setAlignment(Qt::AlignCenter);
    m_photoPreview->setText("Fotoğraf yok");
    m_photoPreview->setScaledContents(true);
    m_personalLayout->addRow("Önizleme:", m_photoPreview);
    
    // Upload progress
    m_uploadProgress = new QProgressBar();
    m_uploadProgress->setVisible(false);
    m_personalLayout->addRow("Yükleme:", m_uploadProgress);
    
    // Photo status
    m_photoStatusLabel = new QLabel();
    m_photoStatusLabel->setVisible(false);
    m_photoStatusLabel->setWordWrap(true);
    m_personalLayout->addRow("Durum:", m_photoStatusLabel);
    
    // Keep the URL field for manual entry (hidden by default)
    m_photoURLEdit = new QLineEdit();
    m_photoURLEdit->setPlaceholderText("https://ornek.com/foto.jpg");
    m_photoURLEdit->setVisible(false);
    m_personalLayout->addRow("Fotoğraf URL:", m_photoURLEdit);
    
    leftColumn->addWidget(m_personalGroup);
    leftColumn->addStretch();
    
    // RIGHT COLUMN - Academic and Contact Information
    QVBoxLayout* rightColumn = new QVBoxLayout();
    
    // Academic Information Group
    m_academicGroup = new QGroupBox("Akademik Bilgiler");
    m_academicLayout = new QFormLayout(m_academicGroup);
    
    m_fieldEdit = new QLineEdit();
    m_fieldEdit->setPlaceholderText("Bölüm adını girin (üniversiteye gitmiyorsa boş bırakabilirsiniz)");
    m_academicLayout->addRow("Bölüm:", m_fieldEdit);
    
    m_schoolCombo = new QComboBox();
    m_schoolCombo->setEditable(false);
    QStringList universities = loadUniversities();
    m_schoolCombo->addItems(universities);
    m_academicLayout->addRow("Okul*:", m_schoolCombo);
    
    m_customSchoolEdit = new QLineEdit();
    m_customSchoolEdit->setPlaceholderText("Özel üniversite adı girin");
    m_customSchoolEdit->setVisible(false);
    m_academicLayout->addRow("Özel Okul:", m_customSchoolEdit);
    
    m_yearSpin = new QSpinBox();
    m_yearSpin->setRange(1950, 2050);
    m_yearSpin->setValue(2023);
    m_academicLayout->addRow("Lise Mezuniyet Yılı:", m_yearSpin);
    
    m_graduationCheck = new QCheckBox("Üniversiteden mezun oldu");
    m_academicLayout->addRow("Üniversite Mezuniyeti:", m_graduationCheck);
    
    rightColumn->addWidget(m_academicGroup);
    
    // Contact Information Group
    m_contactGroup = new QGroupBox("İletişim Bilgileri");
    m_contactLayout = new QFormLayout(m_contactGroup);
    
    m_numberEdit = new QLineEdit();
    m_numberEdit->setPlaceholderText("05XX XXX XX XX");
    m_numberEdit->setMaxLength(18); // Max length for formatted number (supports 5-digit codes)
    m_numberEdit->setToolTip("Türkiye cep telefonu numarası (Tüm operatörler desteklenir: Turkcell, Vodafone, Türk Telekom, NETGSM, vb.)");
    m_contactLayout->addRow("Telefon Numarası:", m_numberEdit);
    
    // Phone format feedback label
    m_phoneFormatLabel = new QLabel();
    m_phoneFormatLabel->setStyleSheet("color: #666; font-size: 11px; font-style: italic;");
    m_phoneFormatLabel->setText("Format: 05XX XXX XX XX");
    m_phoneFormatLabel->setVisible(false);
    m_contactLayout->addRow("", m_phoneFormatLabel);
    
    rightColumn->addWidget(m_contactGroup);
    rightColumn->addStretch();
    
    // Add columns to horizontal layout
    contentLayout->addLayout(leftColumn, 1);
    contentLayout->addLayout(rightColumn, 1);
    
    m_mainLayout->addLayout(contentLayout);
    
    // Validation label
    m_validationLabel = new QLabel();
    m_validationLabel->setStyleSheet("color: red; font-weight: bold;");
    m_validationLabel->setWordWrap(true);
    m_validationLabel->setVisible(false);
    m_mainLayout->addWidget(m_validationLabel);
    
    // Button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &StudentDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_fieldEdit, &QLineEdit::textChanged, this, &StudentDialog::validateForm);
    connect(m_schoolCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), 
            this, &StudentDialog::onSchoolChanged);
    connect(m_numberEdit, &QLineEdit::textChanged, this, &StudentDialog::onPhoneNumberChanged);
    
    // Photo upload signals
    connect(m_selectPhotoButton, &QPushButton::clicked, this, &StudentDialog::onSelectPhoto);
    connect(m_removePhotoButton, &QPushButton::clicked, this, &StudentDialog::onRemovePhoto);
    
    // Connect validation
    connect(m_nameEdit, &QLineEdit::textChanged, this, &StudentDialog::validateForm);
    connect(m_emailEdit, &QLineEdit::textChanged, this, &StudentDialog::validateForm);
    connect(m_schoolCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), 
            this, &StudentDialog::validateForm);
    connect(m_customSchoolEdit, &QLineEdit::textChanged, this, &StudentDialog::validateForm);
    
    // Connect auto-capitalization for Turkish characters
    connect(m_nameEdit, &QLineEdit::textChanged, this, &StudentDialog::onNameTextChanged);
    connect(m_fieldEdit, &QLineEdit::textChanged, this, &StudentDialog::onFieldTextChanged);
    
    validateForm();
}

void StudentDialog::populateFields(const Student& student)
{
    m_nameEdit->setText(student.getName());
    m_emailEdit->setText(student.getEmail());
    m_descriptionEdit->setPlainText(student.getDescription());
    
    // Set field
    m_fieldEdit->setText(student.getField());
    
    // Set school
    int schoolIndex = m_schoolCombo->findText(student.getSchool());
    if (schoolIndex >= 0) {
        m_schoolCombo->setCurrentIndex(schoolIndex);
    } else {
        // Check if this is a "no university" case by looking at field
        if (student.getField().isEmpty() && !student.getGraduation()) {
            m_schoolCombo->setCurrentText("Üniversiteye gitmedi");
        } else {
            m_schoolCombo->setCurrentText("Diğer");
            m_customSchoolEdit->setText(student.getSchool());
            m_customSchoolEdit->setVisible(true);
        }
    }
    m_numberEdit->setText(student.getNumber());
    m_yearSpin->setValue(student.getYear());
    m_graduationCheck->setChecked(student.getGraduation());
    m_photoURLEdit->setText(student.getPhotoURL());
    
    // Load existing photo if available
    if (!student.getPhotoURL().isEmpty()) {
        m_removePhotoButton->setEnabled(true);
        m_photoStatusLabel->setText("Fotoğraf URL'den yüklendi");
        m_photoStatusLabel->setVisible(true);
        loadPhotoFromUrl(student.getPhotoURL());
    }
}

void StudentDialog::onAccepted()
{
    if (!isFormValid()) {
        return;
    }
    
    // For new students, let Firestore generate the document ID
    // For existing students, keep the existing ID
    
    m_student.setName(m_nameEdit->text().trimmed());
    m_student.setEmail(m_emailEdit->text().trimmed());
    m_student.setDescription(m_descriptionEdit->toPlainText().trimmed());
    
    // Handle field
    m_student.setField(m_fieldEdit->text().trimmed());
    
    // Handle school
    QString school;
    if (m_schoolCombo->currentText() == "Diğer" && m_customSchoolEdit->isVisible()) {
        school = m_customSchoolEdit->text().trimmed();
    } else {
        school = m_schoolCombo->currentText();
    }
    m_student.setSchool(school);
    
    // Handle field - clear if not going to university
    if (m_schoolCombo->currentText() == "Üniversiteye gitmedi") {
        m_student.setField("");
    }
    m_student.setNumber(m_numberEdit->text().trimmed());
    m_student.setYear(m_yearSpin->value());
    m_student.setGraduation(m_graduationCheck->isChecked());
    m_student.setPhotoURL(m_photoURLEdit->text().trimmed());
    
    accept();
}


void StudentDialog::onSchoolChanged()
{
    QString currentSchool = m_schoolCombo->currentText();
    bool showCustomSchool = (currentSchool == "Diğer");
    bool isNoUniversity = (currentSchool == "Üniversiteye gitmedi");
    
    m_customSchoolEdit->setVisible(showCustomSchool);
    
    // Update field requirement based on university selection
    if (isNoUniversity) {
        m_fieldEdit->setEnabled(false);
        m_fieldEdit->clear();
        m_fieldEdit->setPlaceholderText("Üniversiteye gitmediği için bölüm yok");
        m_graduationCheck->setEnabled(false);
        m_graduationCheck->setChecked(false);
    } else {
        m_fieldEdit->setEnabled(true);
        m_fieldEdit->setPlaceholderText("Bölüm adını girin (üniversiteye gitmiyorsa boş bırakabilirsiniz)");
        m_graduationCheck->setEnabled(true);
    }
    
    if (showCustomSchool) {
        m_customSchoolEdit->setFocus();
    }
    
    validateForm();
}

void StudentDialog::validateForm()
{
    bool valid = isFormValid();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
    
    if (!valid) {
        QString errorMsg;
        
        if (m_nameEdit->text().trimmed().isEmpty()) {
            errorMsg = "Ad zorunludur.";
        } else if (!m_emailEdit->text().trimmed().isEmpty() && !m_emailEdit->hasAcceptableInput()) {
            errorMsg = "Lütfen geçerli bir e-posta adresi girin.";
        } else if (m_schoolCombo->currentText().isEmpty() || 
                  (m_schoolCombo->currentText() == "Diğer" && m_customSchoolEdit->text().trimmed().isEmpty())) {
            errorMsg = "Okul zorunludur.";
        } else if (m_schoolCombo->currentText() != "Üniversiteye gitmedi" && m_fieldEdit->text().trimmed().isEmpty()) {
            errorMsg = "Üniversiteye gidiyorsa bölüm zorunludur.";
        } else if (!m_numberEdit->text().trimmed().isEmpty()) {
            QString validationError = validateTurkishPhoneNumber(m_numberEdit->text());
            if (!validationError.isEmpty()) {
                errorMsg = validationError;
            }
        }
        
        m_validationLabel->setText(errorMsg);
        m_validationLabel->setVisible(!errorMsg.isEmpty());
    } else {
        m_validationLabel->setVisible(false);
    }
}

bool StudentDialog::isFormValid() const
{
    if (m_nameEdit->text().trimmed().isEmpty()) return false;
    if (!m_emailEdit->text().trimmed().isEmpty() && !m_emailEdit->hasAcceptableInput()) return false;
    // Check school
    if (m_schoolCombo->currentText().isEmpty()) return false;
    if (m_schoolCombo->currentText() == "Diğer" && m_customSchoolEdit->text().trimmed().isEmpty()) return false;
    
    // Check field - only required if going to university
    if (m_schoolCombo->currentText() != "Üniversiteye gitmedi" && m_fieldEdit->text().trimmed().isEmpty()) return false;
    
    // Check phone number format if not empty
    if (!m_numberEdit->text().trimmed().isEmpty()) {
        QString validationError = validateTurkishPhoneNumber(m_numberEdit->text());
        if (!validationError.isEmpty()) return false;
    }
    
    return true;
}

Student StudentDialog::getStudent() const
{
    return m_student;
}

void StudentDialog::setStorageService(FirebaseStorageService* storageService)
{
    m_storageService = storageService;
    
    if (m_storageService) {
        // Connect storage service signals
        connect(m_storageService, &FirebaseStorageService::fileUploaded,
                this, &StudentDialog::onPhotoUploaded);
        connect(m_storageService, &FirebaseStorageService::errorOccurred,
                this, &StudentDialog::onPhotoUploadError);
        connect(m_storageService, &FirebaseStorageService::uploadProgress,
                this, &StudentDialog::onUploadProgress);
    }
}

void StudentDialog::onSelectPhoto()
{
    if (m_isUploading) {
        QMessageBox::information(this, "Yükleme Devam Ediyor", 
                                "Lütfen mevcut yüklemenin tamamlanmasını bekleyin.");
        return;
    }
    
    QString fileName = QFileDialog::getOpenFileName(this,
        "Mezun Fotoğrafı Seç", 
        QString(),
        "Resim Dosyaları (*.png *.jpg *.jpeg *.gif *.bmp);;Tüm Dosyalar (*)");
    
    if (!fileName.isEmpty()) {
        m_selectedPhotoPath = fileName;
        
        // Show preview
        QPixmap pixmap(fileName);
        if (!pixmap.isNull()) {
            m_photoPreview->setPixmap(pixmap);
            m_removePhotoButton->setEnabled(true);
            
            // For existing students, upload immediately
            // For new students, defer upload until after student is saved
            if (m_storageService && m_isEditing && !m_student.getId().isEmpty()) {
                m_isUploading = true;
                m_selectPhotoButton->setEnabled(false);
                m_uploadProgress->setVisible(true);
                m_uploadProgress->setRange(0, 100);
                m_uploadProgress->setValue(0);
                m_photoStatusLabel->setText("Fotoğraf yüklüyor...");
                m_photoStatusLabel->setVisible(true);
                
                // Delete old photo file first for existing students
                if (!m_student.getPhotoURL().isEmpty()) {
                    // Try to extract extension from the stored URL
                    // URL format: ...student_photos%2F{ID}.{ext}?alt=media...
                    QString photoUrl = m_student.getPhotoURL();
                    QRegularExpression extensionRegex(QString("student_photos(?:%2F|/)%1\\.(jpg|jpeg|png|gif|bmp)").arg(m_student.getId()));
                    QRegularExpressionMatch match = extensionRegex.match(photoUrl);
                    
                    if (match.hasMatch()) {
                        // We found the exact extension, delete only that file
                        QString extension = match.captured(1);
                        QString oldStoragePath = QString("student_photos/%1.%2").arg(m_student.getId(), extension);
                        qDebug() << "Deleting old photo with extension:" << extension;
                        m_storageService->deleteFile(oldStoragePath);
                    } else {
                        // Fallback: try common extensions (should rarely happen)
                        qDebug() << "Could not extract extension from URL:" << photoUrl;
                        qDebug() << "Trying common extensions as fallback";
                        QStringList extensions = {"jpg", "jpeg", "png", "gif", "bmp"};
                        for (const QString& ext : extensions) {
                            QString oldStoragePath = QString("student_photos/%1.%2").arg(m_student.getId(), ext);
                            m_storageService->deleteFile(oldStoragePath);
                        }
                    }
                }
                
                // Get file extension from selected file
                QString fileExtension = QFileInfo(fileName).suffix().toLower();
                if (fileExtension.isEmpty()) {
                    fileExtension = "jpg"; // Default extension
                }
                
                QString storagePath = QString("student_photos/%1.%2").arg(m_student.getId(), fileExtension);
                m_storageService->uploadFile(fileName, storagePath);
            } else {
                // For new students or when no storage service, just show selected status
                if (m_isEditing) {
                m_photoStatusLabel->setText("Fotoğraf seçildi (depolama servisi mevcut değil)");
                } else {
                    m_photoStatusLabel->setText("Fotoğraf seçildi - öğrenci kaydedildikten sonra yüklenecek");
                }
                m_photoStatusLabel->setVisible(true);
            }
        } else {
            QMessageBox::warning(this, "Geçersiz Resim", 
                                "Seçilen dosya geçerli bir resim değil.");
        }
    }
}

void StudentDialog::onRemovePhoto()
{
    if (m_isUploading) {
        QMessageBox::information(this, "Yükleme Devam Ediyor", 
                                "Lütfen mevcut yüklemenin tamamlanmasını bekleyin.");
        return;
    }
    
    // If there's an existing photo and we have a storage service, delete it
    if (!m_photoURLEdit->text().isEmpty() && m_storageService && !m_student.getId().isEmpty()) {
        // Try to extract extension from the stored URL
        // URL format: ...student_photos%2F{ID}.{ext}?alt=media...
        QString photoUrl = m_photoURLEdit->text();
        QString studentId = m_student.getId();
        QRegularExpression extensionRegex(QString("student_photos(?:%2F|/)%1\\.(jpg|jpeg|png|gif|bmp)").arg(studentId));
        QRegularExpressionMatch match = extensionRegex.match(photoUrl);
        
        if (match.hasMatch()) {
            // We found the exact extension, delete only that file
            QString extension = match.captured(1);
            QString storagePath = QString("student_photos/%1.%2").arg(studentId, extension);
            qDebug() << "Deleting old photo with extension:" << extension;
            m_storageService->deleteFile(storagePath);
        } else {
            // Fallback: try common extensions (should rarely happen)
            qDebug() << "Could not extract extension from URL:" << photoUrl;
            qDebug() << "Trying common extensions as fallback";
            QStringList extensions = {"jpg", "jpeg", "png", "gif", "bmp"};
            for (const QString& ext : extensions) {
                QString storagePath = QString("student_photos/%1.%2").arg(studentId, ext);
                m_storageService->deleteFile(storagePath);
            }
        }
    }
    
    // Clear photo
    m_photoPreview->clear();
    m_photoPreview->setText("Fotoğraf yok");
    m_selectedPhotoPath.clear();
    m_photoURLEdit->clear();
    m_removePhotoButton->setEnabled(false);
    m_photoStatusLabel->setVisible(false);
}

void StudentDialog::onPhotoUploaded(const QString& storagePath, const QString& downloadUrl)
{
    Q_UNUSED(storagePath)
    
    m_isUploading = false;
    m_selectPhotoButton->setEnabled(true);
    m_uploadProgress->setVisible(false);
    
    // Set the download URL
    m_photoURLEdit->setText(downloadUrl);
    
    m_photoStatusLabel->setText("Fotoğraf başarıyla yüklendi!");
    m_photoStatusLabel->setStyleSheet("color: green;");
    
    // Auto-hide status after 3 seconds (timer is cancelled if dialog is destroyed)
    QTimer::singleShot(3000, this, [this]() {
        if (m_photoStatusLabel) {
            m_photoStatusLabel->setVisible(false);
            m_photoStatusLabel->setStyleSheet("");
        }
    });
    
    // If this was a deferred upload (dialog is not visible), clean up
    if (!isVisible()) {
        // Disconnect from storage service signals to prevent crashes during destruction
        if (m_storageService) {
            disconnect(m_storageService, nullptr, this, nullptr);
        }
        emit deferredUploadCompleted();
        deleteLater();
    }
}

void StudentDialog::onPhotoUploadError(const QString& error)
{
    m_isUploading = false;
    m_selectPhotoButton->setEnabled(true);
    m_uploadProgress->setVisible(false);
    
    m_photoStatusLabel->setText(QString("Yükleme başarısız: %1").arg(error));
    m_photoStatusLabel->setStyleSheet("color: red;");
    m_photoStatusLabel->setVisible(true);
    
    // Only show message box if dialog is visible (not for deferred uploads)
    if (isVisible()) {
        QMessageBox::critical(this, "Yükleme Hatası", 
                             QString("Fotoğraf yüklenemedi: %1").arg(error));
    }
    
    // If this was a deferred upload (dialog is not visible), clean up
    if (!isVisible()) {
        // Disconnect from storage service signals to prevent crashes during destruction
        if (m_storageService) {
            disconnect(m_storageService, nullptr, this, nullptr);
        }
        emit deferredUploadCompleted();
        deleteLater();
    }
}

void StudentDialog::onUploadProgress(const QString& storagePath, qint64 bytesSent, qint64 bytesTotal)
{
    Q_UNUSED(storagePath)
    
    if (bytesTotal > 0) {
        int percentage = (bytesSent * 100) / bytesTotal;
        m_uploadProgress->setValue(percentage);
        m_photoStatusLabel->setText(QString("Yüklüyor... %1%").arg(percentage));
    }
}

void StudentDialog::loadPhotoFromUrl(const QString& url)
{
    if (url.isEmpty()) return;
    
    // Use the authenticated storage service to load the image
    if (m_storageService) {
        // Connect to the storage service signals for this specific load
        connect(m_storageService, &FirebaseStorageService::imageLoaded, this, &StudentDialog::onImageLoaded, Qt::UniqueConnection);
        connect(m_storageService, &FirebaseStorageService::imageLoadFailed, this, &StudentDialog::onImageLoadFailed, Qt::UniqueConnection);
        
        m_storageService->loadImage(url);
    } else {
        m_photoPreview->setText("Depolama servisi yok");
    }
}

void StudentDialog::onImageLoaded(const QString& imageUrl, const QByteArray& imageData)
{
    Q_UNUSED(imageUrl)
    
    QPixmap pixmap;
    if (pixmap.loadFromData(imageData)) {
        m_photoPreview->setPixmap(pixmap);
    } else {
        m_photoPreview->setText("Geçersiz resim");
    }
    
    // Disconnect the signals to avoid handling other image loads
    disconnect(m_storageService, &FirebaseStorageService::imageLoaded, this, &StudentDialog::onImageLoaded);
    disconnect(m_storageService, &FirebaseStorageService::imageLoadFailed, this, &StudentDialog::onImageLoadFailed);
}

void StudentDialog::onImageLoadFailed(const QString& imageUrl, const QString& error)
{
    Q_UNUSED(imageUrl)
    Q_UNUSED(error)
    
    m_photoPreview->setText("Yükleme başarısız");
    
    // Disconnect the signals to avoid handling other image loads
    disconnect(m_storageService, &FirebaseStorageService::imageLoaded, this, &StudentDialog::onImageLoaded);
    disconnect(m_storageService, &FirebaseStorageService::imageLoadFailed, this, &StudentDialog::onImageLoadFailed);
}

QStringList StudentDialog::loadUniversities()
{
    QStringList universities;
    
    // Try multiple paths to find the universities.json file
    QStringList possiblePaths = {
        // First try in the same directory as executable
        QDir(QCoreApplication::applicationDirPath()).filePath("universities.json"),
        // Then try relative to executable directory (for development)
        QDir(QCoreApplication::applicationDirPath()).filePath("../src/universities.json"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../src/universities.json"),
        // Try relative to current working directory
        "src/universities.json",
        "universities.json",
        // Try absolute path from project root
        QDir(QCoreApplication::applicationDirPath()).filePath("../../../src/universities.json")
    };
    
    QFile file;
    QString filePath;
    bool found = false;
    
    for (const QString& path : possiblePaths) {
        file.setFileName(path);
        if (file.exists()) {
            filePath = path;
            found = true;
            break;
        }
    }
    
    if (found && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const QJsonValue &value : array) {
                if (value.isString()) {
                    universities.append(value.toString());
                }
            }
        }
        file.close();
        
        // Debug info - can be removed in production
        qDebug() << "Successfully loaded" << universities.size() << "universities from:" << filePath;
    } else {
        // Debug info - show which paths were tried
        qDebug() << "Could not find universities.json file. Tried paths:";
        for (const QString& path : possiblePaths) {
            qDebug() << " -" << path << "(exists:" << QFile::exists(path) << ")";
        }
        
        // If file cannot be loaded, provide some default universities
        universities << "İSTANBUL TEKNİK ÜNİVERSİTESİ"
                    << "ORTA DOĞU TEKNİK ÜNİVERSİTESİ"
                    << "BOĞAZİÇİ ÜNİVERSİTESİ"
                    << "HACETTEPE ÜNİVERSİTESİ"
                    << "ANKARA ÜNİVERSİTESİ"
                    << "İSTANBUL ÜNİVERSİTESİ"
                    << "EGE ÜNİVERSİTESİ"
                    << "GAZİ ÜNİVERSİTESİ";
                    
        qDebug() << "Using default universities list with" << universities.size() << "entries";
    }
    
    // Add special options at the end
    universities.append("Üniversiteye gitmedi");
    universities.append("Diğer");
    
    return universities;
}

void StudentDialog::onPhoneNumberChanged()
{
    QString input = m_numberEdit->text();
    QString formatted = formatPhoneNumber(input);
    
    if (formatted != input) {
        // Block signals to prevent infinite loop
        m_numberEdit->blockSignals(true);
        m_numberEdit->setText(formatted);
        m_numberEdit->blockSignals(false);
        
        // Get operator name
        QString operatorName = getTurkishOperatorName(formatted);
        QString feedbackText = "✓ Otomatik formatlandı";
        if (!operatorName.isEmpty()) {
            feedbackText += " - " + operatorName;
        }
        
        m_phoneFormatLabel->setText(feedbackText);
        m_phoneFormatLabel->setStyleSheet("color: #28a745; font-size: 11px; font-weight: bold;");
        m_phoneFormatLabel->setVisible(true);
        
        // Don't auto-hide, keep showing operator info
    } else if (!input.isEmpty()) {
        // Check if format is correct
        QString validationError = validateTurkishPhoneNumber(input);
        if (validationError.isEmpty()) {
            // Valid format - show operator name
            QString operatorName = getTurkishOperatorName(input);
            QString feedbackText = "✓ Geçerli numara";
            if (!operatorName.isEmpty()) {
                feedbackText += " - " + operatorName;
            }
            m_phoneFormatLabel->setText(feedbackText);
            m_phoneFormatLabel->setStyleSheet("color: #28a745; font-size: 11px; font-weight: bold;");
            m_phoneFormatLabel->setVisible(true);
        } else {
            // Invalid format or operator
            m_phoneFormatLabel->setText("⚠ " + validationError);
            m_phoneFormatLabel->setStyleSheet("color: #dc3545; font-size: 11px;");
            m_phoneFormatLabel->setVisible(true);
        }
    } else {
        m_phoneFormatLabel->setVisible(false);
    }
    
    validateForm();
}

QString StudentDialog::formatPhoneNumber(const QString& input)
{
    // Remove all non-digit characters except +
    QString temp = input;
    QString digitsOnly;
    
    // Handle international format with +
    if (temp.startsWith("+")) {
        temp.remove(0, 1); // Remove +
    }
    
    // Extract only digits
    for (QChar c : temp) {
        if (c.isDigit()) {
            digitsOnly += c;
        }
    }
    
    // 5-digit operator codes (NETGSM, KKTC)
    QStringList fiveDigitCodes = {"05102", "05428", "05488", "05469", "05338"};
    
    // Check for 5-digit operator codes (12 digits total: 05XXX + 7 digits)
    if (digitsOnly.length() == 12 && digitsOnly.startsWith("05")) {
        QString operatorCode5 = digitsOnly.mid(0, 5);
        if (fiveDigitCodes.contains(operatorCode5)) {
            // 05XXX XXX XX XX format
            return QString("%1 %2 %3")
                   .arg(digitsOnly.mid(0, 5))
                   .arg(digitsOnly.mid(5, 3))
                   .arg(digitsOnly.mid(8, 2) + " " + digitsOnly.mid(10, 2));
        }
    }
    
    // Check for 5-digit codes with missing leading 0 (11 digits: 5XXX + 7 digits)
    if (digitsOnly.length() == 11 && digitsOnly.startsWith("5")) {
        QString operatorCode5 = "0" + digitsOnly.mid(0, 4);
        if (fiveDigitCodes.contains(operatorCode5)) {
            // 05XXX XXX XX XX format
            return QString("0%1 %2 %3")
                   .arg(digitsOnly.mid(0, 4))
                   .arg(digitsOnly.mid(4, 3))
                   .arg(digitsOnly.mid(7, 2) + " " + digitsOnly.mid(9, 2));
        }
    }
    
    // Standard 4-digit operator codes (11 digits total: 05XX + 7 digits)
    if (digitsOnly.length() == 11 && digitsOnly.startsWith("05")) {
        // Already in correct length with 0: 05XXXXXXXXX -> 05XX XXX XX XX
        return QString("%1 %2 %3")
               .arg(digitsOnly.mid(0, 4))
               .arg(digitsOnly.mid(4, 3))
               .arg(digitsOnly.mid(7, 2) + " " + digitsOnly.mid(9, 2));
    }
    else if (digitsOnly.length() == 10 && digitsOnly.startsWith("5")) {
        // Missing leading 0: 5XXXXXXXXX -> 05XX XXX XX XX
        return QString("0%1 %2 %3")
               .arg(digitsOnly.mid(0, 3))
               .arg(digitsOnly.mid(3, 3))
               .arg(digitsOnly.mid(6, 2) + " " + digitsOnly.mid(8, 2));
    }
    else if (digitsOnly.length() == 12 && digitsOnly.startsWith("90")) {
        // International format without +: 90XXXXXXXXXX -> 05XX XXX XX XX
        QString localNumber = digitsOnly.mid(2); // Remove 90
        if (localNumber.startsWith("5")) {
            return QString("0%1 %2 %3")
                   .arg(localNumber.mid(0, 3))
                   .arg(localNumber.mid(3, 3))
                   .arg(localNumber.mid(6, 2) + " " + localNumber.mid(8, 2));
        }
    }
    else if (digitsOnly.length() == 13 && digitsOnly.startsWith("905")) {
        // International format: 905XXXXXXXXX -> 05XX XXX XX XX
        QString localNumber = digitsOnly.mid(2); // Remove 90, keep the 5
        return QString("0%1 %2 %3")
               .arg(localNumber.mid(0, 3))
               .arg(localNumber.mid(3, 3))
               .arg(localNumber.mid(6, 2) + " " + localNumber.mid(8, 2));
    }
    
    // If no formatting rule applies, return original input
    return input;
}

QString StudentDialog::validateTurkishPhoneNumber(const QString& phoneNumber) const
{
    // Remove all spaces and dashes for validation
    QString cleaned = phoneNumber;
    cleaned.remove(' ').remove('-');
    
    // Check basic format: must start with 05 and be 11 digits minimum
    if (!cleaned.startsWith("05") || cleaned.length() < 11) {
        return "Telefon numarası '05XX XXX XX XX' formatında olmalıdır.";
    }
    
    // Check if all characters are digits
    for (QChar c : cleaned) {
        if (!c.isDigit()) {
            return "Telefon numarası sadece rakam içermelidir.";
        }
    }
    
    // Extract the operator code (can be 4 or 5 digits: 05XX or 05XXX)
    QString operatorCode4 = cleaned.mid(0, 4);  // First 4 digits
    QString operatorCode5 = cleaned.length() >= 5 ? cleaned.mid(0, 5) : "";  // First 5 digits
    
    // 5-digit operator codes (NETGSM and KKTC operators)
    QStringList validOperatorCodes5 = {
        // NETGSM (MVNO)
        "05102",
        // KKTC Telsim (Northern Cyprus)
        "05428", "05488", "05469",
        // KKTC Turkcell (Northern Cyprus)
        "05338"
    };
    
    // Check 5-digit codes first (more specific)
    if (!operatorCode5.isEmpty() && validOperatorCodes5.contains(operatorCode5)) {
        // For 5-digit codes, number should be 12 digits total: 05XXX + 7 digits
        if (cleaned.length() != 12) {
            return QString("Operatör kodu %1 için numara 12 haneli olmalıdır.").arg(operatorCode5);
        }
        return ""; // Valid 5-digit code
    }
    
    // Standard 4-digit operator codes
    QStringList validOperatorCodes4 = {
        // Türk Telekom
        "0501", "0505", "0506", "0507",
        // Türk Telekom - BİMcell (MVNO)
        "0551", "0552", "0553", "0554", "0555", "0559",
        // Turkcell
        "0510", "0530", "0531", "0532", "0533", "0534", "0535", "0536", "0537", "0538", "0539",
        // Turkcell - Bursa Mobile (MVNO)
        "0516",
        // Turkcell - 61Cell (MVNO)
        "0561",
        // Vodafone
        "0540", "0541", "0542", "0543", "0544", "0545", "0546", "0547", "0548", "0549"
    };
    
    if (!validOperatorCodes4.contains(operatorCode4)) {
        return QString("Geçersiz operatör kodu: %1. Lütfen geçerli bir Türkiye cep telefonu numarası girin.").arg(operatorCode4);
    }
    
    // For 4-digit codes, number should be 11 digits total: 05XX + 7 digits
    if (cleaned.length() != 11) {
        return QString("Telefon numarası 11 haneli olmalıdır (05XX XXX XX XX formatında).");
    }
    
    return ""; // Valid
}

QString StudentDialog::getTurkishOperatorName(const QString& phoneNumber) const
{
    // Remove all spaces and dashes
    QString cleaned = phoneNumber;
    cleaned.remove(' ').remove('-');
    
    if (cleaned.length() < 4 || !cleaned.startsWith("05")) {
        return "";
    }
    
    // Check for 5-digit operator codes first
    if (cleaned.length() >= 5) {
        QString operatorCode5 = cleaned.mid(0, 5);
        
        // NETGSM
        if (operatorCode5 == "05102") {
            return "NETGSM";
        }
        
        // KKTC Telsim (Northern Cyprus)
        if (operatorCode5 == "05428" || operatorCode5 == "05488" || operatorCode5 == "05469") {
            return "KKTC Telsim";
        }
        
        // KKTC Turkcell (Northern Cyprus)
        if (operatorCode5 == "05338") {
            return "KKTC Turkcell";
        }
    }
    
    // Extract the operator code (first 4 digits: 05XX)
    QString operatorCode = cleaned.mid(0, 4);
    
    // Türk Telekom
    if (operatorCode == "0501" || operatorCode == "0505" || 
        operatorCode == "0506" || operatorCode == "0507") {
        return "Türk Telekom";
    }
    
    // Türk Telekom - BİMcell (MVNO)
    if (operatorCode == "0551" || operatorCode == "0552" || 
        operatorCode == "0553" || operatorCode == "0554" || 
        operatorCode == "0555" || operatorCode == "0559") {
        return "Türk Telekom (BİMcell)";
    }
    
    // Turkcell
    if (operatorCode == "0510" || operatorCode.startsWith("053")) {
        return "Turkcell";
    }
    
    // Turkcell - Bursa Mobile (MVNO)
    if (operatorCode == "0516") {
        return "Turkcell (Bursa Mobile)";
    }
    
    // Turkcell - 61Cell (MVNO)
    if (operatorCode == "0561") {
        return "Turkcell (61Cell)";
    }
    
    // Vodafone
    if (operatorCode == "0540" || operatorCode == "0541" ||
        operatorCode == "0542" || operatorCode == "0543" || 
        operatorCode == "0544" || operatorCode == "0545" ||
        operatorCode == "0546" || operatorCode == "0547" ||
        operatorCode == "0548" || operatorCode == "0549") {
        return "Vodafone";
    }
    
    return "";
}

void StudentDialog::uploadDeferredPhoto(const QString& studentId)
{
    // Only upload if we have a selected photo path and storage service
    if (m_selectedPhotoPath.isEmpty() || !m_storageService || studentId.isEmpty()) {
        return;
    }
    
    // Update the student ID
    m_student.setId(studentId);
    
    // Start the upload process
    m_isUploading = true;
    m_selectPhotoButton->setEnabled(false);
    m_uploadProgress->setVisible(true);
    m_uploadProgress->setRange(0, 100);
    m_uploadProgress->setValue(0);
    m_photoStatusLabel->setText("Fotoğraf yüklüyor...");
    m_photoStatusLabel->setVisible(true);
    
    // Get file extension from selected file
    QString fileExtension = QFileInfo(m_selectedPhotoPath).suffix().toLower();
    if (fileExtension.isEmpty()) {
        fileExtension = "jpg"; // Default extension
    }
    
    QString storagePath = QString("student_photos/%1.%2").arg(studentId, fileExtension);
    m_storageService->uploadFile(m_selectedPhotoPath, storagePath);
}

QString StudentDialog::capitalizeTurkish(const QString& text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    QString result;
    
    for (int i = 0; i < text.length(); ++i) {
        QChar ch = text[i];
        
        // Turkish-specific capitalization rules using only Unicode hex codes
        if (ch == 'i' || ch == QChar(0x0069)) {  // Latin 'i'
            result += QChar(0x0130);  // İ (capital I with dot)
        } else if (ch == QChar(0x0131)) {  // Turkish ı (dotless i)
            result += 'I';  // I (capital I without dot)
        } else if (ch == QChar(0x015F)) {  // ş
            result += QChar(0x015E);  // Ş
        } else if (ch == QChar(0x011F)) {  // ğ
            result += QChar(0x011E);  // Ğ
        } else if (ch == QChar(0x00FC)) {  // ü
            result += QChar(0x00DC);  // Ü
        } else if (ch == QChar(0x00F6)) {  // ö
            result += QChar(0x00D6);  // Ö
        } else if (ch == QChar(0x00E7)) {  // ç
            result += QChar(0x00C7);  // Ç
        } else {
            result += ch.toUpper();
        }
    }
    
    return result;
}

void StudentDialog::onNameTextChanged()
{
    // Get current cursor position
    int cursorPos = m_nameEdit->cursorPosition();
    QString currentText = m_nameEdit->text();
    QString capitalizedText = capitalizeTurkish(currentText);
    
    if (capitalizedText != currentText) {
        // Block signals to prevent infinite loop
        m_nameEdit->blockSignals(true);
        m_nameEdit->setText(capitalizedText);
        m_nameEdit->setCursorPosition(cursorPos);
        m_nameEdit->blockSignals(false);
    }
}

void StudentDialog::onFieldTextChanged()
{
    // Get current cursor position
    int cursorPos = m_fieldEdit->cursorPosition();
    QString currentText = m_fieldEdit->text();
    QString capitalizedText = capitalizeTurkish(currentText);
    
    if (capitalizedText != currentText) {
        // Block signals to prevent infinite loop
        m_fieldEdit->blockSignals(true);
        m_fieldEdit->setText(capitalizedText);
        m_fieldEdit->setCursorPosition(cursorPos);
        m_fieldEdit->blockSignals(false);
    }
}
