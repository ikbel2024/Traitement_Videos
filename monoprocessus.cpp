#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <time.h>

using namespace cv;
using namespace std; // Add this line to use the standard library containers

// Fonction pour détecter les mouvements et afficher la position
int detect_movement(const char *video_path) {
    printf("Traitement de la vidéo : %s\n", video_path);
    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        fprintf(stderr, "Erreur lors de l'ouverture de la vidéo %s\n", video_path);
        return -1;
    }

    Mat frame, gray, prev_gray, diff, thresh, contours_img;
    bool first_frame = true;

    while (cap.read(frame)) {
        cvtColor(frame, gray, COLOR_BGR2GRAY);  // Conversion en niveaux de gris

        if (!first_frame) {
            absdiff(prev_gray, gray, diff);  // Différence absolue entre les images
            threshold(diff, thresh, 25, 255, THRESH_BINARY);  // Seuil pour détecter les changements
            
            // Trouver les contours dans l'image binaire
            vector<vector<Point>> contours;  // Use std::vector
            findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            for (size_t i = 0; i < contours.size(); i++) {
                // Calculer le rectangle englobant pour chaque contour
                Rect bounding_box = boundingRect(contours[i]);

                // Calculer le centre du rectangle (mouvement détecté)
                Point center = (bounding_box.br() + bounding_box.tl()) * 0.5;

                // Afficher les coordonnées du centre du mouvement détecté dans la console
                printf("Mouvement détecté à la position (x, y) : (%d, %d)\n", center.x, center.y);

                // Dessiner le contour et le centre sur l'image pour visualiser
                drawContours(frame, contours, (int)i, Scalar(0, 255, 0), 2); // Contour en vert
                circle(frame, center, 5, Scalar(0, 0, 255), -1);  // Centre en rouge

                // Ajouter du texte avec les coordonnées du centre
                char text[50];
                snprintf(text, sizeof(text), "Pos: (%d, %d)", center.x, center.y);
                putText(frame, text, center, FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 0, 0), 2);
            }
        }

        imshow("Mouvement Détecté", frame);  // Afficher l'image avec les contours et le centre
        if (waitKey(30) >= 0) break;  // Sortir si une touche est pressée

        gray.copyTo(prev_gray);  // Sauvegarder l'image actuelle pour la comparaison suivante
        first_frame = false;
    }

    cap.release();
    return 0;
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

    // Traiter les vidéos une par une dans un processus mono-thread
    for (int i = 0; i < video_count; i++) {
        detect_movement(video_files[i]);
        free(video_files[i]);  // Libérer la mémoire de chaque chemin vidéo
    }

    free(video_files);  // Libérer le tableau des chemins vidéo

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps total d'exécution (MonoProcessus) : %.2f secondes\n", elapsed_time);

    return 0;
}

