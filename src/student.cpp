#include "student.h"
#include <QUuid>
#include <QTimeZone>

Student::Student()
    : m_number("")
    , m_year(0)
    , m_graduation(false)
    , m_lastUpdateTime(QDateTime::currentDateTimeUtc())
{
}

Student::Student(const QString& id, const QString& name, const QString& email,
                 const QString& description, const QString& field, const QString& school,
                 const QString& number, int year, bool graduation, const QString& photoURL)
    : m_id(id)
    , m_name(name)
    , m_email(email)
    , m_description(description)
    , m_field(field)
    , m_school(school)
    , m_number(number)
    , m_year(year)
    , m_graduation(graduation)
    , m_photoURL(photoURL)
    , m_lastUpdateTime(QDateTime::currentDateTimeUtc())
{
}

QJsonObject Student::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["email"] = m_email;
    json["description"] = m_description;
    json["field"] = m_field;
    json["school"] = m_school;
    json["number"] = m_number;
    json["year"] = m_year;
    json["graduation"] = m_graduation;
    json["photoURL"] = m_photoURL;
    json["lastUpdateTime"] = m_lastUpdateTime.toString(Qt::ISODate);
    return json;
}

void Student::fromJson(const QJsonObject& json)
{
    m_id = json["id"].toString();
    m_name = json["name"].toString();
    m_email = json["email"].toString();
    m_description = json["description"].toString();
    m_field = json["field"].toString();
    m_school = json["school"].toString();
    m_number = json["number"].toString();
    m_year = json["year"].toInt();
    m_graduation = json["graduation"].toBool();
    m_photoURL = json["photoURL"].toString();
    
    // Handle lastUpdateTime with backward compatibility
    if (json.contains("lastUpdateTime") && !json["lastUpdateTime"].toString().isEmpty()) {
        m_lastUpdateTime = QDateTime::fromString(json["lastUpdateTime"].toString(), Qt::ISODate);
    } else {
        // For existing records without lastUpdateTime, use epoch time (oldest)
        m_lastUpdateTime = QDateTime::fromSecsSinceEpoch(0, QTimeZone::utc());
    }
}

bool Student::isValid() const
{
    return !m_name.isEmpty() && !m_email.isEmpty() && !m_field.isEmpty() && !m_school.isEmpty();
}
