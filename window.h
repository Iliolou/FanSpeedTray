#ifndef WINDOW_H
#define WINDOW_H

#include <QSystemTrayIcon>

#ifndef QT_NO_SYSTEMTRAYICON

#include <QDialog>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QGroupBox;
class QLabel;
class QMenu;
class QPushButton;
class QSpinBox;
class QHBoxLayout;
//class QSettings;
QT_END_NAMESPACE


class Window : public QDialog
{
    Q_OBJECT

public:
    Window();

    void setVisible(bool visible) override;
    void setValue(int value);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void setIcon(int index);
    void selectFan(int index);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void showMessage();
    void turnoffMonitor();
    void onQuit();

private:
    void getStatus();
    void autofans(QString flag);
    void createIconGroupBox();
    void createActions();
    void createTrayIcon();
    void loadSettings();
    void saveSettings();

    QGroupBox *iconGroupBox;
    QLabel *iconLabel;
    QComboBox *iconComboBox;
    QLabel *stepLabel;
    QSpinBox *stepSpinBox;

    QHBoxLayout *footerLayout;
    QPushButton *refreshButton;
    QComboBox *fansComboBox;
    QLabel *authorLabel;

    QAction *configAction;
    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    //QSettings *settings;
    int mode;
    QList<int> current;
    QList<int> status;
    QList<int> percent;
    QList<int> extrafans;
    QStringList fans;
    QString brightness, keyboard;
};

#endif // QT_NO_SYSTEMTRAYICON

#endif