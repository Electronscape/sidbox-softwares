include("/mnt/LinuxDatas/work/sidbox-softwares/graphics-test/build/Desktop-Release/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/graphics-smsemu-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/mnt/LinuxDatas/work/sidbox-softwares/graphics-test/build/Desktop-Release/graphics-smsemu"
    GENERATE_QT_CONF
)
