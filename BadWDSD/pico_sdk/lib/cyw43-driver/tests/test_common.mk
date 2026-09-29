CYW43_TOP ?= ../..

RM ?= rm
MKDIR ?= mkdir

BUILD ?= build

CC ?= cc

CFLAGS += -I.
CFLAGS += -I$(CYW43_TOP)
CFLAGS += -std=c99
CFLAGS += -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Wno-unused-local-typedefs
CFLAGS += -m32

SRC_BT += \
	src/cyw43_bthci_uart.c \

SRC_WL += \
	src/cyw43_ctrl.c \
	src/cyw43_ll.c \
	src/cyw43_lwip.c \
	src/cyw43_sdio.c \
	src/cyw43_stats.c \

OBJ += $(addprefix $(BUILD)/,$(SRC:.c=.o))

.PHONY: all
all: $(BUILD)/test

.PHONY: clean
clean:
	$(RM) -rf $(BUILD)

.PHONY: test
test: $(BUILD)/test
	./$(BUILD)/test | diff - test.exp

$(BUILD)/test: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

vpath %.c . $(CYW43_TOP)
$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

OBJ_DIRS = $(sort $(dir $(OBJ)))
$(OBJ): | $(OBJ_DIRS)
$(OBJ_DIRS):
	$(MKDIR) -p $@

# Dependency generation
%.o: %.d
CFLAGS += -MP -MMD
DEP := $(OBJ:.o=.d)
$(DEP):

-include $(DEP)
