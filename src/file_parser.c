#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

long long M;

typedef struct
{
    char *mot;
    long long taille;
} Mot ;

// TODO: le fichier en input doit avoir le .in ? Il doit contenit le dossier?
// TODO: traiter plusieurs paragraphes


// Vérifie si le fichier existe et s'il est ISO-8859
struct stat check_file_encoding(const char *filename) {
    struct stat fileInfo;

    if (stat(filename, &fileInfo) != 0) {
        fprintf(stderr,"AODjustify ERROR> Le fichier n'a pas été trouvé. \n");
        exit(1);
    }

    char command[1024];
    snprintf(command, sizeof(command), "file -b '%s' | grep -q 'ISO-8859'", filename);

    if (system(command) != 0)
    {
        fprintf(stderr,"AODjustify ERROR> Fichier en entrée pas au format ISO-8859-1. \n");
        exit(1);
    }
    return fileInfo;
}

char* open_file(const char* filename, struct stat fileInfo, int fd)
{
    if (fd == -1 || stat(filename, &fileInfo) == -1) {
        fprintf(stderr,"AODjustify ERROR> Ouverture fichier impossible");
        exit(1);
    }

    char* map = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr,"AODjustify ERROR> Ouverture fichier impossible");
        exit(1);
    }
    return map;
}

long long delta_i_k(Mot *mots[], long long i, long long k)
{
    long long somme = 0;
    for (long long idx = i; idx < k; idx++)
    {
        somme += mots[idx]->taille;
        if (idx != k - 1)
        {
            somme ++; // Les espaces entre mots
        }
    }
    return M - somme;
}

long long missing_spaces(Mot *mots[], long long i, long long k)
{
    long long somme = 0;
    for (long long idx = i; idx < k; idx++)
    {
        somme += mots[idx]->taille;
    }
    return M - somme;
}

long long norme(const long long e_j)
{
    return e_j * e_j * e_j;
}

long long cout_minimal (Mot *mots[], long long i, long long n)
{
    long long E_i_n = delta_i_k(mots, i, n);
    if (E_i_n >= 0)
    {
        return n;
    }

    long long cout_min = -1; // Pour le premier calcul
    long long index_end_of_line = i;
    for (long long k = i; k < n; k++)
    {
        long long E_j_k = delta_i_k(mots, i, k);
        if (E_j_k < 0)
        {
            return index_end_of_line;
        }

        long long cout_min_intermed = norme(E_j_k) + cout_minimal(mots, k + 1, n);
        if (cout_min_intermed < cout_min || cout_min == -1)
        {
            cout_min = cout_min_intermed;
            index_end_of_line = k;
        }
    }
    return index_end_of_line;
}

void display_formated_lines(Mot *mots[], long long i, long long k)
{
    long long spaces_a_distribuer = missing_spaces(mots, i, k);
    long long spaces_per_mot = spaces_a_distribuer / (k - i - 1); // spaces uniquement entre les mots
    long long spaces_en_plus = spaces_a_distribuer % (k - i - 1);

    for (long long idx = i; idx < k; idx++)
    {
        printf("%s", mots[idx]->mot);
        if (idx != k - 1)
        {
            for (int j = 0; j < spaces_per_mot; j++)
            {
                printf(" ");
            }
            if (spaces_en_plus > 0)
            {
                printf(" ");
                spaces_en_plus --;
            }
        }
    }
}

long long parse_mots(const char *file, const long long file_length, Mot *mots[])
{
    long long compteur_mots = 0;
    long long offset = 0;
    long long word_start = -1;

    while (offset < file_length)
    {
        // Char fait partie d'un mot
        if (!isspace(file[offset]) && word_start == -1)
        {
            word_start = offset;
        }
        else if (file[offset] == ' ' || file[offset] == '\n')
        {
            long long word_length = offset - word_start;
            mots[compteur_mots] = (Mot*)malloc(sizeof(Mot));

            // Recopie le mot
            mots[compteur_mots]->mot = (char*)malloc((word_length + 1) * sizeof(char));
            strncpy(mots[compteur_mots]->mot, &file[word_start], word_length);
            mots[compteur_mots]->mot[word_length] = '\0';

            //Enregistre la taille
            mots[compteur_mots]->taille = word_length;

            compteur_mots++;
            word_start = -1;
        }

        offset++;
    }

    // Traite le dernier mot si le paragraphe ne fini pas par un " "
    if (word_start != -1)
    {
        long long word_length = offset - word_start;

        mots[compteur_mots] = (Mot*)malloc(sizeof(Mot));
        mots[compteur_mots]->mot = (char*)malloc((word_length + 1) * sizeof(char));
        strncpy(mots[compteur_mots]->mot, &file[word_start], word_length);
        mots[compteur_mots]->mot[word_length] = '\0';
        mots[compteur_mots]->taille = word_length;

        compteur_mots++;
    }

    // printf("Word count: %lld\n", compteur_mots);

    return compteur_mots;
}

void justify(Mot *mots[], long long compteur_mots)
{
    long long i = 0;
    while (1)
    {
        long long k = cout_minimal(mots, i, compteur_mots);

        // Ligne interieure
        if (k < compteur_mots - 1)
        {
            display_formated_lines(mots, i, k);
        }
        else
        {
            for (long long idx = i; idx < compteur_mots; idx++)
            {
                printf("%s", mots[idx]->mot);
                if (idx != k - 1)
                {
                    printf(" ");
                }
            }
            printf("\n");
            break;
        }
        printf("\n");
        i = k;

        if (k == 17)
        {
            break;
        }
    }
}

int main(int argc, char* argv[]){
    if (argc < 3)
    {
        fprintf(stderr,"AODjustify ERROR> Utilisation: ./bin/AODjustify <filename> <M>\n");
        exit(1);
    }
    M = strtol(argv[1], NULL, 10);
    if (M <= 0)
    {
        fprintf(stderr,"AODjustify ERROR> Utilisation: M doit être positif\n");
        exit(1);
    }

    const char *filename = argv[2];

    const struct stat fileInfo = check_file_encoding(filename);
    const int fd = open(filename, O_RDONLY);

    char* file = open_file(filename, fileInfo, fd);

    Mot *mots[1024];
    const long long compteur_mots = parse_mots(file, fileInfo.st_size, mots);

    justify(mots, compteur_mots);

    close(fd);
    munmap(file, fileInfo.st_size);
    return 0;
}