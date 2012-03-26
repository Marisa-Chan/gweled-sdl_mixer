## Process this file with automake to produce Makefile.in

SUBDIRS = src data po

pixmapdir   = $(datadir)/pixmaps/$(PACKAGE)
pixmap_DATA = pixmaps/cursor.svg pixmaps/font_24_20.png pixmaps/font_48_40.png \
              pixmaps/gem01.svg pixmaps/gem02.svg pixmaps/gem03.svg \
              pixmaps/gem04.svg pixmaps/gem05.svg pixmaps/gem06.svg \
              pixmaps/gem07.svg pixmaps/tile_even.svg pixmaps/tile_odd.svg \
              pixmaps/powerglow_32.png pixmaps/powerglow_48.png \
              pixmaps/powerglow_64.png pixmaps/sparkle_32.png \
              pixmaps/sparkle_48.png pixmaps/sparkle_64.png

soundsdir   = $(datadir)/sounds/$(PACKAGE)
sounds_DATA = sounds/autonom.s3m sounds/click.wav sounds/swap.wav

EXTRA_DIST = $(sounds_DATA) \
             $(pixmap_DATA)

SCOREFILES = Normal Timed

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	## Change the names of the high score files, unless the local user has
	## beaten us to it.
	-if [ -f $(DESTDIR)$(scoredir)/gweled.easy.scores -a ! -f $(DESTDIR)$(scoredir)/gweled.Normal.scores ]; then mv -f $(DESTDIR)$(scoredir)/gweled.easy.scores $(DESTDIR)$(scoredir)/gweled.Normal.scores  ; fi
	-if [ -f $(DESTDIR)$(scoredir)/gweled.hard.scores -a ! -f $(DESTDIR)$(scoredir)/gweled.Timed.scores ]; then mv -f $(DESTDIR)$(scoredir)/gweled.hard.scores $(DESTDIR)$(scoredir)/gweled.Timed.scores  ; fi
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gweled.$$i.scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gweled.$$i.scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gweled.$$i.scores; \
	 done

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gweled && chmod 2555 $(DESTDIR)$(bindir)/gweled ;\
	fi

