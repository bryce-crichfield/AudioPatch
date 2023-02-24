gui:
	clang++ -o exe main.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17 -O3

build:
	clang++ -o audio main.cpp -lasound -lpthread -ljack -lsndfile -lportaudio -lstdc++fs -std=c++17 -O3