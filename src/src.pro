TEMPLATE = app

TARGET = torora
mac {
    TARGET = Torora
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
}

DEFINES += QT_NO_CAST_FROM_ASCII QT_STRICT_ITERATORS

include(src.pri)

SOURCES += main.cpp

DESTDIR = ../

include(locale/locale.pri)

unix {
    INSTALLS += target translations desktop iconxpm iconsvg icon16 icon32 icon128 man man-compress

    target.path = $$BINDIR

    translations.path = $$PKGDATADIR
    translations.files += .qm/locale

    desktop.path = $$DATADIR/applications
    desktop.files += torora.desktop

    iconxpm.path = $$DATADIR/pixmaps
    iconxpm.files += data/torora.xpm

    iconsvg.path = $$DATADIR/icons/hicolor/scalable/apps
    iconsvg.files += data/torora.svg

    icon16.path = $$DATADIR/icons/hicolor/16x16/apps
    icon16.files += data/16x16/torora.png

    icon32.path = $$DATADIR/icons/hicolor/32x32/apps
    icon32.files += data/32x32/torora.png

    icon128.path = $$DATADIR/icons/hicolor/128x128/apps
    icon128.files += data/128x128/torora.png

    man.path = $$DATADIR/man/man1
    man.files += data/torora.1

    man-compress.path = $$DATADIR/man/man1
    man-compress.extra = "" "gzip -9 -f \$(INSTALL_ROOT)/$$DATADIR/man/man1/torora.1" ""
}
