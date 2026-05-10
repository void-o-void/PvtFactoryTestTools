cmake_minimum_required(VERSION 3.16)
project(FactoryTestModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt/6.11.0/mingw_64")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        SerialPort
        Qml
        Quick
        REQUIRED
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

file(GLOB SRC_LIST
        "process/*.cpp"
        "hmi/*.cpp"
        "hmi/*.hpp"
        "hmi/common/*.cpp"
        "hmi/common/*.hpp"
)

add_executable(FactoryTestModule WIN32 ${SRC_LIST} resources/resources.qrc)

include_directories(FactoryTestModule
        "common"
        "common/protocol_core"
        "hmi"
        "hmi/common"
        "process"
)

target_link_libraries(FactoryTestModule PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::SerialPort
        Qt::Qml
        Qt::Quick
)

qt_add_qml_module(FactoryTestModule
        URI FactoryTestHmi
        QML_FILES
            hmi/Home.qml
            hmi/Head.qml
            hmi/UiTestPanel.qml
            hmi/UiFactoryTest.qml
            hmi/UiTestCard.qml
            hmi/UiFactoryTestRtTable.qml
        RESOURCES
            resources/pvt.png
)

if(WIN32)
    # 1. 先获取 Qt 的 bin 目录
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    cmake_path(GET QT_QMAKE_EXECUTABLE PARENT_PATH QT_BIN_DIR)

    # 2. 查找 windeployqt
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "windeployqt not found at ${QT_BIN_DIR}")
    endif()

    # 3. 构建后自动运行 windeployqt
    add_custom_command(TARGET FactoryTestModule POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt for FactoryTestModule..."
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            "$<TARGET_FILE:FactoryTestModule>"
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/hmi"
            --no-translations
            --no-compiler-runtime
            COMMENT "Deploying Qt runtime dependencies..."
    )
endif()
