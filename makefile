TARGET = theWall

SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
INCLUDE_DIR = include
LIB_DIR = lib_raylib
FFMPEG_DIR = ffmpeg
RELEASE_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(CORE_DIR)/*.c)

CFLAGS = -Wall -std=c99 -I$(INCLUDE_DIR) -I$(CORE_DIR) -I$(FFMPEG_DIR)/include -Icurl/include
LIBS = -L$(LIB_DIR) -Lcurl/lib -L$(FFMPEG_DIR)/lib -lraylib -lcurl -lavcodec -lavformat -lavutil -lswscale -lswresample -lopengl32 -lgdi32 -lwinmm
BIN_TARGET = $(RELEASE_DIR)/$(TARGET).exe

.PHONY: all clean run rebuild

$(BIN_TARGET): $(SOURCES)
	@mkdir -p $(RELEASE_DIR)
	gcc $(CFLAGS) $(SOURCES) -o $@ $(LIBS)

rebuild:
	@rm -f $(BIN_TARGET)
	$(MAKE) $(BIN_TARGET)

run:
	@rm -f $(BIN_TARGET)
	$(MAKE) $(BIN_TARGET)
	./$(BIN_TARGET)

clean:
	rm -rf $(RELEASE_DIR)