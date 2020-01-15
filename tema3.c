/*
	JIANU Maria 331CB
	Tema 3 APD
*/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//smooth
float smooth[3][3] = {
	{1/9, 1/9, 1/9},
	{1/9, 1/9, 1/9},
	{1/9, 1/9, 1/9}
};

//blur
float blur[3][3] = {
	{1/16, 2/16, 1/16},
	{2/16, 4/16, 2/16},
	{1/16, 2/16, 1/16}
};

//sharpen
float sharpen[3][3] = {
	{0, -2/3, 0},
	{-2/3, 11/3, -2/3},
	{0, -2/3, 0}
};

//mean
float mean[3][3] = {
	{-1, -1, -1},
	{-1, 9, -1},
	{-1, -1, -1}
};

//emboss
float emboss[3][3] = {
	{0, 1, 0},
	{0, 0, 0},
	{0, -1, 0}
};

typedef struct {
	unsigned char rgb[3];
} Pixel;

typedef struct {
	int width;			     
	int height;    			 
	int maxValue;  
	int type; 
	int numColors; 
	Pixel** pixels;
} Image;

int main(int argc, char* argv[]) {
	
	int rank;
	int nProcesses;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

	int i, j;
	
	Image in;
	Image out;


	//rankul 0 citeste imaginile
	
		if (rank == 0) {
			
			FILE* file = fopen(argv[1], "rb");

			//sar peste litera P
			fseek(file, 1, SEEK_SET);

			fscanf(file, "%d\n", &(in.type));

			//sar peste linia cu #
			char comment[100];
			fgets(comment, 100, file);

			fscanf(file, "%d %d\n", &(in.width), &(in.height));
			fscanf(file, "%d\n", &(in.maxValue));

			if(in.type == 5)
				in.numColors = 1;
			else
				in.numColors = 3;

			in.pixels = (Pixel**) malloc(sizeof(Pixel*) * in.height);
		

			// citesc fiecare pixel
			for (i = 0; i < in.height; ++i) {
				in.pixels[i] = (Pixel*) malloc(sizeof(Pixel) * in.width);
				
				if (in.type == 6) {
					fread(&in.pixels[i][0], in.width * in.numColors, 1, file);
				} 

				else {
					for (j = 0; j < in.width; ++j) {
						fread(&in.pixels[i][j].rgb, in.numColors, 1, file);
					}
				}
			}
			fclose(file);
		}

		//trimit si primesc imaginea in prin Bcast
		MPI_Bcast(
			&in.width, 
			1, 
			MPI_INT,  
			0, 
			MPI_COMM_WORLD);
		MPI_Bcast(
			&in.height, 
			1, 
			MPI_INT,  
			0, 
			MPI_COMM_WORLD);
		MPI_Bcast(
			&in.type, 
			1, 
			MPI_INT,
			0, 
			MPI_COMM_WORLD);
		MPI_Bcast(
			&in.maxValue, 
			1, 
			MPI_INT, 
			0, 
			MPI_COMM_WORLD);
		MPI_Bcast(
			&in.numColors, 
			1, 
			MPI_INT, 
			0, 
			MPI_COMM_WORLD);

		//aloc imaginea de intrare
		if (rank != 0) {
			in.pixels = (Pixel**) malloc(sizeof(Pixel*) * in.height);

			for (int i = 0; i < in.height; ++i) {
				in.pixels[i] = (Pixel*) malloc(sizeof(Pixel) * in.width);
			}
		}

		//setez si aloc imagiena de iesire
		out.type = in.type;
		out.width = in.width;
		out.height = in.height;
		out.maxValue  = in.maxValue;
		out.numColors = in.numColors;

		out.pixels = (Pixel**) malloc(sizeof(Pixel*) * out.height);

		for (int i = 0; i < out.height; ++i) {
			out.pixels[i] = (Pixel*) malloc(sizeof(Pixel) * out.width);
		}

		//aplicam fiecare filtru din input
		for (int filter = 3; filter < argc; filter++) {
			//rankul 0 trimite imaginea linie cu linie
			for (int i = 0; i < in.height; i++) {
				MPI_Bcast(
					&in.pixels[i][0], 
					3 * in.width, 
					MPI_CHAR, 
					0, 
					MPI_COMM_WORLD);
			}

			int start = (in.height/nProcesses) * rank;
			int stop;

			if (rank == nProcesses - 1)
				stop = in.height;
			else
				stop = (in.height/nProcesses) * (rank + 1);


			float filter_mat[3][3];
			
			//vad ce tip de filtru este
			if (strcmp(argv[filter], "smooth") == 0) {
				memcpy(filter_mat, smooth, 9 * sizeof(float));
			}
			
			if (strcmp(argv[filter], "emboss") == 0) {
				memcpy(filter_mat, emboss, 9 * sizeof(float));
			} 
			
			if (strcmp(argv[filter], "blur") == 0) {
				memcpy(filter_mat, blur, 9 * sizeof(float));
			}

			if (strcmp(argv[filter], "sharpen") == 0) {
				memcpy(filter_mat, sharpen, 9 * sizeof(float));
			} 

			if (strcmp(argv[filter], "mean") == 0) {
				memcpy(filter_mat, mean, 9 * sizeof(float));
			}

			//aplicam filtrul
			for (int i = start; i < stop; i++) {
				for (int j = 0; j < in.width; j++) {
					for (int k = 0; k < 3; k++) {
						//cele de pe margine raman nemodificate
						if (i != 0 && i != in.height - 1 
							&& j != 0 && j != in.width - 1) {
							float final = 0;
							for (int p = -1; p <= 1; p++) {
								for (int l = -1; l <= 1; l++) {
									final += in.pixels[i + p][j 
										+ l].rgb[k] 
										* filter_mat[p + 1][l + 1];
								}
							}
							//clampez valorile
							if(final > 255)
								final = 255;
							if(final < 0)
								final = 0;
							out.pixels[i][j].rgb[k] = (unsigned char)final;
						} 
						else {
							out.pixels[i][j].rgb[k] = (unsigned char)in.pixels[i][j].rgb[k];
						}
					}
				}
			}

			//fiecare proces trimite catre procesul 0 bucata lui de imagine
			for (int l = start; l < stop; l++) {
				MPI_Send(
					&out.pixels[l][0], 
					3 * in.width, 
					MPI_CHAR, 
					0, 
					0, 
					MPI_COMM_WORLD);
			}

			//rankul 0 primeste partile procesate 
			if (rank == 0) {
				for (int i = 0; i < nProcesses; i++) {
					int start = (in.height / nProcesses) * i;
					int stop;

					if (i == nProcesses - 1)
						stop = in.height;
					else
						stop = (in.height / nProcesses) * (i + 1);

					for (int l = start; l < stop; l++) {
						MPI_Recv(
							&in.pixels[l][0], 
							3 * in.width,
							MPI_CHAR, 
							i, 
							0, 
							MPI_COMM_WORLD, 
							&status);
					}
				}
			}
		}

		//rankul 0 scrie imaginea finala in fisier
		if (rank == 0) {
			FILE* output_file = fopen(argv[2], "wb");

			fprintf(output_file, "P%d\n", in.type);
			fprintf(output_file, "%d %d\n", in.width, in.height);
			fprintf(output_file, "%d\n", in.maxValue);

			//scriu fiecare pixel
			for (int i = 0; i < in.height; ++i) {
				if (in.type == 6) {
					fwrite(&in.pixels[i][0], 1, in.width * in.numColors, output_file);
				} else {
					for (int j = 0; j < in.width; ++j) {
						fwrite(&in.pixels[i][j], 1, in.numColors, output_file);
					}
				}
			}

			fclose(output_file);
		}
		
		MPI_Finalize();
	return 0;
}