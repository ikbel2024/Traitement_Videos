#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <time.h>
#include <vector>  // Pour les vecteurs

using namespace cv;
using namespace std;

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
        cvtColor(frame, gray, COLOR_BGR2GRAY);  // Conversion en niveaux de gris

        if (!first_frame) {
            absdiff(prev_gray, gray, diff);  // Différence entre les images
            threshold(diff, diff, 25, 255, THRESH_BINARY);  // Application d'un seuil
            
            // Nombre de pixels affectés par le mouvement
            int movement_pixels = countNonZero(diff);  
            if (movement_pixels > 0) {
                printf("Mouvement détecté dans %s, Nombre de pixels affectés : %d\n", video_path, movement_pixels);

                // Trouver les contours des zones de mouvement
                vector<vector<Point>> contours;
                findContours(diff, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

                // Dessiner les contours et afficher les informations
                for (size_t i = 0; i < contours.size(); i++) {
                    Moments m = moments(contours[i]);
                    if (m.m00 > 0) {
                        int x = static_cast<int>(m.m10 / m.m00);  // Coordonnée x du centre
                        int y = static_cast<int>(m.m01 / m.m00);  // Coordonnée y du centre
                        printf("Mouvement détecté à la position : (%d, %d)\n", x, y);

                        // Dessiner le contour et le centre du mouvement détecté sur l'image
                        drawContours(frame, contours, (int)i, Scalar(0, 255, 0), 2);  // Dessiner les contours en vert
                        circle(frame, Point(x, y), 5, Scalar(0, 0, 255), -1);  // Cercle rouge au centre du mouvement
                        
                        // Afficher la position du centre sur l'image
                        char text[50];
                        snprintf(text, sizeof(text), "Pos: (%d, %d)", x, y);
                        putText(frame, text, Point(x + 10, y + 10), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 0, 0), 2);  // Texte bleu
                    }
                }
            }
        }

        // Afficher l'image avec les contours et les positions détectées
        imshow("Mouvement Détecté", frame);
        if (waitKey(30) >= 0) break;  // Sortir si une touche est pressée

        gray.copyTo(prev_gray);
        first_frame = false;
    }

    cap.release();
    pthread_exit(NULL);
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
    vector<char *> video_files;  // Utilisation de std::vector pour simplifier la gestion de la mémoire

    // Récupérer les fichiers vidéo
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char *video_path = (char *)malloc(512 * sizeof(char));  // Allocation de mémoire pour chaque chemin vidéo
            snprintf(video_path, 512, "videos/%s", entry->d_name);
            video_files.push_back(video_path);  // Ajouter chaque vidéo à la liste
        }
    }

    closedir(dir);

    // Créer des threads pour chaque vidéo
    vector<pthread_t> threads(video_files.size());
    for (size_t i = 0; i < video_files.size(); i++) {
        pthread_create(&threads[i], NULL, detect_movement, video_files[i]);
    }

    // Attendre la fin de tous les threads
    for (size_t i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
        free(video_files[i]);  // Libérer la mémoire après que le thread ait terminé
    }

    video_files.clear();  // Vider le vector

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps total d'exécution (Multithreads Parallèle) : %.2f secondes\n", elapsed_time);

    return 0;
}

