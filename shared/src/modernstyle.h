#pragma once

#include <QString>

class ModernStyle
{
public:
    static QString getDarkThemeStyleSheet();
    static QString getLightThemeStyleSheet();
    static QString getButtonStyle();
    static QString getInputStyle();
    static QString getCardStyle();
    
private:
    ModernStyle() = default;
};