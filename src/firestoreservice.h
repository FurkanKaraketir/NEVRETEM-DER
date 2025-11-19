#ifndef FIRESTORESERVICE_H
#define FIRESTORESERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLoggingCategory>
#include "student.h"

Q_DECLARE_LOGGING_CATEGORY(firestoreLog)
Q_DECLARE_LOGGING_CATEGORY(dataLog)

class FirestoreService : public QObject
{
    Q_OBJECT

public:
    explicit FirestoreService(QObject *parent = nullptr);
    
    void setProjectId(const QString& projectId);
    void setApiKey(const QString& apiKey);
    void setAuthToken(const QString& authToken);
    
    // CRUD operations
    void getAllStudents();
    void getStudent(const QString& studentId);
    void addStudent(const Student& student);
    void updateStudent(const Student& student);
    void deleteStudent(const QString& studentId);

signals:
    void studentsReceived(const QList<Student>& students);
    void studentReceived(const Student& student);
    void studentAdded(const Student& student);
    void studentUpdated(const Student& student);
    void studentDeleted(const QString& studentId);
    void errorOccurred(const QString& error);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    QString buildUrl(const QString& path = "") const;
    QNetworkRequest createRequest(const QString& url) const;
    void handleGetAllStudentsReply(QNetworkReply* reply);
    void handleGetStudentReply(QNetworkReply* reply);
    void handleAddStudentReply(QNetworkReply* reply);
    void handleUpdateStudentReply(QNetworkReply* reply);
    void handleDeleteStudentReply(QNetworkReply* reply, const QString& studentId);
    
    QNetworkAccessManager* m_networkManager;
    QString m_projectId;
    QString m_apiKey;
    QString m_authToken;
    QString m_baseUrl;
    
    enum RequestType {
        GetAllStudents,
        GetStudent,
        AddStudent,
        UpdateStudent,
        DeleteStudent
    };
    
    QHash<QNetworkReply*, RequestType> m_pendingRequests;
    QHash<QNetworkReply*, QString> m_requestIds; // For tracking specific student IDs
};

#endif // FIRESTORESERVICE_H
