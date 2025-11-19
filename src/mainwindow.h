#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QProgressBar>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QFrame>
#include <QToolButton>
#include <QLoggingCategory>

#include "student.h"

Q_DECLARE_LOGGING_CATEGORY(dataLog)
#include "firestoreservice.h"
#include "firebasestorageservice.h"
#include "studentdialog.h"
#include "firebaseauthservice.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMenuBar;
class QStatusBar;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    void setAuthService(FirebaseAuthService* authService);

private slots:
    void onAddStudent();
    void onEditStudent();
    void onDeleteStudent();
    void onRefreshStudents();
    void onSearchTextChanged();
    void onTableItemDoubleClicked(int row, int column);
    void onToggleFilters();
    void onFilterChanged();
    void onClearFilters();
    void onTableContextMenu(const QPoint& pos);
    
    // Firestore service slots
    void onStudentsReceived(const QList<Student>& students);
    void onStudentAdded(const Student& student);
    void onStudentUpdated(const Student& student);
    void onStudentDeleted(const QString& studentId);
    void onFirestoreError(const QString& error);
    
    // Authentication slots
    void onSignOut();
    
    // Image loading slots
    void onImageLoaded(const QString& imageUrl, const QByteArray& imageData);
    void onImageLoadFailed(const QString& imageUrl, const QString& error);
    
    // Deferred photo upload slot
    void onDeferredUploadCompleted();
    
    // Excel import/export slots
    void onExportToExcel();
    void onImportFromExcel();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupFirestore();
    void setupStorage();
    void setupFilterUI();
    void populateFilterDropdowns();
    void populateTable(const QList<Student>& students);
    void updateStudentDetails(const Student& student);
    void clearStudentDetails();
    void filterStudents();
    Student getStudentFromRow(int row) const;
    int findStudentRow(const QString& studentId) const;
    void showLoadingState(bool loading);
    void loadStudentPhoto(QLabel* photoLabel, const QString& photoUrl);
    void loadStudentDetailsPhoto(const QString& photoUrl);
    
    // UI Components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    
    // Left panel - Student list
    QWidget* m_leftPanel;
    QVBoxLayout* m_leftLayout;
    QHBoxLayout* m_searchLayout;
    QLineEdit* m_searchEdit;
    QPushButton* m_refreshButton;
    QToolButton* m_filterToggleButton;
    
    // Filter panel
    QFrame* m_filterFrame;
    QGridLayout* m_filterLayout;
    QComboBox* m_fieldFilterCombo;
    QComboBox* m_schoolFilterCombo;
    QComboBox* m_graduationFilterCombo;
    QSpinBox* m_yearFromSpinBox;
    QSpinBox* m_yearToSpinBox;
    QLineEdit* m_nameFilterEdit;
    QLineEdit* m_emailFilterEdit;
    QPushButton* m_clearFiltersButton;
    QTableWidget* m_studentsTable;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    
    // Right panel - Student details
    QWidget* m_rightPanel;
    QVBoxLayout* m_rightLayout;
    QGroupBox* m_detailsGroup;
    QFormLayout* m_detailsLayout;
    QLabel* m_nameLabel;
    QLabel* m_emailLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_fieldLabel;
    QLabel* m_schoolLabel;
    QLabel* m_numberLabel;
    QLabel* m_yearLabel;
    QLabel* m_graduationLabel;
    QLabel* m_photoLabel;
    QLabel* m_detailsPhotoLabel; // For displaying actual photo in details panel
    
    // Status and progress
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    // Data
    QList<Student> m_allStudents;
    QList<Student> m_filteredStudents;
    FirestoreService* m_firestoreService;
    FirebaseStorageService* m_storageService;
    FirebaseAuthService* m_authService;
    
    // Photo loading management
    QHash<QString, QLabel*> m_photoLabels; // URL -> QLabel mapping for image loading
    QString m_currentDetailsPhotoUrl; // Track current details panel photo URL
    StudentDialog* m_pendingPhotoDialog; // For deferred photo upload
    
    // Actions
    QAction* m_exitAction;
    QAction* m_aboutAction;
    QAction* m_settingsAction;
    QAction* m_signOutAction;
    QAction* m_exportExcelAction;
    QAction* m_importExcelAction;
};

#endif // MAINWINDOW_H
