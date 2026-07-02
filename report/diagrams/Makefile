MERMAID_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
FIGURES_DIR := $(MERMAID_DIR)/../figures
MMDC := mmdc
MMDC_FLAGS := -b white -w 1200 --quiet

MMD_FILES := $(wildcard $(MERMAID_DIR)/*.mmd)
PNG_FILES := $(patsubst $(MERMAID_DIR)/%.mmd,$(FIGURES_DIR)/%.png,$(MMD_FILES))

.PHONY: all clean

all: $(PNG_FILES)

$(FIGURES_DIR)/%.png: $(MERMAID_DIR)/%.mmd
	@mkdir -p $(FIGURES_DIR)
	$(MMDC) -i $< -o $@ $(MMDC_FLAGS)

clean:
	rm -f $(FIGURES_DIR)/*.png
