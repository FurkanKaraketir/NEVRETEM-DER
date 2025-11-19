#include "firestoreservice.h"
#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QDebug>
#include <QLoggingCategory>
#include <QDateTime>

FirestoreService::FirestoreService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    qCInfo(firestoreLog) << "FirestoreService initialized";
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &FirestoreService::onNetworkReply);
}

void FirestoreService::setProjectId(const QString& projectId)
{
    qCInfo(firestoreLog) << "Setting project ID:" << projectId;
    m_projectId = projectId;
    m_baseUrl = QString("https://firestore.googleapis.com/v1/projects/%1/databases/(default)/documents").arg(projectId);
    qCDebug(firestoreLog) << "Base URL set to:" << m_baseUrl;
}

void FirestoreService::setApiKey(const QString& apiKey)
{
    qCInfo(firestoreLog) << "Setting API key, length:" << apiKey.length();
    if (!apiKey.isEmpty()) {
        qCDebug(firestoreLog) << "API key prefix:" << apiKey.left(10) + "...";
    }
    m_apiKey = apiKey;
}

void FirestoreService::setAuthToken(const QString& authToken)
{
    qCInfo(firestoreLog) << "Setting auth token, length:" << authToken.length();
    if (!authToken.isEmpty()) {
        qCDebug(firestoreLog) << "Auth token prefix:" << authToken.left(20) + "...";
    }
    m_authToken = authToken;
}

QString FirestoreService::buildUrl(const QString& path) const
{
    QString url = m_baseUrl;
    if (!path.isEmpty()) {
        if (!path.startsWith("/")) {
            url += "/";
        }
        url += path;
    }
    
    if (!m_apiKey.isEmpty()) {
        QUrl qurl(url);
        QUrlQuery query;
        query.addQueryItem("key", m_apiKey);
        qurl.setQuery(query);
        return qurl.toString();
    }
    
    return url;
}

QNetworkRequest FirestoreService::createRequest(const QString& url) const
{
    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Add authentication header if we have a token
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }
    
    return request;
}

void FirestoreService::getAllStudents()
{
    qCInfo(firestoreLog) << "=== Starting getAllStudents request ===";
    qCDebug(firestoreLog) << "Project ID:" << m_projectId;
    qCDebug(firestoreLog) << "Base URL:" << m_baseUrl;
    qCDebug(firestoreLog) << "Has API key:" << !m_apiKey.isEmpty();
    qCDebug(firestoreLog) << "Has auth token:" << !m_authToken.isEmpty();
    
    QString url = buildUrl("/People");
    qCInfo(firestoreLog) << "Request URL:" << url;
    
    QNetworkRequest request = createRequest(url);
    qCDebug(firestoreLog) << "Request headers:";
    const auto headers = request.rawHeaderList();
    for (const auto& header : headers) {
        qCDebug(firestoreLog) << "  " << header << ":" << request.rawHeader(header);
    }
    
    QNetworkReply* reply = m_networkManager->get(request);
    m_pendingRequests[reply] = GetAllStudents;
    qCInfo(firestoreLog) << "GET request sent, reply object:" << reply;
}

void FirestoreService::getStudent(const QString& studentId)
{
    QString url = buildUrl(QString("/People/%1").arg(studentId));
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply* reply = m_networkManager->get(request);
    m_pendingRequests[reply] = GetStudent;
    m_requestIds[reply] = studentId;
}

