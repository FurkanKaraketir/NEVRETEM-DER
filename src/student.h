#ifndef STUDENT_H
#define STUDENT_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

class Student
{
public:
    Student();
    Student(const QString& id, const QString& name, const QString& email, 
            const QString& description, const QString& field, const QString& school,
            const QString& number, int year, bool graduation, const QString& photoURL = "");

    // Getters
    QString getId() const { return m_id; }
    QString getName() const { return m_name; }
    QString getEmail() const { return m_email; }
    QString getDescription() const { return m_description; }
    QString getField() const { return m_field; }
    QString getSchool() const { return m_school; }
    QString getNumber() const { return m_number; }
    int getYear() const { return m_year; }
    bool getGraduation() const { return m_graduation; }
    QString getPhotoURL() const { return m_photoURL; }
    QDateTime getLastUpdateTime() const { return m_lastUpdateTime; }

    // Setters
    void setId(const QString& id) { m_id = id; }
    void setName(const QString& name) { m_name = name; }
    void setEmail(const QString& email) { m_email = email; }
    void setDescription(const QString& description) { m_description = description; }
    void setField(const QString& field) { m_field = field; }
    void setSchool(const QString& school) { m_school = school; }
    void setNumber(const QString& number) { m_number = number; }
    void setYear(int year) { m_year = year; }
    void setGraduation(bool graduation) { m_graduation = graduation; }
    void setPhotoURL(const QString& photoURL) { m_photoURL = photoURL; }
    void setLastUpdateTime(const QDateTime& lastUpdateTime) { m_lastUpdateTime = lastUpdateTime; }

    // JSON conversion
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    bool isValid() const;

private:
    QString m_id;
    QString m_name;
    QString m_email;
    QString m_description;
    QString m_field;
    QString m_school;
    QString m_number;
    int m_year;
    bool m_graduation;
    QString m_photoURL;
    QDateTime m_lastUpdateTime;
};

#endif // STUDENT_H
