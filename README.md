# compilazione
per compilare il solutore è necessario cmake. Ho utilizzato gcc come compilatore
di riferimento.

vanno lanciati i seguenti comandi:

mkdir build
cd build
cmake ..
make -j

# utilizzo

ora il solutore è compilato, e nella cartella build dovrebbero esserci due
eseguibili:
- ./pigeon_hole che serve a generare istanze del problema della tana e del piccione
- ./solver che implementa il solutore sat

Si possono ottenere informazioni sull'utilizzo lanciando gli eseguibili con
l'opzione -h o --help. Per una prova rapida di funzionamento si possono anche
collegare tra loro i due seguibili per esempio con

./pigeon_hole 2 | ./solver

che crea il problema con 2 tane e 3 piccioni, e lo fa risolvere al solutore.
