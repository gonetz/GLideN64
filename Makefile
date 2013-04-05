DEFINES	=	-D__LINUX__ -DX86_ASM
CXX	=	g++
CXXFLAGS =	-I. $(DEFINES) `sdl-config --cflags` `gtk-config --cflags gtk gthread` \
		`glib-config --cflags` -fPIC -finline-functions -funroll-loops \
		-ffast-math -mcpu=`uname -m` -O2
#CXXFLAGS =	-I. $(DEFINES) `sdl-config --cflags` `gtk-config --cflags gtk gthread` \
#		 `glib-config --cflags` -g3 #-DDEBUG
LD	=	g++
LDFLAGS	=	-shared -fPIC
LIBS	=	`sdl-config --libs` `gtk-config --libs gtk gthread` `glib-config --libs` -lGL -lGLU
TARGET	=	glN64-0.4.1.so
OBJECTS	=	2xSAI.o \
		CRC.o \
		Combiner.o \
		Config_linux.o \
		Debug_linux.o \
		DepthBuffer.o \
		F3D.o \
		F3DEX.o \
		F3DEX2.o \
		F3DDKR.o \
		F3DPD.o \
		F3DWRUS.o \
		FrameBuffer.o \
		glN64.o \
		GBI.o \
		gDP.o \
		gSP.o \
		L3D.o \
		L3DEX.o \
		L3DEX2.o \
		N64.o \
		NV_register_combiners.o \
		OpenGL.o \
		RDP.o \
		RSP.o \
		S2DEX.o \
		S2DEX2.o \
		texture_env.o \
		texture_env_combine.o \
		Textures.o \
		VI.o

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $(LIBS) -o $@ $(OBJECTS)

clean:
	rm -f $(TARGET) $(OBJECTS)
