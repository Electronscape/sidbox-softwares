include("/mnt/LinuxDatas/work/sidbox-softwares/HIDFiffy/build/Desktop-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/HIDFiffy-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/mnt/LinuxDatas/work/sidbox-softwares/HIDFiffy/build/Desktop-Debug/HIDFiffy"
    GENERATE_QT_CONF
)
