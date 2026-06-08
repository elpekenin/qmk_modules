ifneq ($(findstring indicators, $(COMMUNITY_MODULES)),)
    FIRST_OF_BOTH := $(firstword $(filter indicators ledmap, $(COMMUNITY_MODULES)))
    ifeq ($(FIRST_OF_BOTH),indicators)
        $(error "indicators" is enabled before "ledmap", which will overwrite indicators.)
    endif
endif
