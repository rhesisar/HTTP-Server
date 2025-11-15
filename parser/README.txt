Execution sur 10,000 tests
	make tests

Affichage de l'arbre entier
	./http-server <file>

Recherche d'element dans l'arbre
	./http-server <file> <rulename>
	Recherche avec les rulename ABNF, sauf '-' remplace par '_'

Code source dans src/
	- syntax.c/h : Partie syntaxe abnf
	- tree.c/h : Partie creation et recherche dans l'arbre
	- util.c/h : Fonctions utilitaires
	- main.c: Point d'entrée

format.md
	Fichier utilise pour rapidement creer les fonctions de syntaxe,
	laisse pour regarder a titre indicatif, peut comporter des
	erreurs / approximations

allrfc.abnf
	Fichier de syntaxe ABNF

test.sh
	Executable shell, executant le programme sur 10,000 tests

fuzzer/
	10,000 fichiers de tests

arbres/
	Quelques tests, laissés a titre indicatif
