cmake_minimum_required(VERSION 3.10)
project(AlephOne)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__vita__ -Wno-error")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__vita__")
add_executable(alephone
    Source_Files/vita_init.c
    Source_Files/shell.o
    Source_Files/shell_misc.o
    Source_Files/shell_options.o
    Source_Files/main.o
)
target_link_libraries(alephone
    -Wl,--allow-multiple-definition
    -Wl,--start-group
    ${CMAKE_SOURCE_DIR}/Source_Files/CSeries/libcseries.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Files/libfiles.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Videos/libvideos.a
    ${CMAKE_SOURCE_DIR}/Source_Files/GameWorld/libgameworld.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Input/libinput.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Lua/liba1lua.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Misc/libmisc.a
    ${CMAKE_SOURCE_DIR}/Source_Files/ModelView/libmodelview.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Network/libnetwork.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Network/Metaserver/libmetaserver.a
    ${CMAKE_SOURCE_DIR}/Source_Files/RenderMain/librendermain.a
    ${CMAKE_SOURCE_DIR}/Source_Files/RenderOther/librenderother.a
    ${CMAKE_SOURCE_DIR}/Source_Files/Sound/libsound.a
    ${CMAKE_SOURCE_DIR}/Source_Files/TCPMess/libtcpmess.a
    ${CMAKE_SOURCE_DIR}/Source_Files/XML/libxml.a
    SDL2_image SDL2 SceGxm_stub SceDisplay_stub SceCtrl_stub SceAppMgr_stub
    SceAppUtil_stub SceAudio_stub SceAudioIn_stub SceSysmodule_stub
    SceIofilemgr_stub SceCommonDialog_stub SceTouch_stub SceHid_stub
    SceMotion_stub ScePower_stub SceProcessmgr_stub webpdemux webp sharpyuv
    openal SceAudio_stub sndfile mp3lame FLAC opus mpg123 SDL2_ttf freetype
    bz2 png16 vorbisenc vorbis ogg z pthread m boost_filesystem boost_system
    stdc++
    -Wl,--end-group
)
vita_create_self(eboot.bin alephone UNSAFE)
vita_create_vpk(alephone-vita.vpk ALPH00001 eboot.bin
  VERSION "01.00"
  NAME "Aleph One"
  FILE
    ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
    ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg0.png sce_sys/livearea/contents/bg0.png
    ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
