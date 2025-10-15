QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17


QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast -Woverloaded-virtual -Wfloat-equal -Wundef -Wnon-virtual-dtor -Wformat=2 -Wredundant-decls  -Wcast-align -Wzero-as-null-pointer-constant -Wdouble-promotion
#QMAKE_CXXFLAGS += -Wmissing-declarations  -Wstrict-overflow=5
#QMAKE_CXXFLAGS += -Wsign-conversion
#QMAKE_CXXFLAGS += -Wconversion
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
QMAKE_CXXFLAGS += -O2
win32: LIBS += -ldbghelp


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res/resources.qrc \

RC_ICONS = res/M6800.ico

FORMS += \
    src/dialogs/ExternalDisplay.ui \
    src/dialogs/InstructionInfoDialog.ui \
    src/mainwindow/MainWindow.ui

HEADERS += \
    src/assembler/Assembler.h \
    src/assembler/Disassembler.h \
    src/core/Core.h \
    src/dialogs/FocusAwareLineEdit.h \
    src/processor/Processor.h \
    src/dialogs/ExternalDisplay.h \
    src/dialogs/InstructionInfoDialog.h \
    src/mainwindow/MainWindow.h \
    src/utils/ActionQueue.h

SOURCES += \
    src/assembler/Assembler.cpp \
    src/assembler/Disassembler.cpp \
    src/core/Core.cpp \
    src/core/main.cpp \
    src/mainwindow/CodeMarkingSys.cpp \
    src/processor/InstructionFunctions.cpp \
    src/processor/Processor.cpp \
    src/dialogs/ExternalDisplay.cpp \
    src/dialogs/InstructionInfoDialog.cpp \
    src/mainwindow/FileManager.cpp \
    src/mainwindow/MainWindow.cpp \
    src/mainwindow/MainWindowSlots.cpp
