#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QMessageBox>
#include "window.h"
 
int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(fanspeedtray);

    QApplication app(argc, argv);

    QLocale locale;
    QTranslator appTranslator;appTranslator.load(locale, "i18n", "_", ":/");
    QCoreApplication::installTranslator(&appTranslator);

    QProcess *process = new QProcess;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << "ls /sys/class/hwmon/hwmon*/pwm?_enable");
    process->start();
#else
    QString cmd = "/bin/sh -c \"ls /sys/class/hwmon/hwmon*/pwm?_enable\""; process->start(cmd);
#endif
    process->waitForFinished();
    QByteArray r = process->readAllStandardOutput();
    if (r.isEmpty()) { 
        QMessageBox::critical(0, QObject::tr("FanSpeedTray"),
                              QObject::tr("Could not find any fan controls. " "\n" "Exiting"));
        return 1;
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(0, QObject::tr("FanSpeedTray"),
                              QObject::tr("I couldn't detect any system tray "
                                          "on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);

    Window window;
    //window.show();
    return app.exec();
}
