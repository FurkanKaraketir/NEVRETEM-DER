#include "thememanager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

ThemeManager::ThemeManager()
{
    initializeDefaultColors();
}

void ThemeManager::initializeDefaultColors()
{
    // 🌊 PRIMARY DARK CYAN SHADES
    m_colors["bg-darkest"]      = QColor("#051923");  // Deep ocean base
    m_colors["bg-darker"]       = QColor("#0A2E3C");  // Dark cyan primary
    m_colors["bg-dark"]         = QColor("#0D3D4F");  // Medium dark cyan
    m_colors["bg-medium"]       = QColor("#164E63");  // Cyan accent base
    m_colors["bg-light"]        = QColor("#1A5A6F");  // Lighter cyan
    
    // ✨ GOLD & AMBER ACCENTS
    m_colors["gold-primary"]    = QColor("#FFB703");  // Bright gold
    m_colors["gold-hover"]      = QColor("#FFCB47");  // Light gold
    m_colors["gold-bright"]     = QColor("#DCAE1D");  // Rich gold
    m_colors["amber-deep"]      = QColor("#FB8500");  // Deep amber
    m_colors["amber-dark"]      = QColor("#F77F00");  // Dark amber
    
    // 🔷 CYAN HIGHLIGHTS
    m_colors["cyan-bright"]     = QColor("#06B6D4");  // Bright cyan
    m_colors["cyan-medium"]     = QColor("#0891B2");  // Medium cyan
    m_colors["cyan-light"]      = QColor("#22D3EE");  // Light cyan
    m_colors["cyan-glow"]       = QColor("#67E8F9");  // Cyan glow
    
    // 📝 TEXT COLORS
    m_colors["text-primary"]    = QColor("#F0FDFA");  // Almost white
    m_colors["text-secondary"]  = QColor("#CCFBF1");  // Light cyan tint
    m_colors["text-muted"]      = QColor("#99F6E4");  // Muted cyan
    m_colors["text-gold"]       = QColor("#FDE68A");  // Light gold
    
    // 🎨 UTILITY COLORS
    m_colors["border-dark"]     = QColor("#1E5B6F");  // Dark cyan border
    m_colors["border-medium"]   = QColor("#2C7A8F");  // Medium cyan border
    m_colors["success"]         = QColor("#10B981");  // Success green
    m_colors["disabled"]        = QColor("#475569");  // Disabled gray
}

QString ThemeManager::loadThemedStylesheet(const QString& qssPath)
{
    QFile file(qssPath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Failed to open stylesheet file:" << qssPath;
        return QString();
    }
    
    QTextStream in(&file);
    QString qss = in.readAll();
    file.close();
    
    return processStylesheet(qss);
}

QString ThemeManager::processStylesheet(const QString& qss)
{
    QString result = qss;
    
    // Replace all color placeholders with actual color values
    // Format: ${color-name} or @{color-name}
    for (auto it = m_colors.constBegin(); it != m_colors.constEnd(); ++it) {
        const QString& colorName = it.key();
        const QColor& color = it.value();
        
        // Support multiple placeholder formats
        QStringList placeholders = {
            QString("${%1}").arg(colorName),
            QString("@{%1}").arg(colorName),
            QString("var(%1)").arg(colorName)
        };
        
        for (const QString& placeholder : placeholders) {
            result.replace(placeholder, color.name());
        }
        
        // Also support rgba format with alpha channel
        // Format: ${color-name, alpha} where alpha is 0.0-1.0
        QRegularExpression rgbaPattern(QString("\\$\\{%1,\\s*(\\d*\\.?\\d+)\\}").arg(colorName));
        QRegularExpressionMatchIterator matches = rgbaPattern.globalMatch(result);
        
        QList<QPair<QString, QString>> replacements;
        while (matches.hasNext()) {
            QRegularExpressionMatch match = matches.next();
            QString fullMatch = match.captured(0);
            double alpha = match.captured(1).toDouble();
            
            // Convert to rgba format
            QString rgba = QString("rgba(%1, %2, %3, %4)")
                .arg(color.red())
                .arg(color.green())
                .arg(color.blue())
                .arg(alpha);
            
            replacements.append(qMakePair(fullMatch, rgba));
        }
        
        // Apply all rgba replacements
        for (const auto& replacement : replacements) {
            result.replace(replacement.first, replacement.second);
        }
    }
    
    return result;
}

QColor ThemeManager::getColor(const QString& colorName) const
{
    return m_colors.value(colorName, QColor());
}

void ThemeManager::setColor(const QString& colorName, const QColor& color)
{
    m_colors[colorName] = color;
}

void ThemeManager::resetToDefaults()
{
    m_colors.clear();
    initializeDefaultColors();
}

QMap<QString, QColor> ThemeManager::getAllColors() const
{
    return m_colors;
}