void FirestoreService::addStudent(const Student& student)
{
    qCInfo(firestoreLog) << "=== Starting addStudent request ===";
    qCInfo(dataLog) << "Adding student:" << student.getName() << "(" << student.getEmail() << ")";
    
    QString url = buildUrl("/People");
    qCDebug(firestoreLog) << "Add student URL:" << url;
    QNetworkRequest request = createRequest(url);
    
    // Convert Student to Firestore document format
    QJsonObject fields;
    
    // Create a mutable copy to set the lastUpdateTime
    Student updatedStudent = student;
    updatedStudent.setLastUpdateTime(QDateTime::currentDateTimeUtc());
    
    QJsonObject studentJson = updatedStudent.toJson();
    
    for (auto it = studentJson.begin(); it != studentJson.end(); ++it) {
        QJsonObject field;
        QJsonValue value = it.value();
        
        if (value.isString()) {
            field["stringValue"] = value.toString();
        } else if (value.isBool()) {
            field["booleanValue"] = value.toBool();
        } else if (value.isDouble()) {
            // Note: "number" field is now a string (phone number), so this should only handle "year"
            field["integerValue"] = QString::number(value.toInt());
        }
        
        fields[it.key()] = field;
    }
    
    QJsonObject document;
    document["fields"] = fields;
    
    QJsonDocument jsonDoc(document);
    QByteArray data = jsonDoc.toJson();
    
    qCDebug(dataLog) << "Student JSON data:" << jsonDoc.toJson(QJsonDocument::Compact);
    qCInfo(firestoreLog) << "POST request data size:" << data.size() << "bytes";
    
    QNetworkReply* reply = m_networkManager->post(request, data);
    m_pendingRequests[reply] = AddStudent;
    qCInfo(firestoreLog) << "POST request sent, reply object:" << reply;
}

void FirestoreService::updateStudent(const Student& student)
{
    qCInfo(firestoreLog) << "=== Starting updateStudent request ===";
    qCInfo(dataLog) << "Updating student:" << student.getName() << "ID:" << student.getId();
    
    // Check if student has a valid ID
    if (student.getId().isEmpty()) {
        qCCritical(firestoreLog) << "Cannot update student: ID is empty";
        emit errorOccurred("Cannot update student: Student ID is missing. This usually means the student was not properly loaded from the database.");
        return;
    }
    
    QString url = buildUrl(QString("/People/%1").arg(student.getId()));
    qCDebug(firestoreLog) << "Update student URL:" << url;
    QNetworkRequest request = createRequest(url);
    
    // Convert Student to Firestore document format
    QJsonObject fields;
    
    // Create a mutable copy to update the lastUpdateTime
    Student updatedStudent = student;
    updatedStudent.setLastUpdateTime(QDateTime::currentDateTimeUtc());
    
    QJsonObject studentJson = updatedStudent.toJson();
    
    for (auto it = studentJson.begin(); it != studentJson.end(); ++it) {
        QJsonObject field;
        QJsonValue value = it.value();
        
        if (value.isString()) {
            field["stringValue"] = value.toString();
        } else if (value.isBool()) {
            field["booleanValue"] = value.toBool();
        } else if (value.isDouble()) {
            // Note: "number" field is now a string (phone number), so this should only handle "year"
            field["integerValue"] = QString::number(value.toInt());
        }
        
        fields[it.key()] = field;
    }
    
    QJsonObject document;
    document["fields"] = fields;
    
    QJsonDocument jsonDoc(document);
    QByteArray data = jsonDoc.toJson();
    
    qCDebug(dataLog) << "Updated student JSON data:" << jsonDoc.toJson(QJsonDocument::Compact);
    qCInfo(firestoreLog) << "PATCH request data size:" << data.size() << "bytes";
    
    QNetworkReply* reply = m_networkManager->sendCustomRequest(request, "PATCH", data);
    m_pendingRequests[reply] = UpdateStudent;
    m_requestIds[reply] = student.getId();
    qCInfo(firestoreLog) << "PATCH request sent, reply object:" << reply;
}

