TOP=	..
include ${TOP}/Makefile.config

LIB=		ag_rg
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"54223b00-f4c5-4b49-ab84-cd2576c7f8d6"
LIB_LINKS=	ag_core_static ag_gui_static ag_vg_static pthreads \
		SDL opengl freetype

SRCS=	tileset.c tileview.c tile.c feature.c fill.c pixmap.c prim.c \
	animation.c texture.c texsel.c transform.c animview.c
MAN3=	RG_Anim.3 RG_Feature.3 RG_Pixmap.3 RG_Tile.3 RG_Tileset.3 RG_Tileview.3

#SRCS+= sketch.c sketch_line.c sketch_circle.c sketch_polygon.c sketchproj.c

CFLAGS+=-D_AGAR_RG_INTERNAL ${SDL_CFLAGS} ${MATH_CFLAGS} ${GETTEXT_CFLAGS}

include .manlinks.mk
include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
