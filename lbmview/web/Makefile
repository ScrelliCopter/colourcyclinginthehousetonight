EMSDK   ?= $${HOME}/repos/sdk/emsdk
DESTDIR ?= out

TARGET   := colourcyclinginthehousetonight
SOURCE   := ../
BUILDDIR := build

CMFLAGS := -GNinja -DCMAKE_BUILD_TYPE=Release -DUSE_VORBISFILE:BOOL=OFF


.PHONY: all
all: $(DESTDIR)/$(TARGET).wasm $(DESTDIR)/$(TARGET).data $(DESTDIR)/$(TARGET).js $(DESTDIR)/index.html

CMAKE := cmake
MKDIR := mkdir
CP    := cp
RM    := rm

$(BUILDDIR)/$(TARGET).wasm $(BUILDDIR)/$(TARGET).js | $(SOURCE)/CMakeLists.txt:
	@. "$(EMSDK)/emsdk_env.sh" && emcmake $(CMAKE) "$(SOURCE)" -B "$(BUILDDIR)" $(CMFLAGS)
	@$(CMAKE) --build "$(BUILDDIR)"

$(DESTDIR):
	@$(MKDIR) -p $@

$(DESTDIR)/$(TARGET).wasm: $(BUILDDIR)/$(TARGET).wasm | $(DESTDIR)
	@$(CP) $< $@

$(DESTDIR)/$(TARGET).data: $(BUILDDIR)/$(TARGET).data | $(DESTDIR)
	@$(CP) $< $@

$(DESTDIR)/$(TARGET).js: $(BUILDDIR)/$(TARGET).js | $(DESTDIR)
	@./formatjs.sh "$<" "$@"

$(DESTDIR)/index.html: index.html | $(DESTDIR)
	@$(CP) $< $@

.PHONY: clean
clean:
	@$(RM) -rf $(DESTDIR) $(BUILDDIR)
