set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(
	jiro
	"main.cpp"
	"log.hpp"
	"stop_source.hpp"
	"Archive.cpp"
	"Archive.hpp"
	"ui/Actions.hpp"
	"ui/MainWindow.cpp"
	"ui/MainWindow.hpp"
	"ui/ImageView.cpp"
	"ui/ImageView.hpp"
	"ui/ProgressWidget.cpp"
	"ui/ProgressWidget.hpp"
	"ziputil/ArchiveFormats.hpp"
	"ziputil/com_ptr.hpp"
	"ziputil/InStream.cpp"
	"ziputil/InStream.hpp"
	"ziputil/SequentialOutStream.cpp"
	"ziputil/SequentialOutStream.hpp"
	"ziputil/ArchiveExtractCallback.cpp"
	"ziputil/ArchiveExtractCallback.hpp"
)

target_link_libraries(jiro PRIVATE project_options project_warnings)

find_package(Qt5Core CONFIG REQUIRED)
target_link_libraries(jiro PRIVATE Qt5::Core Qt5::WinMain)

find_package(Qt5Widgets CONFIG REQUIRED)
target_link_libraries(jiro PRIVATE Qt5::Widgets)

find_package(Qt5Gui CONFIG REQUIRED)
target_link_libraries(jiro PRIVATE Qt5::Gui)

find_package(7zip CONFIG REQUIRED)
target_link_libraries(jiro PRIVATE 7zip::7zip)
# make sure IID_* variables are declared
target_compile_definitions(jiro PRIVATE INITGUID)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(jiro PRIVATE fmt::fmt-header-only)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(jiro PRIVATE spdlog::spdlog_header_only)
target_compile_definitions(jiro PRIVATE SPDLOG_ACTIVE_LEVEL=1)
