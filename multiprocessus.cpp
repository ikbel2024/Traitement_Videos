#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>  // Ajoutez cet en-tête pour utiliser std::vector

using namespace cv;
using namespace std;  // N'oubliez pas d'ajouter cet espace de noms pour std::vector

int detect_movement(const char *video_path) {
    printf("Traitement de la vidéo : %s dans le processus %d\n", video_path, getpid());
    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        fprintf(stderr, "Erreur lors de l'ouverture de la vidéo %s\n", video_path);
        return -1;
    }

    Mat frame, gray, prev_gray, diff;
    bool first_frame = true;

    while (cap.read(frame)) {
        cvtColor(frame, gray, COLOR_BGR2GRAY);  // Conversion en niveaux de gris

        if (!first_frame) {
            absdiff(prev_gray, gray, diff);  // Différence entre les images
            threshold(diff, diff, 25, 255, THRESH_BINARY);  // Application d'un seuil
            
            // Trouver les contours des zones de mouvement
            vector<vector<Point>> contours;  // Utilisation de std::vector
            findContours(diff, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            for (size_t i = 0; i < contours.size(); i++) {
                Moments m = moments(contours[i]);
                if (m.m00 > 0) {  // Si le contour a une zone non nulle
                    int x = static_cast<int>(m.m10 / m.m00);  // Coordonnée x du centre
                    int y = static_cast<int>(m.m01 / m.m00);  // Coordonnée y du centre
                    printf("Mouvement détecté dans %s à la position : (%d, %d)\n", video_path, x, y);

                    // Dessiner le contour et le centre du mouvement détecté sur l'image
                    drawContours(frame, contours, (int)i, Scalar(0, 255, 0), 2);  // Dessiner les contours en vert
                    circle(frame, Point(x, y), 5, Scalar(0, 0, 255), -1);  // Cercle rouge au centre du mouvement
                    
                    // Afficher la position sur l'image
                    char text[50];
                    snprintf(text, sizeof(text), "Pos: (%d, %d)", x, y);
                    putText(frame, text, Point(x + 10, y + 10), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 0, 0), 2);  // Texte bleu
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
    return 0;
}

int main() {
    clock_t start_time = clock();  // Démarrer le chronomètre
    
    struct dirent *entry;
    DIR *dir = opendir("videos");  // Ouvrir le dossier contenant les vidéos
    if (dir == NULL) {
        printf("Impossible d'ouvrir le dossier de vidéos\n");
        return 1;
    }

    // Lire chaque fichier dans le dossier "videos"
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Si c'est un fichier (pas un dossier)
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "videos/%s", entry->d_name);
            
            pid_t pid = fork();
            if (pid == 0) {
                // Processus enfant
                detect_movement(filepath);
                _exit(0); // Termine le processus enfant
            }
        }
    }

    // Attendre la fin de tous les processus enfants
    while (wait(NULL) > 0);

    closedir(dir);

    clock_t end_time = clock();  // Fin du chronomètre
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps total d'exécution (Multiprocessus) : %.2f secondes\n", elapsed_time);

    return 0;
}

