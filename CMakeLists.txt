cmake_minimum_required(VERSION 3.20)

project(DynamicWorkflowScheduler LANGUAGES CXX)

add_library(workflow_scheduler
    Order.cpp
    QueueManager.cpp
    Scheduler.cpp
    TerminalDisplay.cpp
    CommandProcessor.cpp
)

target_include_directories(
    workflow_scheduler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(
    workflow_scheduler
    PUBLIC
        cxx_std_17
)

if(MSVC)
    target_compile_options(
        workflow_scheduler
        PRIVATE
            /W4
            /permissive-
    )
else()
    target_compile_options(
        workflow_scheduler
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

add_executable(
    scheduler
    main.cpp
)

target_link_libraries(
    scheduler
    PRIVATE
        workflow_scheduler
)