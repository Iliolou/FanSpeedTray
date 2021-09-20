
QMAKE_CXXFLAGS += -O2 -pipe -march=native
QT       += core gui
 
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
requires(qtConfig(combobox))

TARGET = fanspeedtray
TEMPLATE = app
CONFIG += c++11 lrelease embed_translations
LRELEASE_DIR=. 
QM_FILES_RESOURCE_PREFIX=/
 
SOURCES += main.cpp \
        window.cpp
 
HEADERS  += window.h
 
#FORMS    += window.ui

RESOURCES     = fanspeedtray.qrc
TRANSLATIONS += i18n_el.ts
LCONVERT_LANGS=el
include(translations.pri)

unix {

	# variables
	OBJECTS_DIR = .obj
	MOC_DIR     = .moc
	UI_DIR      = .ui

	isEmpty(PREFIX) {
		PREFIX = /usr
	}

	isEmpty(BINDIR) {
		BINDIR = $${PREFIX}/bin
	}

	isEmpty(DATADIR) {
		DATADIR = $${PREFIX}/share
	}
	# make install
	INSTALLS += target desktop icon

	target.path = $${BINDIR}

	desktop.path = $${DATADIR}/applications
	desktop.files += $${TARGET}.desktop

	icon.path = $${DATADIR}/icons/hicolor/48x48/apps
	icon.files += icons/$${TARGET}.png

}