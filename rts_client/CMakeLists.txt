set(target rts_client)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Network Multimedia Svg)

qt_standard_project_setup()

qt_add_resources(RCC_SOURCES rts_client.qrc)

qt_add_executable("${target}"
    screens/authorizationprogressscreen.cpp
    screens/lobbyscreen.cpp
    screens/profilepopup.cpp
    screens/roomsettingspopup.cpp
    screens/authorizationscreen.cpp
    screens/roleselectionscreen.cpp
    screens/roleselectionprogressscreen.cpp
    screens/readinessscreen.cpp

    main.cpp
    network_thread.cpp
    network_manager.cpp
    application.cpp
    authorizationcredentials.cpp
    roomlisttable.cpp
    mainwindow.cpp
    roomwidget.cpp
    roomentry.cpp
    actionpanelrenderer.cpp
    hud.cpp
    uniticonset.cpp
    unitgenerator.cpp
    unitsetrenderer.cpp
    unitteamrenderer.cpp
    unitrenderer.cpp
    coloredrenderer.cpp
    coloredtexturedrenderer.cpp
    texturedrenderer.cpp
    effectrenderer.cpp
    minimaprenderer.cpp
    groupoverlayrenderer.cpp
    selectionpanelrenderer.cpp
    hudrenderer.cpp
    scenerenderer.cpp
    coordmap.cpp

    "${RCC_SOURCES}"
)

target_include_directories("${target}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries("${target}" PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets Qt6::Network Qt6::Multimedia Qt6::Svg)
target_link_libraries("${target}" PRIVATE libhccn libmatchstate librtsn libapi)
