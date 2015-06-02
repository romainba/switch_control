QT += network widgets

HEADERS       = client.h \
    switch.h
SOURCES       = client.cpp \
                main.cpp

# install
target.path = build
INSTALLS += target
