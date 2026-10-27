rm *.gba
make clean
make -j $(nproc) firered_rev1
cp pokefirered_rev1.gba "C:\Users\Cedric Walter C. Son\Documents\Visual Studio 18\GameboyAdvance\platforms\windows\release\pokefirered_rev1.gba"