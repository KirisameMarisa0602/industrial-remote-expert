#include "modernstyle.h"

QString ModernStyle::getDarkThemeStyleSheet()
{
    return R"(
        /* Main Application Dark Theme */
        QWidget {
            background-color: #2b2b2b;
            color: #ffffff;
            selection-background-color: #0078d4;
        }
        
        QMainWindow {
            background-color: #2b2b2b;
        }
        
        /* Buttons */
        QPushButton {
            padding: 8px 16px;
            border: none;
            border-radius: 6px;
            background-color: #0078d4;
            color: #ffffff;
            font-weight: bold;
            min-height: 20px;
        }
        
        QPushButton:hover {
            background-color: #106ebe;
        }
        
        QPushButton:pressed {
            background-color: #005a9e;
        }
        
        QPushButton:disabled {
            background-color: #555555;
            color: #888888;
        }
        
        QPushButton.secondary {
            background-color: #404040;
            border: 1px solid #666666;
        }
        
        QPushButton.secondary:hover {
            background-color: #505050;
        }
        
        /* Input Fields */
        QLineEdit, QTextEdit, QPlainTextEdit {
            padding: 8px;
            border: 1px solid #555555;
            border-radius: 4px;
            background-color: #404040;
            color: #ffffff;
        }
        
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border: 2px solid #0078d4;
        }
        
        /* Labels */
        QLabel {
            color: #ffffff;
        }
        
        QLabel.heading {
            font-size: 18px;
            font-weight: bold;
            color: #ffffff;
        }
        
        QLabel.subheading {
            font-size: 14px;
            color: #cccccc;
        }
        
        /* Group Boxes */
        QGroupBox {
            font-weight: bold;
            border: 2px solid #555555;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
            color: #ffffff;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        /* Lists and Trees */
        QListWidget, QTreeWidget {
            background-color: #404040;
            border: 1px solid #555555;
            border-radius: 4px;
            color: #ffffff;
            outline: none;
        }
        
        QListWidget::item, QTreeWidget::item {
            padding: 8px;
            border-bottom: 1px solid #555555;
        }
        
        QListWidget::item:selected, QTreeWidget::item:selected {
            background-color: #0078d4;
        }
        
        QListWidget::item:hover, QTreeWidget::item:hover {
            background-color: #505050;
        }
        
        /* Dock Widgets */
        QDockWidget {
            color: #ffffff;
            background-color: #2b2b2b;
        }
        
        QDockWidget::title {
            text-align: center;
            background-color: #404040;
            padding: 8px;
            border-bottom: 1px solid #555555;
        }
        
        /* Menu and Status Bar */
        QMenuBar {
            background-color: #404040;
            color: #ffffff;
            border-bottom: 1px solid #555555;
        }
        
        QMenuBar::item {
            background-color: transparent;
            padding: 8px 12px;
        }
        
        QMenuBar::item:selected {
            background-color: #0078d4;
        }
        
        QStatusBar {
            background-color: #404040;
            color: #ffffff;
            border-top: 1px solid #555555;
        }
        
        /* Splitters */
        QSplitter::handle {
            background-color: #555555;
        }
        
        QSplitter::handle:horizontal {
            width: 3px;
        }
        
        QSplitter::handle:vertical {
            height: 3px;
        }
    )";
}

QString ModernStyle::getLightThemeStyleSheet()
{
    return R"(
        /* Light Theme - TODO: Implement if needed */
        QWidget {
            background-color: #ffffff;
            color: #000000;
        }
    )";
}

QString ModernStyle::getButtonStyle()
{
    return R"(
        QPushButton {
            padding: 10px 20px;
            border: none;
            border-radius: 6px;
            background-color: #0078d4;
            color: #ffffff;
            font-weight: bold;
            min-height: 20px;
        }
        
        QPushButton:hover {
            background-color: #106ebe;
        }
        
        QPushButton:pressed {
            background-color: #005a9e;
        }
        
        QPushButton:disabled {
            background-color: #555555;
            color: #888888;
        }
    )";
}

QString ModernStyle::getInputStyle()
{
    return R"(
        QLineEdit, QTextEdit {
            padding: 8px;
            border: 1px solid #555555;
            border-radius: 4px;
            background-color: #404040;
            color: #ffffff;
        }
        
        QLineEdit:focus, QTextEdit:focus {
            border: 2px solid #0078d4;
        }
    )";
}

QString ModernStyle::getCardStyle()
{
    return R"(
        .card {
            background-color: #404040;
            border: 1px solid #555555;
            border-radius: 8px;
            padding: 16px;
            margin: 8px;
        }
    )";
}