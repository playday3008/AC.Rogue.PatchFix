include(FetchContent)

FetchContent_Declare(safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG        755a4a08aa7d0d7143331dc854235ebc2d1570f9 # main
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(safetyhook)

FetchContent_Declare(injector
    #GIT_REPOSITORY https://github.com/ThirteenAG/injector.git
    GIT_REPOSITORY https://github.com/playday3008/injector.git
    GIT_TAG        b7acc953cd5e957f329983a3a6bf08a3c68c0644
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(injector)

add_library(injector INTERFACE)
set_property(TARGET injector PROPERTY CXX_STANDARD 20)
target_include_directories(injector SYSTEM INTERFACE
    "${injector_SOURCE_DIR}/include"
)
target_compile_definitions(injector INTERFACE INJECTOR_GVM_DUMMY INJECTOR_GVM_OWN_DETECT)
target_link_libraries(injector INTERFACE safetyhook)
