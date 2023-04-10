#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>

int isspace(int argument);
// declare structure pgm 
typedef struct {
	char version[3]; 
	int width;
	int height;
	int maxGrayLevel;
	int **imageData;
	int **gx;
	int **gy;
} pgm;

void init_out_image( pgm* out, pgm image){
	int i, j;
	//copy les dimensions ,la version et maxgraylevel sur les champs de out image
	strcpy(out->version, image.version);
	out->width = image.width;
	out->height = image.height;
	out->maxGrayLevel = image.maxGrayLevel;
	// reserver memoire pour stocker les info d'image
	out->imageData = (int**) calloc(out->height, sizeof(int*));
	for(i = 0; i < out->height; i++) {
		out->imageData[i] = calloc(out->width, sizeof(int));
	}
	//reserver memoire pour stocker les infos de gradient x
	out->gx = (int**) calloc(out->height, sizeof(int*));
	for(i = 0; i < out->height; i++) {
		out->gx[i] = calloc(out->width, sizeof(int));
	}
	//reserver memoire pour stocker les infos de gradient y
	out->gy = (int**) calloc(out->height, sizeof(int*));
	for(i = 0; i < out->height; i++) {
		out->gy[i] = calloc(out->width, sizeof(int));
	}
	//reserver memoire pour stocker les infos de gradient 
	for(i = 0; i < out->height; i++) {
		for(j = 0; j < out->width; j++) {
			out->imageData[i][j] = image.imageData[i][j];
			out->gx[i][j] = image.imageData[i][j];
			out->gy[i][j] = image.imageData[i][j];
		};
	}
}

void ignore_comments(FILE *input_image) {
	char ch;
	char line[100];
   // ignorer les lignes vides
	while ((ch = fgetc(input_image)) != EOF && (isspace(ch)))
	;
    //ignore les commantaires avec le mecanisme de recursivité
	//les Commentaires dans les images PGM  Commencent par '#'
	if (ch == '#') {
        fgets(line, sizeof(line), input_image);
		ignore_comments(input_image);
    } 
	else {
	//si la ligne ne commence pas par # on deplace le pointeur input-image vers la ligne suivante
		fseek(input_image, -1, SEEK_CUR);
	}
}
//fontion pour ouvrir l'image saisie
void read_pgm_file(char* dir, pgm* image) {
void read_pgm_file(char* dir, pgm* image) {
	FILE* input_image; 
	int i, j, num;
    //lire l'image en mode 'read binary' 
	input_image = fopen(dir, "rb");
	//si le fichier n'exite pas on affiche le message suivant
	if (input_image == NULL) {
		printf("File could not opened!");
		return;
	} 

	//si le fichier existe 
	//lire les 3 caracteres de la premiere ligne de fichier input_image et les stocker dans dans le string image->version afin d'extraire la version d'image
	fgets(image->version, sizeof(image->version), input_image);
	//lire les commentaires de input_image
	ignore_comments(input_image);
    //lire les dimensions et maxGraylevel d'image 
	fscanf(input_image, "%d %d %d", &image->width, &image->height, &image->maxGrayLevel);
	//allocation de memoire pour stocker les informations d'image (la valeure de chaque pixel)
	image->imageData = (int**) calloc(image->height, sizeof(int*));
	for(i = 0; i < image->height; i++) {
		image->imageData[i] = calloc(image->width, sizeof(int));

	}
	//verifier si la version d'image est de type P2 ou
	// de type P5 pour savoir comment on doit stocker 
	if (strcmp(image->version, "P2") == 0) {
		for (i = 0; i < image->height; i++) {
			for (j = 0; j < image->width; j++) {
				fscanf(input_image, "%d", &num);
				image->imageData[i][j] = num;
			}
		}	
	}
	else if (strcmp(image->version, "P5") == 0) {
		char *buffer;
		int buffer_size = image->height * image->width;
		buffer = (char*) malloc( ( buffer_size + 1) * sizeof(char));
		
		if(buffer == NULL) {
			printf("Can not allocate memory for buffer! \n");
			return;
		}
		fread(buffer, sizeof(char), image->width * image-> height, input_image);
		for (i = 0; i < image->height * image ->width; i++) {
			image->imageData[i / (image->width)][i % image->width] = buffer[i];
		}
		free(buffer);
	}
	fclose(input_image);
	// affichage les info d'image
	 printf("_______________IMAGE INFO__________________\n");
	 printf("Version: %s \nWidth: %d \nHeight: %d \nMaximum Gray Level: %d \n", image->version, image->width, image->height, image->maxGrayLevel);
}
//ajoutons une bordure de pixels avec la valeur zéro autour des bords des images d’entrée
void padding(pgm* image) {
	int i;
	for (i = 0; i < image->width; i++) {
		image->imageData[0][i] = 0;
		image->imageData[image->height - 1][i] = 0;
	}
	
	for (i = 0; i < image->height; i++) {
		image->imageData[i][0] = 0;
		image->imageData[i][image->width - 1] = 0;
	} 
}

int convolution(pgm* image, int kernel[3][3], int row, int col) {
	int i, j, sum = 0;
	//calculons le produit de convolution entre la matrice kernel 3*3 et notre matrice image 
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			sum += image->imageData[i + row][j + col] * kernel[i][j];
		}
	}
	return sum;
}

