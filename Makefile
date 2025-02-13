BUILD_DIR := ./build
SRC_DIR := ./src

SRCS_MYZ := ./src/main.c ./src/myz-funcs.c ./src/myz-utils.c

OBJS_MYZ := ./$(SRCS_MYZ:%=$(BUILD_DIR)/%.o)

DEPS := $(OBJS_MYZ.o=.d)

INC_DIR := $(shell find $(SRCS_MYZ) -type d)
INC_FLAGS := $(addprefix -I, $(INC_DIR))

CPPFLAGS := $(INC_FLAGS) -MMD -MP
LDFLAGS := -Wall -Werror

all: $(BUILD_DIR)/myz

$(BUILD_DIR)/myz: $(OBJS_MYZ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
