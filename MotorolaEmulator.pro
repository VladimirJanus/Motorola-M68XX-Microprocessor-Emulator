QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res/resources.qrc

RC_ICONS = res/M6800.ico

FORMS += \
    src/ui/ExternalDisplay.ui \
    src/ui/InstructionInfoDialog.ui \
    src/ui/MainWindow.ui

HEADERS += \
    src/assembler/Assembler.h \
    src/assembler/Disassembler.h \
    src/core/Core.h \
    src/processor/Processor.h \
    src/ui/ExternalDisplay.h \
    src/ui/InstructionInfoDialog.h \
    src/ui/MainWindow.h \
    src/utils/ActionQueue.h

SOURCES += \
    src/assembler/Assembler.cpp \
    src/assembler/Disassembler.cpp \
    src/core/Core.cpp \
    src/core/main.cpp \
    src/processor/InstructionFunctions.cpp \
    src/processor/Processor.cpp \
    src/ui/ExternalDisplay.cpp \
    src/ui/InstructionInfoDialog.cpp \
    src/ui/MainWindow.cpp \
    src/ui/SelectionSys.cpp