void FirestoreService::deleteStudent(const QString& studentId)
{
    qCInfo(firestoreLog) << "=== Starting deleteStudent request ===";
    qCInfo(dataLog) << "Deleting student ID:" << studentId;
    
    // Check if student ID is valid
    if (studentId.isEmpty()) {
        qCCritical(firestoreLog) << "Cannot delete student: ID is empty";
        emit errorOccurred("Cannot delete student: Student ID is missing.");
        return;
    }
    
    QString url = buildUrl(QString("/People/%1").arg(studentId));
    qCDebug(firestoreLog) << "Delete student URL:" << url;
    QNetworkRequest request = createRequest(url);
    
    QNetworkReply* reply = m_networkManager->deleteResource(request);
    m_pendingRequests[reply] = DeleteStudent;
    m_requestIds[reply] = studentId;
    qCInfo(firestoreLog) << "DELETE request sent, reply object:" << reply;
}

void FirestoreService::onNetworkReply(QNetworkReply* reply)
{
    if (!reply) {
        qCWarning(firestoreLog) << "Received null reply in onNetworkReply";
        return;
    }
    
    qCInfo(firestoreLog) << "=== Processing network reply ===";
    qCInfo(firestoreLog) << "Reply object:" << reply;
    qCInfo(firestoreLog) << "Reply URL:" << reply->url().toString();
    qCInfo(firestoreLog) << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qCInfo(firestoreLog) << "Network Error:" << reply->error() << reply->errorString();
    
    // Ensure reply is properly cleaned up
    reply->deleteLater();
    
    if (!m_pendingRequests.contains(reply)) {
        qCWarning(firestoreLog) << "Reply not found in pending requests - URL:" << reply->url().toString();
        qCWarning(firestoreLog) << "This might indicate a timing issue or unexpected reply";
        return;
    }
    
    RequestType requestType = m_pendingRequests.take(reply);
    QString requestId = m_requestIds.take(reply);
    
    QString requestTypeStr;
    switch (requestType) {
    case GetAllStudents: requestTypeStr = "GetAllStudents"; break;
    case GetStudent: requestTypeStr = "GetStudent"; break;
    case AddStudent: requestTypeStr = "AddStudent"; break;
    case UpdateStudent: requestTypeStr = "UpdateStudent"; break;
    case DeleteStudent: requestTypeStr = "DeleteStudent"; break;
    }
    
    qCInfo(firestoreLog) << "Processing" << requestTypeStr << "response";
    if (!requestId.isEmpty()) {
        qCDebug(firestoreLog) << "Request ID:" << requestId;
    }
    
    // Log response headers
    qCDebug(firestoreLog) << "Response headers:";
    const auto headers = reply->rawHeaderPairs();
    for (const auto& header : headers) {
        qCDebug(firestoreLog) << "  " << header.first << ":" << header.second;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qCCritical(firestoreLog) << "Network error occurred:" << reply->error() << reply->errorString();
        QByteArray errorData = reply->readAll();
        if (!errorData.isEmpty()) {
            qCDebug(firestoreLog) << "Error response body:" << errorData;
        }
        emit errorOccurred(QString("Network error: %1").arg(reply->errorString()));
        return;
    }
    
    switch (requestType) {
    case GetAllStudents:
        handleGetAllStudentsReply(reply);
        break;
    case GetStudent:
        handleGetStudentReply(reply);
        break;
    case AddStudent:
        handleAddStudentReply(reply);
        break;
    case UpdateStudent:
        handleUpdateStudentReply(reply);
        break;
    case DeleteStudent:
        handleDeleteStudentReply(reply, requestId);
        break;
    }
}

