## Process this file with automake to produce Makefile.in

INCLUDES = \
    -DDATADIR=\""$(datadir)"\"  \
	-DPACKAGE_DATA_DIR=\""$(datadir)/gweled"\"  \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	-DSCOREDIR=\""$(scoredir)"\"

AM_CFLAGS = 		  \
    @GTK_CFLAGS@      \
	@RSVG_CFLAGS@	  \
	@MIKMOD_CFLAGS@   \
	@PTHREAD_CFLAGS@

bin_PROGRAMS = gweled

gweled_SOURCES = \
	main.c main.h \
	callbacks.c \
	board_engine.c board_engine.h \
	graphic_engine.c graphic_engine.h \
	sge_core.c sge_core.h \
	sge_utils.c sge_utils.h \
	sound.c sound.h \
	games-score.c games-score.h \
	games-scores.c games-scores.h \
	games-scores-backend.c games-scores-backend.h \
	games-scores-dialog.c games-scores-dialog.h \
	games-setgid-io.c games-setgid-io.h \
	games-scores-dialog-private.h

gweled_LDADD = @GTK_LIBS@ @RSVG_LIBS@ @MIKMOD_LIBS@ @PTHREAD_LIBS@


