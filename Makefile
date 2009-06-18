COMPFLAGS=-O4
#COMPFLAGS=-g


all: exposit

vimtest: exposit
	# ./exposit -noise=noise_10sec1.jpg ../../m101/*07.jpg ../../m101/*.jpg

	#	./exposit -falloff=falloff_180mmf2.8.png ../../m101/*04.jpg ../../m101/cap*.jpg
	#	./exposit -falloff=falloff_180mmf2.8.png ../../m101_a/*07.jpg ../../m101_a/cap*.jpg
	#### ./exposit -falloff=falloff_180mmf2.8.png -watch=./capt."*\."jpg

	# test de rotation
	####./exposit -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png /home/jd/series/t1/20s/capt0002.jpg /home/jd/series/t1/u/capt0011.jpg

	# test de bluriness
	./exposit -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png /home/jd/series/t1/20s/capt0002.jpg \
	/home/jd/series/t1/u???*/capt0010.jpg \
	/home/jd/series/t1/u???*/capt0014.jpg \
	/home/jd/series/t1/u???*/capt0025.jpg \

	###########./exposit -falloff=/home/jd/homecacao/simplestacker/falloff_180mmf2.8.png /home/jd/m101_a/*07.jpg /home/jd/m101_a/*.jpg
	#	./exposit -doublescale -falloff=falloff_180mmf2.8.png ../../m101_a/*07.jpg ../../m101_a/cap*.jpg

	# ./exposit -noise=noise_10sec1.jpg -falloff=falloff_180mmf2.8.png ../../m101/*07.jpg ../../m101/cap*.jpg
	# ./exposit ../../m101/*07.jpg `ls ../../m101/*.jpg | head -10`
	# ./exposit ../../m101/*07.jpg ../../m101/*67.jpg

exposit: simplechrono.o draw.o exposit.o jeuchar.o graphutils.o vstar.o starsmap.o gp_imagergbl.o 
	g++ -Wall ${COMPFLAGS} -o exposit exposit.o gp_imagergbl.o starsmap.o vstar.o simplechrono.o draw.o jeuchar.o graphutils.o `sdl-config --cflags --libs` -lSDL_image

exposit.o: exposit.cpp simplechrono.h
	g++ -Wall ${COMPFLAGS} -c exposit.cpp `sdl-config --cflags`

gp_imagergbl.o: gp_imagergbl.cpp gp_imagergbl.h
	g++ -Wall ${COMPFLAGS} -c gp_imagergbl.cpp `sdl-config --cflags`

starsmap.o: starsmap.cpp starsmap.h
	g++ -Wall ${COMPFLAGS} -c starsmap.cpp

simplechrono.o: simplechrono.cpp simplechrono.h
	g++ -Wall ${COMPFLAGS} -c simplechrono.cpp

vstar.o: vstar.cpp vstar.h
	g++ -Wall ${COMPFLAGS} -c vstar.cpp

draw.o: draw.c draw.h
	gcc -Wall ${COMPFLAGS} -c draw.c

jeuchar.o: jeuchar.c jeuchar.h
	gcc -Wall ${COMPFLAGS} -c jeuchar.c `sdl-config --cflags`

graphutils.o: graphutils.c graphutils.h
	gcc -Wall ${COMPFLAGS} -c graphutils.c `sdl-config --cflags`

clean:
	rm -f graphutils.o jeuchar.o draw.o exposit.o starsmap.o gp_imagergbl.o vstar.o exposit

distclean: clean
