#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>
#include "student.h"

class QVBoxLayout;
class QGridLayout;
class QLabel;
class QProgressBar;

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(const QList<Student>& students, QWidget *parent = nullptr);

private:
    void setupUI();
    void calculateStatistics();
    
    // Helper to create a summary card
    QWidget* createSummaryCard(const QString& title, const QString& value, const QString& iconPath, const QString& color);
    
    // Helper to create a progress bar row for charts
    QWidget* createChartRow(const QString& label, int value, int total, const QString& color);

    QList<Student> m_students;
    
    // Data holders
    int m_totalStudents;
    int m_graduates;
    int m_activeStudents;
    QMap<QString, int> m_schoolDistribution;
    QMap<QString, int> m_fieldDistribution;
    QMap<int, int> m_yearDistribution;
};

#endif // STATISTICSDIALOG_H
