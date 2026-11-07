idf_component_register(
    SRCS
        "src/event_bus.cpp"
        "src/step_service.cpp"
        "src/sedentary_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES hb_common
)
