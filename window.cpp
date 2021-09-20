#include "window.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <QCoreApplication>
#include <QAction>
#include <QComboBox>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFileInfo>
//#include <QDebug>

Window::Window()
{
    getStatus();
    createIconGroupBox();

    iconLabel->setMinimumWidth(iconLabel->sizeHint().width());

    createActions();
    createTrayIcon();

    connect(refreshButton, &QAbstractButton::clicked, this, &Window::showMessage);
    connect(iconComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Window::setIcon);
    connect(fansComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Window::selectFan);
    connect(stepSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &Window::saveSettings);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &Window::iconActivated);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(iconGroupBox);
    mainLayout->addLayout(footerLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(authorLabel);
    mainLayout->setAlignment(authorLabel,Qt::AlignRight);
    setLayout(mainLayout);

    setIcon(0);
    trayIcon->installEventFilter(this);
    trayIcon->show();

    resize(390, 170); setFixedHeight(sizeHint().height()+30);
}

void Window::getStatus()
{
    status.clear(); current.clear(); percent.clear(); fans.clear(); extrafans.clear();
    QProcess *process = new QProcess;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << "awk '(FNR==1){print FILENAME}1' /sys/class/hwmon/hwmon?/pwm?");
    process->start();
#else
    QString cmd = "/bin/sh -c \"awk '(FNR==1){print FILENAME}1' /sys/class/hwmon/hwmon?/pwm?\"";
    process->start(cmd);
#endif
    process->waitForFinished();
    QByteArray r = process->readAllStandardOutput(); r.chop(1);
    QList<QByteArray> lines = r.split('\n');

    if (lines.length()<2) exit(0);

    // Store fans files
    bool ok; int ccvalue;
    for( int i=0; i<lines.count(); i+=2 ) {
	ccvalue = lines[i+1].toInt(&ok, 10);
	if (ok && ccvalue>0) {
		fans.append(lines[i]);
		if (status.isEmpty()) { status.append(ccvalue); status.append(255); }
		else extrafans.append(ccvalue);
	}
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << "awk '(FNR==1){print FILENAME}1' /sys/class/backlight/*/*_brightness");
    process->start();
#else
    cmd = "/bin/sh -c \"awk '(FNR==1){print FILENAME}1' /sys/class/backlight/*/*_brightness\""; // to cover acpi_video as well
    process->start(cmd);
#endif
    process->waitForFinished();
    r = process->readAllStandardOutput(); r.chop(1);
    lines = r.split('\n');

    for( int i=0; i<lines.count(); ++i ) {
	if (i==0) { brightness = QString(lines[i]).remove("actual_"); }
	else if ((i % 2) && (i<5)) { // get the first set
		ccvalue = lines[i].toInt(&ok, 10); if (ok) status.append(ccvalue);
	}
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << "awk '(FNR==1){print FILENAME}1' /sys/class/leds/*\\:\\:kbd_backlight/*brightness");
    process->start();
#else
    cmd = "/bin/sh -c \"awk '(FNR==1){print FILENAME}1' /sys/class/leds/*\\:\\:kbd_backlight/*brightness\""; // to cover tpacpi as well
    process->start(cmd);
#endif
    process->waitForFinished();
    r = process->readAllStandardOutput(); r.chop(1);
    lines = r.split('\n');

    if (r.isEmpty()) { status.append(1);status.append(1); } // no backlight
    else keyboard = QString(lines[0]);
    //for(const QByteArray& line : lines) {
    for( int i=1; i<lines.count(); i+=2 ) {
	ccvalue = lines[i].toInt(&ok, 10); if (ok) status.append(ccvalue);
    }

    if (status.length()<6) { exit(0); }
    QFileInfo bf(brightness); if (!bf.exists()) exit(0);

    autofans("1");

    current = QList<int>() << status[4] << status[0] << status[2];
    percent = QList<int>() << qRound(double(100*current[0]/status[5])) << qRound(double(100*current[1]/status[1])) << qRound(double(100*current[2]/status[3]));
}

void Window::autofans(QString flag)
{
    QProcess *process = new QProcess;
    QString fc;
    for(const QString& fan : fans)
    {
	fc.append("echo "); fc.append(flag); fc.append(" > "); fc.append(fan); fc.append("_enable; ");
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << fc);
    process->start();
#else
    fc.prepend("/bin/sh -c \""); fc. append("\"");
    process->start(fc);
#endif
    process->waitForFinished();

    // Alternatively - simpler, but requires one qprocess for each fan
/*    for(const QString& fan : fans)
    {
	process->setStandardOutputFile(fan + "_enable", QIODevice::Truncate);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	process->setProgram("echo");
	process->setArguments(QStringList() << flag);
	process->start();
#else
	process->start("echo " + flag);
#endif
	process->waitForFinished();
    }
*/
}


