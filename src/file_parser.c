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
    char *carte = mmap(NULL, (size_t)info_fichier.st_size, PROT_READ, MAP_SHARED, fd, 0);
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

long long calculer_delta_rapide(long long *prefix_sum, long long i, long long k) {
    if (i >= k) return M;

    // Somme des tailles de mots de i à k-1
    long long somme_tailles = prefix_sum[k] - prefix_sum[i];

    // Nombre d'espaces entre les mots
    long long nb_espaces = k - i - 1;

    return M - somme_tailles - nb_espaces;
}

// Programmation dynamique itérative pour trouver le découpage optimal
void decoupage_optimal(Mot *mots[], long long n, Memoisation *memo) {
    // Précalculer les sommes préfixées des tailles de mots
    long long *prefix_sum = calloc((size_t)(n + 1), sizeof(long long));
    if (!prefix_sum) {
        fprintf(stderr, "Erreur : allocation mémoire échouée.\n");
        exit(EXIT_FAILURE);
    }

    for (long long i = 0; i < n; i++) {
        prefix_sum[i + 1] = prefix_sum[i] + mots[i]->taille;
    }

    // Initialisation
    for (long long i = 0; i <= n; i++) {
        memo->cout[i] = LLONG_MAX;
        memo->prochaine[i] = -1;
    }

    memo->cout[n] = 0;

    // Calcul bottom-up
    for (long long i = n - 1; i >= 0; i--) {
        // Vérifier si on peut mettre tous les mots restants sur une ligne
        long long delta_fin = calculer_delta_rapide(prefix_sum, i, n);
        if (delta_fin >= 0) {
            memo->cout[i] = 0;
            memo->prochaine[i] = n;
            continue;
        }

        // Essayer toutes les coupures possibles
        for (long long k = i + 1; k <= n; k++) {
            long long delta = calculer_delta_rapide(prefix_sum, i, k);
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

    free(prefix_sum);
}

// Écrit une ligne justifiée dans le fichier de sortie
void ecrire_ligne_justifiee(FILE *sortie, Mot *mots[], long long i, long long k, int derniere_ligne) {
    if (i >= k) return;

    if (derniere_ligne || k - i == 1) {
        // Dernière ligne ou ligne avec un seul mot : pas de justification
        for (long long idx = i; idx < k; idx++) {
            fwrite(mots[idx]->debut, 1, (size_t)mots[idx]->taille, sortie);
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
            fwrite(mots[idx]->debut, 1, (size_t)mots[idx]->taille, sortie);
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
    memo.cout = calloc((size_t)(n + 1), sizeof(long long));
    memo.prochaine = calloc((size_t)(n + 1), sizeof(long long));

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
    Mot **mots = NULL;         // Tableau dynamique de pointeurs vers Mot
    size_t nb_mots = 0;        // Nombre de mots dans le paragraphe
    size_t capacite = 0;       // Capacité actuelle du tableau
    size_t position = 0;       // Position actuelle dans le fichier

    while (position < taille_fichier) {
        nb_mots = 0;

        // Lire un paragraphe
        while (position < taille_fichier) {
            // Ignorer les espaces sauf les sauts de ligne
            while (position < taille_fichier && isspace(fichier[position]) && fichier[position] != '\n') {
                position++;
            }

            // Fin de paragraphe : ligne vide
            if (position < taille_fichier && fichier[position] == '\n') {
                position++;
                size_t pos_temp = position;
                while (pos_temp < taille_fichier && (fichier[pos_temp] == ' ' || fichier[pos_temp] == '\t')) {
                    pos_temp++;
                }
                if (pos_temp < taille_fichier && fichier[pos_temp] == '\n') {
                    position = pos_temp + 1;
                    break;
                }
            }

            // Début d’un mot
            if (position < taille_fichier && !isspace(fichier[position])) {
                const char *debut_mot = &fichier[position];
                size_t taille_mot = 0;

                // Calculer la taille du mot
                while (position < taille_fichier && !isspace(fichier[position])) {
                    taille_mot++;
                    position++;
                }

                // Agrandir le tableau si nécessaire
                if (nb_mots >= capacite) {
                    capacite = (capacite == 0) ? 128 : capacite * 2;
                    mots = realloc(mots, capacite * sizeof(Mot *));
                    if (!mots) {
                        fprintf(stderr, "Erreur : allocation mémoire échouée.\n");
                        exit(EXIT_FAILURE);
                    }
                }

                // Créer et stocker le mot
                mots[nb_mots] = malloc(sizeof(Mot));
                if (!mots[nb_mots]) {
                    fprintf(stderr, "Erreur : allocation mémoire échouée.\n");
                    exit(EXIT_FAILURE);
                }

                mots[nb_mots]->debut = debut_mot;
                mots[nb_mots]->taille = (long long)taille_mot;
                nb_mots++;
            }
        }

        // Justifier et écrire le paragraphe si non vide
        if (nb_mots > 0) {
            justifier_paragraphe(sortie, mots, (long long)nb_mots);

            // Ajouter une ligne vide après le paragraphe
            if (position < taille_fichier) {
                fputc('\n', sortie);
            }

            // Libérer la mémoire des mots
            for (size_t i = 0; i < nb_mots; i++) {
                free(mots[i]);
            }
        }
    }

    // Libérer le tableau de pointeurs
    free(mots);
}


// Génère le nom du fichier d'entrée (ajoute .in)
char* generer_nom_entree(const char *nom_base) {
    size_t longueur = strlen(nom_base);

    // Vérifier si le nom se termine déjà par .in
    if (longueur > 3 && strcmp(&nom_base[longueur - 3], ".in") == 0) {
        // Déjà avec .in, copier tel quel
        char *nom_entree = (char*)malloc(longueur + 1);
        strcpy(nom_entree, nom_base);
        return nom_entree;
    } else {
        // Ajouter .in
        char *nom_entree = (char*)malloc(longueur + 4);  // +3 pour ".in" +1 pour '\0'
        strcpy(nom_entree, nom_base);
        strcat(nom_entree, ".in");
        return nom_entree;
    }
}

// Génère le nom du fichier de sortie
char* generer_nom_sortie(const char *nom_base) {
    size_t longueur = strlen(nom_base);

    // Vérifier si le nom se termine par .in
    if (longueur > 3 && strcmp(&nom_base[longueur - 3], ".in") == 0) {
        char *nom_sortie = (char*)malloc(longueur + 2); // +5 pour ".out" et '\0' et -3 pour longueur - 3
        strncpy(nom_sortie, nom_base, longueur - 3);
        nom_sortie[longueur - 3] = '\0';
        strcat(nom_sortie, ".out");
        return nom_sortie;
    } else {
        char *nom_sortie = (char*)malloc(longueur + 5); // +5 pour ".out" et '\0'
        if (!nom_sortie) {
            fprintf(stderr, "AODjustify ERROR> Allocation mémoire échouée\n");
            exit(EXIT_FAILURE);
        }
        strcpy(nom_sortie, nom_base);
        strcat(nom_sortie, ".out");
        return nom_sortie;
    }
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

    // Nom de base (sans extension)
    const char *nom_base = argv[2];

    // Générer le nom du fichier d'entrée (ajouter .in)
    char *nom_entree = generer_nom_entree(nom_base);


    // Vérifier l'encodage
    struct stat info_fichier = verifier_encodage_fichier(nom_entree);

    // Ouvrir le fichier d'entrée
    int fd = open(nom_entree, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "AODjustify ERROR> Ouverture fichier impossible\n");
        free(nom_entree);
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
        munmap(fichier, (size_t)info_fichier.st_size);
        close(fd);
        free(nom_entree);
        free(nom_sortie);
        exit(1);
    }

    // Traiter le fichier
    traiter_fichier(fichier, (size_t)info_fichier.st_size, sortie);

    // Fermer le fichier de sortie
    fclose(sortie);

    // Afficher la norme totale
    fprintf(stderr, "AODjustify CORRECT> %lld\n", valeur_norme_totale);

    // Nettoyage
    munmap(fichier, (size_t)info_fichier.st_size);
    close(fd);
    free(nom_entree);
    free(nom_sortie);

    return 0;
}
