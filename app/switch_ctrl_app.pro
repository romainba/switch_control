QT += network widgets

HEADERS       = client.h \
    switch.h \
    device.h \
    client.h
SOURCES       = client.cpp \
    main.cpp \
    device.cpp

# install
target.path = build
INSTALLS += target
