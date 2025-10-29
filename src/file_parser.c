#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// Variables globales
long long M; // Nombre de caractères par ligne
long long valeur_norme_totale = 0; // Norme 3 totale du fichier

typedef struct {
    const char *debut;  // Pointeur vers le début du mot dans le fichier mappé
    long long taille;   // Taille du mot
} Mot;

typedef struct {
    long long *cout;        // Tableau des coûts minimaux
    long long *prochaine;   // Tableau des indices de fin de ligne optimale
} Memoisation;

// Vérifie l'encodage ISO-8859 du fichier
struct stat verifier_encodage_fichier(const char *nom_fichier) {
    struct stat info_fichier;

    if (stat(nom_fichier, &info_fichier) != 0) {
        fprintf(stderr, "AODjustify ERROR> Le fichier n'a pas été trouvé.\n");
        exit(1);
    }

    char commande[1024];
    snprintf(commande, sizeof(commande), "file -b '%s' | grep -q 'ISO-8859'", nom_fichier);

    if (system(commande) != 0) {
        fprintf(stderr, "AODjustify ERROR> fichier en entrée pas au format ISO-8859-1\n");
        exit(1);
    }

    return info_fichier;
}

// Ouvre et mappe le fichier en mémoire
char* ouvrir_fichier(const struct stat info_fichier, int fd) {
    char *carte = mmap(NULL, info_fichier.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (carte == MAP_FAILED) {
        fprintf(stderr, "AODjustify ERROR> Ouverture fichier impossible\n");
        close(fd);
        exit(1);
    }
    return carte;
}

// Calcule le delta (espace restant) pour une ligne de i à k (exclusif)
long long calculer_delta(Mot *mots[], long long i, long long k) {
    if (i >= k) return M;

    long long somme = 0;
    for (long long idx = i; idx < k; idx++) {
        somme += mots[idx]->taille;
        if (idx < k - 1) {
            somme++; // Espace entre les mots
        }
    }
    return M - somme;
}

// Calcule la norme cubique
long long norme_cubique(long long e) {
    return e * e * e;
}

// Programmation dynamique itérative pour trouver le découpage optimal
void decoupage_optimal(Mot *mots[], long long n, Memoisation *memo) {
    // Initialisation
    for (long long i = 0; i <= n; i++) {
        memo->cout[i] = LLONG_MAX;
        memo->prochaine[i] = -1;
    }

    memo->cout[n] = 0;

    // Calcul bottom-up
    for (long long i = n - 1; i >= 0; i--) {
        // Vérifier si on peut mettre tous les mots restants sur une ligne
        long long delta_fin = calculer_delta(mots, i, n);
        if (delta_fin >= 0) {
            memo->cout[i] = 0;
            memo->prochaine[i] = n;
            continue;
        }

        // Essayer toutes les coupures possibles
        for (long long k = i + 1; k <= n; k++) {
            long long delta = calculer_delta(mots, i, k);
            if (delta < 0) break; // Le mot ne rentre plus

            long long cout_ligne = norme_cubique(delta);
            if (memo->cout[k] != LLONG_MAX) {
                long long cout_total = cout_ligne + memo->cout[k];
                if (cout_total < memo->cout[i]) {
                    memo->cout[i] = cout_total;
                    memo->prochaine[i] = k;
                }
            }
        }
    }
}

// Écrit une ligne justifiée dans le fichier de sortie
void ecrire_ligne_justifiee(FILE *sortie, Mot *mots[], long long i, long long k, int derniere_ligne) {
    if (i >= k) return;

    if (derniere_ligne || k - i == 1) {
        // Dernière ligne ou ligne avec un seul mot : pas de justification
        for (long long idx = i; idx < k; idx++) {
            fwrite(mots[idx]->debut, 1, mots[idx]->taille, sortie);
            if (idx < k - 1) {
                fputc(' ', sortie);
            }
        }
    } else {
        // Ligne justifiée
        long long espaces_totaux = M;
        for (long long idx = i; idx < k; idx++) {
            espaces_totaux -= mots[idx]->taille;
        }

        long long nb_intervalles = k - i - 1;
        long long espaces_base = espaces_totaux / nb_intervalles;
        long long espaces_supplementaires = espaces_totaux % nb_intervalles;

        for (long long idx = i; idx < k; idx++) {
            fwrite(mots[idx]->debut, 1, mots[idx]->taille, sortie);
            if (idx < k - 1) {
                long long nb_espaces = espaces_base;
                if (espaces_supplementaires > 0) {
                    nb_espaces++;
                    espaces_supplementaires--;
                }
                for (long long j = 0; j < nb_espaces; j++) {
                    fputc(' ', sortie);
                }
            }
        }
    }
    fputc('\n', sortie);
}

// Justifie un paragraphe et écrit le résultat
void justifier_paragraphe(FILE *sortie, Mot *mots[], long long n) {
    if (n == 0) return;

    // Vérifier que tous les mots rentrent sur une ligne
    for (long long i = 0; i < n; i++) {
        if (mots[i]->taille > M) {
            fprintf(stderr, "AODjustify ERROR> le fichier possède un mot de longueur supérieure à %lld caractères : justification sur %lld caractères impossible\n", M, M);
            exit(1);
        }
    }

    // Allocation mémoire pour la mémoïsation
    if (n < 0) {
        fprintf(stderr, "AODjustify ERROR: n est negatif (%lld)\n", n);
        exit(EXIT_FAILURE);
    }

    Memoisation memo;
    memo.cout = calloc(n + 1, sizeof(long long));
    memo.prochaine = calloc(n + 1, sizeof(long long));

    // Calcul du découpage optimal
    decoupage_optimal(mots, n, &memo);

    // Ajout de la norme au total
    valeur_norme_totale += memo.cout[0];

    // Écriture des lignes
    long long i = 0;
    while (i < n) {
        long long k = memo.prochaine[i];
        int derniere_ligne = (k == n);
        ecrire_ligne_justifiee(sortie, mots, i, k, derniere_ligne);
        i = k;
    }

    // Libération mémoire
    free(memo.cout);
    free(memo.prochaine);
}

// Parse et traite le fichier entier
void traiter_fichier(const char *fichier, size_t taille_fichier, FILE *sortie) {
    Mot *mots[100000]; // Tableau pour stocker les mots d'un paragraphe
    long long nb_mots = 0;
    size_t position = 0;

    while (position < taille_fichier) {
        nb_mots = 0;

        // Parser un paragraphe
        while (position < taille_fichier) {
            // Ignorer les espaces en début
            while (position < taille_fichier && isspace(fichier[position]) && fichier[position] != '\n') {
                position++;
            }

            // Vérifier fin de paragraphe (ligne vide)
            if (position < taille_fichier && fichier[position] == '\n') {
                position++;
                // Vérifier si c'est une ligne vide (nouveau paragraphe)
                size_t pos_temp = position;
                while (pos_temp < taille_fichier && (fichier[pos_temp] == ' ' || fichier[pos_temp] == '\t')) {
                    pos_temp++;
                }
                if (pos_temp < taille_fichier && fichier[pos_temp] == '\n') {
                    // Ligne vide trouvée, fin du paragraphe
                    position = pos_temp + 1;
                    break;
                }
            }

            // Trouver le début d'un mot
            if (position < taille_fichier && !isspace(fichier[position])) {
                const char *debut_mot = &fichier[position];
                long long taille_mot = 0;

                // Calculer la taille du mot
                while (position < taille_fichier && !isspace(fichier[position])) {
                    taille_mot++;
                    position++;
                }

                // Créer et stocker le mot
                mots[nb_mots] = (Mot*)malloc(sizeof(Mot));
                mots[nb_mots]->debut = debut_mot;
                mots[nb_mots]->taille = taille_mot;
                nb_mots++;
            }
        }

        // Justifier et écrire le paragraphe si non vide
        if (nb_mots > 0) {
            justifier_paragraphe(sortie, mots, nb_mots);

            // Ajouter une ligne vide après le paragraphe (sauf à la fin)
            if (position < taille_fichier) {
                fputc('\n', sortie);
            }

            // Libérer la mémoire des mots
            for (long long i = 0; i < nb_mots; i++) {
                free(mots[i]);
            }
        }
    }
}

// Génère le nom du fichier de sortie
char* generer_nom_sortie(const char *nom_entree) {
    // Retirer l'extension .in si présente
    size_t longueur = strlen(nom_entree);
    char *nom_sortie = (char*)malloc(longueur + 5); // +5 pour ".out" et '\0'

    if (longueur > 3 && strcmp(&nom_entree[longueur - 3], ".in") == 0) {
        strncpy(nom_sortie, nom_entree, longueur - 3);
        nom_sortie[longueur - 3] = '\0';
        strcat(nom_sortie, ".out");
    } else {
        strcpy(nom_sortie, nom_entree);
        strcat(nom_sortie, ".out");
    }

    return nom_sortie;
}

int main(int argc, char *argv[]) {
    // Vérification des arguments
    if (argc != 3) {
        fprintf(stderr, "AODjustify ERROR> Utilisation: ./bin/AODjustify <M> <filename>\n");
        exit(1);
    }

    // Parser M
    M = strtoll(argv[1], NULL, 10);
    if (M <= 0) {
        fprintf(stderr, "AODjustify ERROR> Utilisation: M doit être positif\n");
        exit(1);
    }

    // Nom du fichier d'entrée
    const char *nom_entree = argv[2];

    // Vérifier l'encodage
    struct stat info_fichier = verifier_encodage_fichier(nom_entree);

    // Ouvrir le fichier d'entrée
    int fd = open(nom_entree, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "AODjustify ERROR> Ouverture fichier impossible\n");
        exit(1);
    }

    // Mapper le fichier en mémoire
    char *fichier = ouvrir_fichier(info_fichier, fd);

    // Générer le nom du fichier de sortie
    char *nom_sortie = generer_nom_sortie(nom_entree);

    // Ouvrir le fichier de sortie
    FILE *sortie = fopen(nom_sortie, "w");
    if (!sortie) {
        fprintf(stderr, "AODjustify ERROR> Création du fichier de sortie impossible\n");
        munmap(fichier, info_fichier.st_size);
        close(fd);
        free(nom_sortie);
        exit(1);
    }

    // Traiter le fichier
    traiter_fichier(fichier, info_fichier.st_size, sortie);

    // Fermer le fichier de sortie
    fclose(sortie);

    // Afficher la norme totale
    fprintf(stderr, "AODjustify CORRECT> %lld\n", valeur_norme_totale);

    // Nettoyage
    munmap(fichier, info_fichier.st_size);
    close(fd);
    free(nom_sortie);

    return 0;
}