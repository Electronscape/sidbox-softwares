include("/mnt/LinuxDatas/work/sidbox-softwares/SidboxTerminal/build/Desktop-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/SidboxTerminal-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase;qtserialport")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/mnt/LinuxDatas/work/sidbox-softwares/SidboxTerminal/build/Desktop-Debug/SidboxTerminal"
    GENERATE_QT_CONF
)
