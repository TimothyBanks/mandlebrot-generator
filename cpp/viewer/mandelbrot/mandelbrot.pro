QT += widgets

HEADERS       = mandelbrotwidget.h \
                renderthread.h
SOURCES       = main.cpp \
                mandelbrotwidget.cpp \
                renderthread.cpp

INCLUDEPATH += ../../⁨fractal/⁨fractal/⁨include⁩ /Users/banksti/Documents/projects/fractal/fractal/include
#DEPENDPATH += ../../⁨fractal/⁨fractal/⁨include⁩
CONFIG += c++1z

unix:!mac:!vxworks:!integrity:!haiku:LIBS += -lm

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/threads/mandelbrot
INSTALLS += target
