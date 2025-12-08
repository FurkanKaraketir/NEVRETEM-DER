#ifndef STUDENTDIALOG_H
#define STUDENTDIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QTimer>

#include "student.h"
#include "firebasestorageservice.h"

class StudentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StudentDialog(QWidget *parent = nullptr);
    explicit StudentDialog(const Student& student, QWidget *parent = nullptr);
    ~StudentDialog();

    Student getStudent() const;
    void setStorageService(FirebaseStorageService* storageService);
    void uploadDeferredPhoto(const QString& studentId);

signals:
    void deferredUploadCompleted();

private slots:
    void onAccepted();
    void onSchoolChanged();
    void validateForm();
    void onPhoneNumberChanged();
    void onSelectPhoto();
    void onRemovePhoto();
    void onPhotoUploaded(const QString& storagePath, const QString& downloadUrl);
    void onPhotoUploadError(const QString& error);
    void onUploadProgress(const QString& storagePath, qint64 bytesSent, qint64 bytesTotal);
    void onImageLoaded(const QString& imageUrl, const QByteArray& imageData);
    void onImageLoadFailed(const QString& imageUrl, const QString& error);
    void onNameTextChanged();
    void onFieldTextChanged();

private:
    void setupUI();
    void populateFields(const Student& student);
    bool isFormValid() const;
    void loadPhotoFromUrl(const QString& url);
    QString formatPhoneNumber(const QString& input);
    QStringList loadUniversities();
    QString capitalizeTurkish(const QString& text);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_personalGroup;
    QFormLayout* m_personalLayout;
    QGroupBox* m_academicGroup;
    QFormLayout* m_academicLayout;
    QGroupBox* m_contactGroup;
    QFormLayout* m_contactLayout;
    
    QLineEdit* m_nameEdit;
    QLineEdit* m_emailEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_fieldEdit;
    QComboBox* m_schoolCombo;
    QLineEdit* m_customSchoolEdit;
    QLineEdit* m_numberEdit;
    QSpinBox* m_yearSpin;
    QCheckBox* m_graduationCheck;
    QLineEdit* m_photoURLEdit;
    
    // Photo upload components
    QHBoxLayout* m_photoLayout;
    QPushButton* m_selectPhotoButton;
    QPushButton* m_removePhotoButton;
    QLabel* m_photoPreview;
    QProgressBar* m_uploadProgress;
    QLabel* m_photoStatusLabel;
    
    QDialogButtonBox* m_buttonBox;
    QLabel* m_validationLabel;
    
    Student m_student;
    bool m_isEditing;
    FirebaseStorageService* m_storageService;
    QString m_selectedPhotoPath;
    bool m_isUploading;
};

#endif // STUDENTDIALOG_H