void Window::setVisible(bool visible)
{
    configAction->setEnabled(isMaximized() || !visible);
    QDialog::setVisible(visible);
}

void Window::closeEvent(QCloseEvent *event)
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}
bool Window::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type()==QEvent::Wheel) {
	//event->ignore();
	QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
	int delta = wheelEvent->angleDelta().y();
	int step = stepSpinBox->value(); int value;

	int fanno = fansComboBox->currentIndex();
	if (mode==1 && fanno>0) {
		if (delta>0) value = qMin(255,extrafans[fanno-1] + step);
		else value = qMax(0,extrafans[fanno-1] - step);
		if (value!=extrafans[fanno-1]) setValue(value);
		return true;
	}

	if (mode>0) {
		if (delta>0) value = qMin(status[2*mode-1],current[mode] + step);
		else value = qMax(0,current[mode] - step);
	} else {
		if (delta>0) value = qMin(status[5],current[mode] + 1);
		else value = qMax(0,current[mode] - 1);
	}
	//qDebug() << value;
	if (value!=current[mode]) setValue(value);
        return true;
    }
    //return false; //QMainWindow::eventFilter(obj, event);
    return QObject::eventFilter(obj, event);
    //return Window::eventFilter(obj, event);
}

void Window::setIcon(int index)
{
    mode = (index + 1 )% iconComboBox->count();
    QString modetext = iconComboBox->itemText(index);
    QIcon icon = iconComboBox->itemIcon(index);
    trayIcon->setIcon(icon);
    setWindowIcon(icon);
    setWindowTitle(tr(modetext.toUtf8() + " Settings"));

    if (mode==1 && fans.length()>1) fansComboBox->setVisible(true); else fansComboBox->setVisible(false);
    int fanno = fansComboBox->currentIndex();
    if (mode==1 && fanno>0) {
	trayIcon->setToolTip("<nobr>" + modetext + ": <b>" + QString::number(qRound(double(0.3922*extrafans[fanno-1]))) + " %</b></nobr>");
	iconGroupBox->setTitle(tr("Current value: ") + QString::number(extrafans[fanno-1]));
    } else {
    trayIcon->setToolTip("<nobr>" + modetext + ": <b>" + QString::number(percent[mode]) + " %</b></nobr>");
    iconGroupBox->setTitle(tr("Current value: ") + QString::number(current[mode]));
    }
}

void Window::selectFan(int index)
{
    QString modetext = iconComboBox->itemText(index);
    if (index>0) {
	iconGroupBox->setTitle(tr("Current value: ") + QString::number(extrafans[index-1]));
	trayIcon->setToolTip("<nobr>" + modetext + ": <b>" + QString::number(qRound(double(0.3922*extrafans[index-1]))) + " %</b></nobr>");
    } else {
	iconGroupBox->setTitle(tr("Current value: ") + QString::number(current[1]));
	trayIcon->setToolTip("<nobr>" + modetext + ": <b>" + QString::number(percent[1]) + " %</b></nobr>");
    }
}

void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
	iconComboBox->setCurrentIndex((mode+1)%3);
	setVisible(true); break;
    case QSystemTrayIcon::Trigger:
        iconComboBox->setCurrentIndex(mode); break;
    case QSystemTrayIcon::MiddleClick:
        turnoffMonitor(); break;
    default:
        ;
    }
}

void Window::setValue(int value)
{
    QProcess *process = new QProcess;
    QString dev;
    int fanno = fansComboBox->currentIndex();
    switch( mode ) {
	case 1: dev = fans[fanno]; break;
	case 2: dev = brightness; break;
	case 0: dev = keyboard; break;
    }
    process->setStandardOutputFile(dev, QIODevice::Truncate);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("echo"); process->setArguments(QStringList() << QString::number(value)); process->start();
#else
    process->start("echo " + QString::number(value));
#endif
    process->waitForFinished();
    if (process->error()==QProcess::UnknownError) {
	if (mode==1 && fanno>0) {
		extrafans[fanno-1] = value;
		trayIcon->setToolTip("<nobr>" + tr("New value: ") + ": <b>" + QString::number(qRound(double(0.3922*value))) + " %</b></nobr>");
	} else {
		current[mode] = value;
		int statusindex = 2*mode-1; if (statusindex<0) statusindex=5;
		percent[mode] = qRound(double(100*value/status[statusindex]));
		trayIcon->setToolTip("<nobr>" + tr("New value: ") + ": <b>" + QString::number(percent[mode]) + " %</b></nobr>");
	}
	iconGroupBox->setTitle(tr("Current value: ") + QString::number(value));
   } //else { QByteArray r = process->readAllStandardError(); qDebug() << QString(r); }
}

