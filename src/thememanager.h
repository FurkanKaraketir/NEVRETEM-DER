#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QMap>
#include <QColor>

/**
 * @brief ThemeManager - Manages application theming with color variables
 * 
 * This class provides a centralized way to manage colors for the application theme.
 * Colors are defined in C++ and injected into QSS using placeholder replacement.
 * 
 * Usage:
 *   ThemeManager theme;
 *   QString stylesheet = theme.loadThemedStylesheet(":/resources/style.qss");
 *   qApp->setStyleSheet(stylesheet);
 */
class ThemeManager
{
public:
    ThemeManager();
    
    /**
     * @brief Loads a QSS file and replaces color placeholders with actual values
     * @param qssPath Path to the QSS file (can be a Qt resource path)
     * @return The processed stylesheet with all color variables replaced
     */
    QString loadThemedStylesheet(const QString& qssPath);
    
    /**
     * @brief Get a specific color by name
     * @param colorName The name of the color variable (e.g., "bg-darkest")
     * @return The QColor value, or an invalid color if not found
     */
    QColor getColor(const QString& colorName) const;
    
    /**
     * @brief Set a custom color value
     * @param colorName The name of the color variable
     * @param color The color value to set
     */
    void setColor(const QString& colorName, const QColor& color);
    
    /**
     * @brief Reset all colors to default values
     */
    void resetToDefaults();
    
    /**
     * @brief Get all defined color variables
     * @return Map of color names to color values
     */
    QMap<QString, QColor> getAllColors() const;

private:
    void initializeDefaultColors();
    QString processStylesheet(const QString& qss);
    
    QMap<QString, QColor> m_colors;
};

#endif // THEMEMANAGER_H

