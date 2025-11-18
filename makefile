TARGET = theWall

SRC_DIR = src
INCLUDE_DIR = include
LIB_DIR = lib_raylib
RELEASE_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.c)

CFLAGS = -Wall -std=c99 -I$(INCLUDE_DIR) -Icurl/include
LIBS = -L$(LIB_DIR) -Lcurl/lib -lraylib -lcurl -lopengl32 -lgdi32 -lwinmm
BIN_TARGET = $(RELEASE_DIR)/$(TARGET).exe

.PHONY: all clean run rebuild

$(BIN_TARGET): $(SOURCES)
	@mkdir -p $(RELEASE_DIR)
	gcc $(CFLAGS) $(SOURCES) -o $@ $(LIBS)

# Forçar rebuild
rebuild:
	@rm -f $(BIN_TARGET)
	$(MAKE) $(BIN_TARGET)

run:
	@rm -f $(BIN_TARGET)
	$(MAKE) $(BIN_TARGET)
	./$(BIN_TARGET)

clean:
	rm -rf $(RELEASE_DIR)