void FirestoreService::handleGetAllStudentsReply(QNetworkReply* reply)
{
    qCInfo(firestoreLog) << "=== Processing GetAllStudents response ===";
    
    QByteArray data = reply->readAll();
    qCInfo(firestoreLog) << "Response data size:" << data.size() << "bytes";
    qCDebug(dataLog) << "Raw response data:" << data;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCCritical(firestoreLog) << "JSON parse error:" << error.errorString();
        qCDebug(firestoreLog) << "Failed to parse data:" << data;
        emit errorOccurred(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    qCDebug(firestoreLog) << "JSON parsed successfully";
    
    QList<Student> students;
    QJsonObject root = doc.object();
    
    qCDebug(dataLog) << "Root object keys:" << root.keys();
    
    QJsonArray documents = root["documents"].toArray();
    qCInfo(dataLog) << "Found" << documents.size() << "documents in response";
    
    for (int i = 0; i < documents.size(); ++i) {
        const QJsonValue& value = documents[i];
        qCDebug(dataLog) << "Processing document" << (i + 1) << "of" << documents.size();
        QJsonObject document = value.toObject();
        
        // Extract document ID from the document name
        QString documentName = document["name"].toString();
        QString studentId = documentName.split("/").last();
        qCDebug(dataLog) << "Document name:" << documentName << "Extracted ID:" << studentId;
        
        QJsonObject fields = document["fields"].toObject();
        
        Student student;
        QJsonObject studentJson;
        
        // Convert Firestore fields back to regular JSON
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            QJsonObject field = it.value().toObject();
            
            if (field.contains("stringValue")) {
                studentJson[it.key()] = field["stringValue"].toString();
            } else if (field.contains("booleanValue")) {
                studentJson[it.key()] = field["booleanValue"].toBool();
            } else if (field.contains("integerValue")) {
                if (it.key() == "number") {
                    // Phone number is now stored as string, check both string and integer fields for backward compatibility
                    if (field.contains("stringValue")) {
                        studentJson[it.key()] = field["stringValue"].toString();
                    } else if (field.contains("integerValue")) {
                        // Convert old integer format to string for backward compatibility
                        studentJson[it.key()] = field["integerValue"].toString();
                    }
                } else {
                    studentJson[it.key()] = field["integerValue"].toString().toInt();
                }
            }
        }
        
        student.fromJson(studentJson);
        student.setId(studentId);  // Set the extracted document ID
        qCDebug(dataLog) << "Parsed student:" << student.getName() << "(" << student.getEmail() << ") with ID:" << studentId;
        students.append(student);
    }
    
    qCInfo(dataLog) << "Successfully parsed" << students.size() << "students";
    qCInfo(firestoreLog) << "Emitting studentsReceived signal with" << students.size() << "students";
    emit studentsReceived(students);
}

void FirestoreService::handleGetStudentReply(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    QJsonObject document = doc.object();
    
    // Extract document ID from the document name
    QString documentName = document["name"].toString();
    QString studentId = documentName.split("/").last();
    
    QJsonObject fields = document["fields"].toObject();
    
    Student student;
    QJsonObject studentJson;
    
    // Convert Firestore fields back to regular JSON
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        QJsonObject field = it.value().toObject();
        
        if (field.contains("stringValue")) {
            studentJson[it.key()] = field["stringValue"].toString();
        } else if (field.contains("booleanValue")) {
            studentJson[it.key()] = field["booleanValue"].toBool();
        } else if (field.contains("integerValue")) {
            if (it.key() == "number") {
                studentJson[it.key()] = field["integerValue"].toString().toLongLong();
            } else {
                studentJson[it.key()] = field["integerValue"].toString().toInt();
            }
        }
    }
    
    student.fromJson(studentJson);
    student.setId(studentId);  // Set the extracted document ID
    emit studentReceived(student);
}

