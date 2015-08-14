OBJS = main.c
BRES_OBJS = bresenham3d.c
CLIP_OBJS = texture.c
CC = gcc
WIN_INCLUDE_PATHS = -IC:\minglibs\include\SDL2
WIN_LIB_PATHS = -LC:\minglibs\lib
COMPILER_FLAGS = -w
LINKER_FLAGS = -lSDL2main -lSDL2
WIN_LINKER_FLAGS = -lmingw32 $(LINKER_FLAGS)
TARGET = lester
BRES_TARGET = bresenham
CLIP_TARGET = clip
WIN_TARGET = $(TARGET).exe
WIN_BRES_TARGET = $(BRES_TARGET).exe
WIN_CLIP_TARGET = $(CLIP_TARGET).exe

win : $(OBJS)
	$(CC) $(OBJS) $(WIN_INCLUDE_PATHS) $(WIN_LIB_PATHS) $(COMPILER_FLAGS) $(WIN_LINKER_FLAGS) -o $(WIN_TARGET)
	
bresenhamwin : $(BRES_OBJS)
	$(CC) $(BRES_OBJS) $(WIN_INCLUDE_PATHS) $(WIN_LIB_PATHS) $(COMPILER_FLAGS) $(WIN_LINKER_FLAGS) -o $(WIN_BRES_TARGET)
	
clipwin : $(CLIP_OBJS)
	$(CC) $(CLIP_OBJS) $(WIN_INCLUDE_PATHS) $(WIN_LIB_PATHS) $(COMPILER_FLAGS) $(WIN_LINKER_FLAGS) -o $(WIN_CLIP_TARGET)