rm *.gba
make clean
make -j $(nproc) firered_rev1
cp pokefirered_rev1.gba pokemonfirered.gba