void sobel_edge_detector(pgm* image, pgm* out_image) {
	int i, j, gx, gy;
	//declarer le masque mx
	int mx[3][3] = {
		{-1, 0, 1},
		{-2, 0, 2},
		{-1, 0, 1}
	};
	//declarer le masque my
	int my[3][3] = {
		{-1, -2, -1},
		{0, 0, 0},
		{1, 2, 1}
	};
	
	for (i = 1; i < image->height - 2; i++) {
		for (j = 1; j < image->width - 2; j++) {
			//appliquer le produit de convolution entre le masque x et l'image afin de calculer gradient x
			gx = convolution(image, mx, i, j);
			//appliquer le produit de convolution entre le masque y et l'image afin de calculer gradient y
			gy = convolution(image, my, i, j);
			//pour obtenir le
			out_image->imageData[i][j] = sqrt(gx*gx + gy*gy);
			out_image->gx[i][j] = gx;
			out_image->gy[i][j] = gy;
		}
	}
	
}

void min_max_normalization(pgm* image, int** matrix) {
	int min = 1000000, max = 0, i, j;
	//extraire la valeur min 
	for(i = 0; i < image->height; i++) {
		for(j = 0; j < image->width; j++) {
			if (matrix[i][j] < min) {
				min = matrix[i][j];
			}
			//extraire la valeur max
			else if (matrix[i][j] > max) {
				max = matrix[i][j];
			}
		}
	}
	// nous modifions les données pour avoir une limite inférieure de 0. Pour ce faire, nous soustrayons la valeur minimale de chaque valeur
	//Ensuite, nous modifions les données pour avoir une limite supérieure de 1. Pour ce faire, nous divisons chaque valeur par la plage d’origine :
	//enfin on multiplie tout les données par 255 pour avoir un plage des valeurs entre 0 et 255
	for(i = 0; i < image->height; i++) {
		for(j = 0; j < image->width; j++) {
			double ratio = (double) (matrix[i][j] - min) / (max - min);
			matrix[i][j] = ratio * 255;
		}
	} 
}
			

void write_pgm_file(pgm* image, char dir[], int** matrix, char name[]) {
	FILE* out_image;
	int i, j, count = 0;
	//Extraire le mot avant . et le stocker dans token 
	char* token = strtok(dir, ".");
	if (token != NULL) {
		//concatenation de token et le nom
		strcat(token, name);
		//ouvrir token image en mode write binary
		out_image = fopen(token, "wb");
	}
	//ouvrir dir image en mode write binary
	out_image = fopen(dir, "wb");
	//ecrire les infos de l'image dans le file out_image(la version les dimensions et les valeurs pixels)
	fprintf(out_image, "%s\n", image->version);
	fprintf(out_image, "%d %d\n", image->width, image->height);
	fprintf(out_image, "%d\n", image->maxGrayLevel);
	
	if (strcmp(image->version, "P2") == 0) {
		for(i = 0; i < image->height; i++) {
			for(j = 0; j < image->width; j++) {
				fprintf(out_image,"%d", matrix[i][j]);
				if (count % 17 == 0) 
					fprintf(out_image,"\n");
				else 
					fprintf(out_image," ");
				count ++;
			}
		} 
	}
	else if (strcmp(image->version, "P5") == 0) {
		for(i = 0; i < image->height; i++) {
			for(j = 0; j < image->width; j++) {
				char num = matrix[i][j];
				fprintf(out_image,"%c", num);
			}
		} 
	} 
	fclose(out_image);
}

int main(int argc, char **argv)
{   

 	pgm image[27], out_image[27];
	char dir[27][200]={{"sinjab.pgm"},{"lion.pgm"},{"montain.pgm"},{"mou.pgm"},{"city.pgm"},{"gh.pgm"},{"gha.pgm"},{"hh.pgm"},{"horse.pgm"},{"kwala.pgm"},{"mouche.pgm"},{"mer.pgm"},{"giraf.pgm"},{"mm.pgm"},{"n.pgm"},{"duck.pgm"},{"tom1.pgm"},{"dora.pgm"},{"giraff.pgm"},{"spongpop.pgm"},{"zon.pgm"},{"micky.pgm"},{"jerry.pgm"},{"giraff1.pgm"},{"elephant.pgm"},{"imgg.pgm"},{"chat.pgm"}};
    double T1;
	double T2;
    T1=omp_get_wtime();
	for(int i=0;i<27;i++){
	int ID=omp_get_thread_num();
	//lire l'image 
	//extraire ses infos et les afficher 
	read_pgm_file(dir[i], &image[i]);
	//ajoutons une bordure de pixels avec la valeur zéro autour des bords  d'images d’entrée
	padding(&image[i]);
    //initialiser out image a partie les infos de l'image et reserver la memoire pour stocker le gradient x rt y
	init_out_image(&out_image[i], image[i]);
	//appliquer l'algorithme de sobel sur l'image 
	sobel_edge_detector(&image[i], &out_image[i]);	
     // pour modifie la plage de valeurs d’intensité des pixels [0.255] de gx et gy et image gradient 
	   min_max_normalization(&out_image[i], out_image[i].imageData);
	   min_max_normalization(&out_image[i], out_image[i].gx);
	   min_max_normalization(&out_image[i], out_image[i].gy);
   
    //write file avec extension .G.pgm et ecrire sur lui les infos de l'image
	write_pgm_file(&out_image[i], dir[i], out_image[i].imageData, ".G.pgm");
	printf("\nGradient saved: %s (%d) \n", dir[i],ID); 

    }
	T2=omp_get_wtime();
	printf("time is %f",T2-T1);

	return 0;
}