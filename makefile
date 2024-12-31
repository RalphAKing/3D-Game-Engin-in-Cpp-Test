all: compile link clean

compile:
	g++ -Isrc/include -c main.cpp

link:
	g++ main.o -o main -L/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -lglu32 

clean:
	del main.o