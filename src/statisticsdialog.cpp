#include "statisticsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <algorithm>

StatisticsDialog::StatisticsDialog(const QList<Student>& students, QWidget *parent)
    : QDialog(parent), m_students(students)
{
    setWindowTitle("İstatistik Paneli");
    resize(900, 700);
    
    calculateStatistics();
    setupUI();
}

void StatisticsDialog::calculateStatistics()
{
    m_totalStudents = m_students.size();
    m_graduates = 0;
    m_activeStudents = 0;
    
    for (const Student& s : m_students) {
        // Count graduates vs active
        if (s.getGraduation()) {
            m_graduates++;
        } else {
            m_activeStudents++;
        }
        
        // School distribution
        QString school = s.getSchool();
        if (school.isEmpty()) school = "Belirtilmemiş";
        m_schoolDistribution[school]++;
        
        // Field distribution
        QString field = s.getField();
        if (field.isEmpty()) field = "Belirtilmemiş";
        m_fieldDistribution[field]++;
        
        // Year distribution
        int year = s.getYear();
        if (year > 0) {
            m_yearDistribution[year]++;
        }
    }
}

void StatisticsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // Header
    QLabel* titleLabel = new QLabel("Genel Bakış");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #E5D4A4;");
    mainLayout->addWidget(titleLabel);
    
    // Scroll Area for content
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(24);
    contentLayout->setContentsMargins(0, 0, 16, 0); // Right margin for scrollbar
    
    // 1. Summary Cards Row
    QHBoxLayout* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(20);
    
    cardsLayout->addWidget(createSummaryCard("Toplam Mezun", QString::number(m_totalStudents), "", "#2B7A8C"));
    cardsLayout->addWidget(createSummaryCard("Üniversite Mezunu", QString::number(m_graduates), "", "#2C5AA0"));
    cardsLayout->addWidget(createSummaryCard("Devam Eden", QString::number(m_activeStudents), "", "#C9A962"));
    
    contentLayout->addLayout(cardsLayout);
    
    // 2. Charts Section - Split View
    QHBoxLayout* chartsLayout = new QHBoxLayout();
    chartsLayout->setSpacing(24);
    
    // Left Column: Schools
    QFrame* schoolsFrame = new QFrame();
    schoolsFrame->setObjectName("statsCard");
    schoolsFrame->setStyleSheet("#statsCard { background: rgba(30, 95, 111, 0.4); border: 1px solid #C9A962; border-radius: 12px; }");
    QVBoxLayout* schoolsLayout = new QVBoxLayout(schoolsFrame);
    schoolsLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* schoolsTitle = new QLabel("En Çok Öğrenci Olan Okullar");
    schoolsTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #E5D4A4; margin-bottom: 10px;");
    schoolsLayout->addWidget(schoolsTitle);
    
    // Sort schools by count
    QList<QPair<int, QString>> sortedSchools;
    for (auto it = m_schoolDistribution.begin(); it != m_schoolDistribution.end(); ++it) {
        sortedSchools.append({it.value(), it.key()});
    }
    std::sort(sortedSchools.begin(), sortedSchools.end(), std::greater<QPair<int, QString>>());
    
    // Show top 5
    int count = 0;
    for (const auto& pair : sortedSchools) {
        if (count++ >= 5) break;
        schoolsLayout->addWidget(createChartRow(pair.second, pair.first, m_totalStudents, "#C9A962"));
    }
    schoolsLayout->addStretch();
    
    // Right Column: Fields
    QFrame* fieldsFrame = new QFrame();
    fieldsFrame->setObjectName("statsCard");
    fieldsFrame->setStyleSheet("#statsCard { background: rgba(30, 95, 111, 0.4); border: 1px solid #C9A962; border-radius: 12px; }");
    QVBoxLayout* fieldsLayout = new QVBoxLayout(fieldsFrame);
    fieldsLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* fieldsTitle = new QLabel("En Çok Tercih Edilen Alanlar");
    fieldsTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #E5D4A4; margin-bottom: 10px;");
    fieldsLayout->addWidget(fieldsTitle);
    
    // Sort fields by count
    QList<QPair<int, QString>> sortedFields;
    for (auto it = m_fieldDistribution.begin(); it != m_fieldDistribution.end(); ++it) {
        sortedFields.append({it.value(), it.key()});
    }
    std::sort(sortedFields.begin(), sortedFields.end(), std::greater<QPair<int, QString>>());
    
    // Show top 5
    count = 0;
    for (const auto& pair : sortedFields) {
        if (count++ >= 5) break;
        fieldsLayout->addWidget(createChartRow(pair.second, pair.first, m_totalStudents, "#2B7A8C"));
    }
    fieldsLayout->addStretch();
    
    chartsLayout->addWidget(schoolsFrame);
    chartsLayout->addWidget(fieldsFrame);
    
    contentLayout->addLayout(chartsLayout);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
    
    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* closeButton = new QPushButton("Kapat");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    padding: 10px 30px;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #C9A962, stop:1 #A67C52);"
        "    color: #1E5F6F;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFD700, stop:1 #C9A962);"
        "}"
    );
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

QWidget* StatisticsDialog::createSummaryCard(const QString& title, const QString& value, const QString& iconPath, const QString& color)
{
    Q_UNUSED(iconPath);
    QFrame* card = new QFrame();
    card->setStyleSheet(QString(
        "QFrame {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 #1E3A5F);"
        "    border-radius: 12px;"
        "    border: 1px solid rgba(255, 255, 255, 0.1);"
        "}"
    ).arg(color));
    
    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("color: rgba(255, 255, 255, 0.8); font-size: 14px; font-weight: 500; background: transparent; border: none;");
    
    QLabel* valueLabel = new QLabel(value);
    valueLabel->setStyleSheet("color: #FFFFFF; font-size: 32px; font-weight: bold; margin-top: 5px; background: transparent; border: none;");
    
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addStretch();
    
    return card;
}

QWidget* StatisticsDialog::createChartRow(const QString& label, int value, int total, const QString& color)
{
    QWidget* row = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 5, 0, 5);
    layout->setSpacing(5);
    
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel(label);
    nameLabel->setStyleSheet("color: #E5D4A4; font-weight: 500; background: transparent;");
    
    QLabel* countLabel = new QLabel(QString::number(value));
    countLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; background: transparent;");
    
    headerLayout->addWidget(nameLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(countLabel);
    
    QProgressBar* bar = new QProgressBar();
    bar->setRange(0, total);
    bar->setValue(value);
    bar->setTextVisible(false);
    bar->setFixedHeight(8);
    bar->setStyleSheet(QString(
        "QProgressBar {"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "    border: none;"
        "    border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: %1;"
        "    border-radius: 4px;"
        "}"
    ).arg(color));
    
    layout->addLayout(headerLayout);
    layout->addWidget(bar);
    
    return row;
}
