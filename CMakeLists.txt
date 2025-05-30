cmake_minimum_required(VERSION 3.16)

project(kile)

set(KILE_VERSION CACHE STRING "Kile's version; can be used to set the internal version of Kile on MacOSX")

find_package(ECM 5.19.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMAddAppIcon)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)

include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include(CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)

kde_enable_exceptions()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Qt5 5.15 CONFIG REQUIRED
	Core
	DBus
	Widgets
	Script
	Test
)

find_package(KF5 5.108 REQUIRED COMPONENTS
	Config
	CoreAddons
	Crash
	DBusAddons
	DocTools
	GuiAddons
	I18n
	IconThemes
	KIO
	Parts
	TextEditor
	WindowSystem
	XmlGui
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Okular5 REQUIRED)
set_package_properties(Okular5 PROPERTIES
	DESCRIPTION "Okular core library"
	URL "https://okular.kde.org")

find_package(Poppler COMPONENTS Qt5)
set_package_properties("Poppler" PROPERTIES
	TYPE RECOMMENDED
	PURPOSE "Support for PDF file operations.")

if(Poppler_Qt5_FOUND)
	set(HAVE_POPPLER TRUE)
	include_directories(${Poppler_INCLUDE_DIRS})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

# find_package(SharedMimeInfo REQUIRED)

add_definitions(
#   -DQT_USE_QSTRINGBUILDER
#   -DQT_NO_CAST_FROM_ASCII
#   -DQT_NO_CAST_FROM_BYTEARRAY
#   -DQT_USE_FAST_OPERATOR_PLUS
  -DQT_NO_CAST_TO_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
)

if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_definitions(-DQT_MESSAGELOGCONTEXT)
endif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")

add_subdirectory(doc)
add_subdirectory(src)

########### install files ###############

install(
	FILES kile.xml
	DESTINATION ${XDG_MIME_INSTALL_DIR})
# update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

install(
	FILES
		kile.upd
	DESTINATION ${KDE_INSTALL_DATAROOTDIR}/kconf_update)

install(
	PROGRAMS
		kile1.6_upd.pl
		kile1.7_upd.pl
		kile1.9_upd.pl
		kile1.9-1_upd.pl
		kile2.0-1_upd.pl
		kile2.0-2_upd.pl
		kile2.0-3_upd.pl
		kile2.0-4_upd.pl
		kile2.0-5_upd.pl
	DESTINATION ${KDE_INSTALL_DATAROOTDIR}/kconf_update)

install(FILES
		AUTHORS
		ChangeLog
		README
		README.cwl
		kile-remote-control.txt
	DESTINATION ${KDE_INSTALL_DATAROOTDIR}/doc/kile)

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
