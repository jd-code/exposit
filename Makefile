
# this should match most linuces
COMPFLAGS=-O4 -g

# on macosX maverick O4 prevent debugging (?) and libstdc++ is needed for piping extensions
#COMPFLAGS=-g -stdlib=libstdc++


# for macports ...
ADDINCLUDES=-I/opt/local/include

CFLAGS=${COMPFLAGS} ${ADDINCLUDES}


all: exposit

oldvimtest: exposit show testchunkio
	# ./exposit -noise=noise_10sec1.jpg ../../m101/*07.jpg ../../m101/*.jpg

	#	./exposit -falloff=falloff_180mmf2.8.png ../../m101/*04.jpg ../../m101/cap*.jpg
	#	./exposit -falloff=falloff_180mmf2.8.png ../../m101_a/*07.jpg ../../m101_a/cap*.jpg
	#### ./exposit -falloff=falloff_180mmf2.8.png -watch=./capt."*\."jpg

	# test avec m51 / decalage en patterns ! explosion sur l'ouverture du premier fichier
#	./exposit -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png \
#	/home/jd/series/e/a/ttt/capt0001.jpg /home/jd/series/e/a/ttt/*

	# test de rotation
	# ./exposit -finetune -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png /home/jd/series/t1/20s/capt0002.jpg /home/jd/series/t1/u/capt0011.jpg

	# test de bluriness
	# ./exposit -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png /home/jd/series/t1/20s/capt0002.jpg \
	# /home/jd/series/t1/u???*/capt0010.jpg \
	# /home/jd/series/t1/u???*/capt0014.jpg \
	# /home/jd/series/t1/u???*/capt0025.jpg \

	###########./exposit -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png /home/jd/m101_a/*07.jpg /home/jd/m101_a/*.jpg
	#	./exposit -doublescale -falloff=falloff_180mmf2.8.png ../../m101_a/*07.jpg ../../m101_a/cap*.jpg

	# ./exposit -noise=noise_10sec1.jpg -falloff=falloff_180mmf2.8.png ../../m101/*07.jpg ../../m101/cap*.jpg
	# ./exposit ../../m101/*07.jpg `ls ../../m101/*.jpg | head -10`
	# ./exposit ../../m101/*07.jpg ../../m101/*67.jpg


	# ./procm31.sh
	# ./mytest.sh
	#./exposit -crop=10,10,20,20

	###########################333
	#######./show

	./testchunkio

vimtest: exposit testchunkio
	# ./testchunkio
	export SDL_VIDEODRIVER=X11 ; ./exposit -readxpo=m42+flame.xpo


testchunkio: testchunkio.o chunkio.o
	g++ -Wall ${CFLAGS} -o testchunkio testchunkio.o chunkio.o

exposit: simplechrono.o chunkio.o draw.o exposit.o jeuchar.o graphutils.o vstar.o starsmap.o gp_imagergbl.o interact.o 
	g++ -Wall ${CFLAGS} -o exposit exposit.o chunkio.o gp_imagergbl.o starsmap.o vstar.o simplechrono.o draw.o jeuchar.o graphutils.o interact.o `sdl-config --cflags --libs` -lSDL_image -lpng

interact.o: interact.cpp simplechrono.h
	g++ -Wall ${CFLAGS} -c interact.cpp `sdl-config --cflags`

exposit.o: exposit.cpp simplechrono.h
	g++ -Wall ${CFLAGS} -c exposit.cpp `sdl-config --cflags`

gp_imagergbl.o: gp_imagergbl.cpp gp_imagergbl.h
	g++ -Wall ${CFLAGS} -c gp_imagergbl.cpp `sdl-config --cflags`

chunkio.o: chunkio.cpp
	g++ -Wall ${CFLAGS} -c chunkio.cpp

testchunkio.o: testchunkio.cpp
	g++ -Wall ${CFLAGS} -c testchunkio.cpp

starsmap.o: starsmap.cpp starsmap.h
	g++ -Wall ${CFLAGS} -c starsmap.cpp

simplechrono.o: simplechrono.cpp simplechrono.h
	g++ -Wall ${CFLAGS} -c simplechrono.cpp

vstar.o: vstar.cpp vstar.h
	g++ -Wall ${CFLAGS} -c vstar.cpp

draw.o: draw.c draw.h
	gcc -Wall ${CFLAGS} -c draw.c `sdl-config --cflags`

jeuchar.o: jeuchar.c jeuchar.h
	gcc -Wall ${CFLAGS} -c jeuchar.c `sdl-config --cflags`

graphutils.o: graphutils.c graphutils.h
	gcc -Wall ${CFLAGS} -c graphutils.c `sdl-config --cflags`

clean:
	rm -f exposit *.o show testchunkio types.vim tags 

distclean: clean


####################################################################################

show: show.o chunkio.o draw.o jeuchar.o graphutils.o vstar.o starsmap.o gp_imagergbl.o simplechrono.o draw.o
	g++ -Wall ${CFLAGS} -o show chunkio.o show.o gp_imagergbl.o starsmap.o vstar.o simplechrono.o draw.o jeuchar.o graphutils.o `sdl-config --cflags --libs` -lSDL_image -lpng

show.o: show.cpp show.h
	g++ -Wall ${CFLAGS} -c `sdl-config --cflags` show.cpp

