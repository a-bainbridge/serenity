set(SOURCES
    Init.cpp
)

serenity_lib(LibGL gl)
target_link_libraries(LibGL LibGUI LibM LibCore)
