macro(MANPAGE) {
    FIND_PROGRAM(GZIP gzip)
    set(MAN_NAMES disman.1)
    set(MAN_FILES)

    foreach(m IN LISTS MAN_NAMES)
    set(mf ${CMAKE_BINARY_DIR}/${m})
    set(ms ${CMAKE_SOURCE_DIR}/${m}.txt)
    add_custom_command(OUTPUT ${mf}
        DEPENDS ${ms}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${GZIP} -c ${ms} > $.gz
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${ms}
        COMMENT "Building manpage ${mf}"
        VERBATIM)
    list(APPEND MAN_FILES ${mf})
    endforeach()
    
    INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_MAKE_PROGRAM} man)")
    
    # Detects if PKGMAN has been specified
    SET(DESTINATION_MAN_PAGE share/man/man1)
    IF(MANDIR)
        SET(DESTINATION_MAN_PAGE ${MANDIR}/man1)
    ENDIF(MANDIR)

    # Install man page.
    INSTALL(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.1.gz
        DESTINATION ${DESTINATION_MAN_PAGE}
    )
}
