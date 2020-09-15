set(SOURCES
    main.cpp
)

serenity_bin(LibGLDemo)
target_link_libraries(LibGLDemo LibGL LibGUI LibIPC LibCore)
