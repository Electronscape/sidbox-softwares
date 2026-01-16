include("/mnt/LinuxDatas/work/sidbox-softwares/graphics-OS_WindowingTest/Release/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/graphics-gui_testing-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/mnt/LinuxDatas/work/sidbox-softwares/graphics-OS_WindowingTest/Release/graphics-gui_testing"
    GENERATE_QT_CONF
)
