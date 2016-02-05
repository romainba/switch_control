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

DISTFILES += \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/AndroidManifest.xml \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
