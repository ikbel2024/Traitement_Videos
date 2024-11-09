#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>  // Ajout de l'en-tête nécessaire pour wait()

using namespace cv;

void *detect_movement(void *arg) {
    const char *video_path = (const char *)arg;
    printf("Traitement de la vidéo dans un thread : %s\n", video_path);
    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        fprintf(stderr, "Erreur lors de l'ouverture de la vidéo %s\n", video_path);
        pthread_exit(NULL);
    }

    Mat frame, gray, prev_gray, diff;
    bool first_frame = true;

    while (cap.read(frame)) {
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        if (!first_frame) {
            absdiff(prev_gray, gray, diff);
            threshold(diff, diff, 25, 255, THRESH_BINARY);
            int movement_pixels = countNonZero(diff);
            if (movement_pixels > 0) {
                printf("Mouvement détecté dans %s\n", video_path);
            }
        }
        gray.copyTo(prev_gray);
        first_frame = false;
    }

    cap.release();
    pthread_exit(NULL);
}

void process_videos_in_directory(char **video_files, int video_count) {
    pthread_t threads[video_count];
    
    // Créer des threads en parallèle pour traiter les vidéos simultanément dans chaque processus
    for (int i = 0; i < video_count; i++) {
        pthread_create(&threads[i], NULL, detect_movement, video_files[i]);
    }

    // Attendre la fin de tous les threads
    for (int i = 0; i < video_count; i++) {
        pthread_join(threads[i], NULL);
        free(video_files[i]);  // Libérer la mémoire de chaque chemin vidéo
    }
}

int main() {
    clock_t start_time = clock();

    struct dirent *entry;
    DIR *dir = opendir("videos");
    if (dir == NULL) {
        printf("Impossible d'ouvrir le dossier de vidéos\n");
        return 1;
    }

    // Liste des vidéos à traiter
    char **video_files = NULL;
    int video_count = 0;

    // Récupérer les fichiers vidéo
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            video_files = (char **)realloc(video_files, (video_count + 1) * sizeof(char *));
            video_files[video_count] = (char *)malloc(512 * sizeof(char));
            snprintf(video_files[video_count], 512, "videos/%s", entry->d_name);
            video_count++;
        }
    }

    closedir(dir);

    // Déterminer le nombre de processus à créer (en fonction des cœurs disponibles)
    int num_processes = 4; // Exemple avec 4 processus. Vous pouvez ajuster selon le nombre de cœurs.
    int videos_per_process = video_count / num_processes;
    if (video_count % num_processes != 0) {
        videos_per_process++;
    }

    // Créer plusieurs processus pour traiter les vidéos
    pid_t pid;
    for (int i = 0; i < num_processes; i++) {
        pid = fork();
        if (pid == 0) {  // Code du processus enfant
            // Diviser les vidéos à traiter pour ce processus
            int start_index = i * videos_per_process;
            int end_index = (i + 1) * videos_per_process;
            if (end_index > video_count) {
                end_index = video_count;
            }
            
            // Processus traite les vidéos assignées
            process_videos_in_directory(&video_files[start_index], end_index - start_index);

            exit(0); // Quitter le processus enfant après traitement
        }
    }

    // Attendre que tous les processus enfants terminent
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    free(video_files);  // Libérer la mémoire du tableau des chemins vidéo

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps total d'exécution (Multiprocessus + Multithreads) : %.2f secondes\n", elapsed_time);

    return 0;
}