void Window::turnoffMonitor()
{
    QProcess *process = new QProcess;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << "DISPLAY=:0 sleep 0.2; /usr/bin/xset dpms force off");
    process->start();
#else
    QString cmd = "/bin/sh -c \"DISPLAY=:0 sleep 0.2; /usr/bin/xset dpms force off\"";
    process->start(cmd);
#endif
    process->waitForFinished();
}

void Window::showMessage()
{
    getStatus();
    int v1 = current[1];
    int fanno = fansComboBox->currentIndex();
    if (mode==1 && fanno>0) v1 = extrafans[fanno-1];
    QString report = tr("Fan Speed") + ": " + QString::number(v1) + "\n" +
		tr("Monitor Brightness") + ": " + QString::number(current[2]) + "\n" +
		tr("Keyboard Backlight") + ": " + QString::number(current[0]);
    trayIcon->showMessage(tr("Current values"), report, QSystemTrayIcon::Information, 8000);
}


void Window::createIconGroupBox()
{
    iconGroupBox = new QGroupBox(tr("Settings"));
    iconGroupBox->setStyleSheet("QGroupBox { font-weight: bold; } ");
    iconLabel = new QLabel("Mode:");

    iconComboBox = new QComboBox;
    iconComboBox->addItem(QIcon(":/icons/fan.png"), tr("Fan Speed"));
    iconComboBox->addItem(QIcon(":/icons/brightness.png"), tr("Monitor Brightness"));
    iconComboBox->addItem(QIcon(":/icons/keyboard.png"), tr("Keyboard Backlight"));


    QHBoxLayout *iconLayout = new QHBoxLayout;
    //iconLayout->addStretch();

    stepLabel = new QLabel(tr("Step:"));

    stepSpinBox = new QSpinBox;
    stepSpinBox->setRange(1, 60);
    stepSpinBox->setValue(30);

    iconLayout->addWidget(iconLabel,0,Qt::AlignLeft);
    iconLayout->addWidget(iconComboBox,1,Qt::AlignLeft);
    iconLayout->addWidget(stepLabel,0,Qt::AlignRight);
    iconLayout->addWidget(stepSpinBox);
    //iconLayout->addWidget(showIconCheckBox);
    iconGroupBox->setLayout(iconLayout);

    fansComboBox = new QComboBox;
    int nofans = fans.length();

    // Get fan labels if exist
    QProcess *process = new QProcess;
    QString fc;
    for(QString& fan : fans)
    {
	fc.append("cat "); fc.append(fan); fc.append("_label; ");
    }
    fc.replace(QString("pwm"),QString("fan"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    process->setProgram("/bin/sh");
    process->setArguments(QStringList() << "-c" << fc);
    process->start();
#else
    fc.prepend("/bin/sh -c \""); fc. append("\"");
    process->start(fc);
#endif
    process->waitForFinished();
    QByteArray r = process->readAllStandardOutput(); r.chop(1);
    QList<QByteArray> lines = r.split('\n');
    if (lines.length()==nofans) {
	for(const QByteArray& line : lines) { fansComboBox->addItem(line); }
	fansComboBox->setFixedSize(140, 30);
    } else {
	for (int i = 0; i < nofans; i++) { fansComboBox->addItem(tr("Fan ")+QString::number(i)); }
	fansComboBox->setFixedSize(70, 30);
	//fansComboBox->setMinimumWidth(minimumSizeHint().width());
    }

    refreshButton = new QPushButton(tr("Refresh"));
    refreshButton->setFixedSize(QSize(100, 30));

    authorLabel = new QLabel(tr("By Thanassis Misaridis"));
    QFont font = authorLabel->font();
    font.setBold(true); font.setPointSize(8);
    authorLabel->setFont(font);

    footerLayout = new QHBoxLayout;
    if (nofans>1) { footerLayout->addWidget(fansComboBox); }
    //footerLayout->addWidget(fansComboBox);
    footerLayout->addWidget(refreshButton);
    footerLayout->setAlignment(refreshButton,Qt::AlignCenter);
}

void Window::createActions()
{
    configAction = new QAction(tr("&Options"), this);
    configAction->setIcon(QIcon(":/icons/menuOptions.png"));
    connect(configAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    quitAction->setIcon(QIcon(":/icons/menuQuit.png"));
    //connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(quitAction, &QAction::triggered, this, &Window::onQuit);
}

void Window::onQuit()
{
    // do proper closing here - put fans back to auto mode
    autofans("2");
    QCoreApplication::quit();
}

void Window::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(configAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

void Window::loadSettings()
{
    //
}

void Window::saveSettings()
{
    //settings = new QSettings(qAppName() + ".ini");
}

#endif