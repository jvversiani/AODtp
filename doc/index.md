# Documentation

## Description
Programme de justification optimale de texte utilisant la programmation dynamique pour minimiser le coût de découpage des paragraphes selon une norme cubique.

## Utilisation

```bash
./bin/AODjustify M fichier.in
```

### Paramètres
- `M` : Nombre de caractères maximum par ligne (entier positif)
- `fichier.in` : Fichier texte d'entrée au format ISO-8859-1

### Exemple
```bash
./bin/AODjustify 80 monTexte.in
```
Génère automatiquement `monTexte.out` avec des lignes de 80 caractères maximum.

## Technique de Mémoïsation

Le programme utilise la programmation dynamique avec mémoïsation pour résoudre l'équation de Bellman :

```
φ(i) = {
    0                                           si E(i,n) ≥ 0  (dernière ligne)
    min{φ(k) + N(E(i,k)) | i<k≤n, E(i,k)≥0}   sinon
}
```

Où :
- `φ(i)` : coût minimal du paragraphe commençant au mot i
- `E(i,k)` : espace restant sur une ligne contenant les mots i à k-1
- `N(x) = x³` : fonction de pénalité cubique

L'algorithme calcule les coûts de manière itérative en partant de la fin du paragraphe, stockant les résultats dans des tableaux pour éviter les recalculs.

## Structures de Données

### Structure `Mot`
```c
typedef struct {
    const char *debut;  // Pointeur vers le début du mot
    long long taille;   // Taille du mot en caractères
}
```
Représente un mot avec un pointeur direct vers le texte mappé en mémoire.

### Structure `Memoisation`
```c
typedef struct {
    long long *cout;      // Tableau des coûts minimaux φ(i)
    long long *prochaine; // Indices de fin de ligne optimale
}
```
Stocke les résultats de la programmation dynamique pour chaque position.

## Algorithme Principal

1. Parsing : Extraction des mots d'un paragraphe
2. Optimisation : Calcul du découpage optimal par programmation dynamique
3. Justification : Répartition uniforme des espaces sur chaque ligne
4. Écriture : Génération du fichier de sortie justifié

## Sortie

Le programme génère :
- Un fichier `.out` avec le texte justifié dans le meme repertoire du fichier d'entrée.
- La norme totale (somme des coûts cubiques) sur `stderr`

### Format de sortie
```
AODjustify CORRECT> [valeur_norme]     # Si succès
AODjustify ERROR> [message]            # Si erreur
```

## Compilation

```bash
make                    # Compile le programme
make clean             # Nettoie les fichiers générés
make test              # Lance les tests avec valgrind et cachegrind
make benchmark         # Tableau les performances et défauts de cache
```

- Fichier d'entrée obligatoirement en ISO-8859-1
- Tous les mots doivent avoir une taille ≤ M
- Les paragraphes sont séparés par des lignes vides
