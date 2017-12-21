export
eclipse_home=C:\ProgramData\Telit\IoT_AppZone_IDE\eclipse
SDK_VERSION=3.0.3
FW_VERSION=12_00_XX8-B014
PLUGIN_VERSION=0.8.14.121729
APPZONE_DIR=${eclipse_home}\\plugins\com.telit.appzonec.plugin.he910_ue910_${FW_VERSION}_${PLUGIN_VERSION}
APPZONE_LIB=$(APPZONE_DIR)\lib
TOOLCHAIN_PATH=${eclipse_home}\\plugins\com.telit.appzonec.toolchain.plugin.gccARMv6_493_4.9.3
APPZONE_INC=$(APPZONE_DIR)\m2m_inc
APPZONE_MAKEFILE=$(APPZONE_DIR)\makefiles
APPZONE_BIN=${eclipse_home}\\plugins\com.appzonec.plugin.prebuilt_3.0.3\prebuilt\bin
APPZONE_MAKEFILE_COMMON=${eclipse_home}\\plugins\com.appzonec.plugin_3.0.3\makefiles
TOOLCHAIN_BIN=${eclipse_home}\\plugins\com.telit.appzonec.toolchain.plugin.gccARMv6_493_4.9.3\arm_gcc493/bin/
LIB_PATH=-L "${eclipse_home}\\plugins\com.telit.appzonec.toolchain.plugin.gccARMv6_493_4.9.3\arm_gcc493/lib/gcc/arm-none-eabi/4.9.3" -L "${eclipse_home}\\plugins\com.telit.appzonec.toolchain.plugin.gccARMv6_493_4.9.3\arm_gcc493/arm-none-eabi/lib" 