void FirestoreService::handleAddStudentReply(QNetworkReply* reply)
{
    qCInfo(firestoreLog) << "=== Processing AddStudent response ===";
    
    QByteArray data = reply->readAll();
    qCInfo(firestoreLog) << "Response data size:" << data.size() << "bytes";
    qCDebug(dataLog) << "Raw response data:" << data;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCCritical(firestoreLog) << "JSON parse error:" << error.errorString();
        emit errorOccurred(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    // Extract the created student from response
    QJsonObject document = doc.object();
    QString documentName = document["name"].toString();
    QString studentId = documentName.split("/").last();
    
    QJsonObject fields = document["fields"].toObject();
    Student student;
    QJsonObject studentJson;
    
    // Convert Firestore fields back to regular JSON
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        QJsonObject field = it.value().toObject();
        
        if (field.contains("stringValue")) {
            studentJson[it.key()] = field["stringValue"].toString();
        } else if (field.contains("booleanValue")) {
            studentJson[it.key()] = field["booleanValue"].toBool();
        } else if (field.contains("integerValue")) {
            if (it.key() == "number") {
                studentJson[it.key()] = field["integerValue"].toString().toLongLong();
            } else {
                studentJson[it.key()] = field["integerValue"].toString().toInt();
            }
        }
    }
    
    student.fromJson(studentJson);
    student.setId(studentId);
    
    qCInfo(dataLog) << "Successfully added student:" << student.getName() << "with ID:" << studentId;
    qCInfo(firestoreLog) << "Emitting studentAdded signal";
    emit studentAdded(student);
}

void FirestoreService::handleUpdateStudentReply(QNetworkReply* reply)
{
    qCInfo(firestoreLog) << "=== Processing UpdateStudent response ===";
    
    QByteArray data = reply->readAll();
    qCInfo(firestoreLog) << "Response data size:" << data.size() << "bytes";
    qCDebug(dataLog) << "Raw response data:" << data;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCCritical(firestoreLog) << "JSON parse error:" << error.errorString();
        emit errorOccurred(QString("JSON parse error: %1").arg(error.errorString()));
        return;
    }
    
    QJsonObject document = doc.object();
    
    // Extract document ID from the document name
    QString documentName = document["name"].toString();
    QString studentId = documentName.split("/").last();
    
    QJsonObject fields = document["fields"].toObject();
    
    Student student;
    QJsonObject studentJson;
    
    // Convert Firestore fields back to regular JSON
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        QJsonObject field = it.value().toObject();
        
        if (field.contains("stringValue")) {
            studentJson[it.key()] = field["stringValue"].toString();
        } else if (field.contains("booleanValue")) {
            studentJson[it.key()] = field["booleanValue"].toBool();
        } else if (field.contains("integerValue")) {
            if (it.key() == "number") {
                studentJson[it.key()] = field["integerValue"].toString().toLongLong();
            } else {
                studentJson[it.key()] = field["integerValue"].toString().toInt();
            }
        }
    }
    
    student.fromJson(studentJson);
    student.setId(studentId);  // Set the extracted document ID
    
    qCInfo(dataLog) << "Successfully updated student:" << student.getName() << "ID:" << studentId;
    qCInfo(firestoreLog) << "Emitting studentUpdated signal";
    emit studentUpdated(student);
}

void FirestoreService::handleDeleteStudentReply(QNetworkReply* reply, const QString& studentId)
{
    qCInfo(firestoreLog) << "=== Processing DeleteStudent response ===";
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    
    qCDebug(firestoreLog) << "Delete response status code:" << statusCode;
    qCDebug(firestoreLog) << "Delete response data:" << responseData;
    
    if (statusCode == 200) {
        qCInfo(dataLog) << "Successfully deleted student ID:" << studentId;
        qCInfo(firestoreLog) << "Emitting studentDeleted signal";
        emit studentDeleted(studentId);
    } else {
        QString errorMsg = QString("Failed to delete student. HTTP Status: %1").arg(statusCode);
        if (!responseData.isEmpty()) {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (!jsonDoc.isNull()) {
                QJsonObject errorObj = jsonDoc.object();
                if (errorObj.contains("error")) {
                    QJsonObject error = errorObj["error"].toObject();
                    errorMsg += QString(" - %1").arg(error["message"].toString());
                }
            }
        }
        qCWarning(firestoreLog) << "Delete failed:" << errorMsg;
        emit errorOccurred(errorMsg);
    }
}
