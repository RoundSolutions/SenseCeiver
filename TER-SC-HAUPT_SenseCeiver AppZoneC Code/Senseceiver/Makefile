CPPFLAGS=
LDFLAGS=
CFLAGS=
OBJS=
include prod_tool.in
-include Makefile.in
include export.mk

#build using packaged libs
ifeq ($(PRODUCT), 2G)
  PRODUCT = TELIT_2G_I
endif

ifeq ($(PRODUCT), 3G)
  PRODUCT = TELIT_3G_I
endif

ifeq ($(PRODUCT), 4G)
  PRODUCT = TELIT_4G_I
endif

space :=
space +=
makefilePath := $(APPZONE_MAKEFILE_COMMON)/az_makefile.mk
makefilePath := $(subst $(space),\$(space),$(makefilePath))
include $(makefilePath)