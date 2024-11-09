#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>  // Pour les vecteurs

using namespace cv;
using namespace std;

int detect_movement(const char *video_path, int pipe_fd) {
    printf("Traitement de la vidéo : %s dans le processus %d\n", video_path, getpid());
    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        fprintf(stderr, "Erreur lors de l'ouverture de la vidéo %s\n", video_path);
        return -1;
    }

    Mat frame, gray, prev_gray, diff;
    bool first_frame = true;
    bool movement_detected = false;

    while (cap.read(frame)) {
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        if (!first_frame) {
            absdiff(prev_gray, gray, diff);  // Calcul de la différence
            threshold(diff, diff, 25, 255, THRESH_BINARY);  // Seuillage
            int movement_pixels = countNonZero(diff);  // Nombre de pixels affectés par le mouvement

            if (movement_pixels > 0) {
                movement_detected = true;

                // Trouver les contours du mouvement
                vector<vector<Point>> contours;
                findContours(diff, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                
                // Dessiner les contours et afficher les informations
                for (size_t i = 0; i < contours.size(); i++) {
                    Moments m = moments(contours[i]);
                    if (m.m00 > 0) {
                        int x = static_cast<int>(m.m10 / m.m00);  // Coordonnée x du centre
                        int y = static_cast<int>(m.m01 / m.m00);  // Coordonnée y du centre
                        
                        // Dessiner le contour et le centre du mouvement détecté
                        drawContours(frame, contours, (int)i, Scalar(0, 255, 0), 2);  // Dessiner les contours en vert
                        circle(frame, Point(x, y), 5, Scalar(0, 0, 255), -1);  // Cercle rouge au centre
                        
                        // Afficher la position du centre sur l'image
                        char text[50];
                        snprintf(text, sizeof(text), "Pos: (%d, %d)", x, y);
                        putText(frame, text, Point(x + 10, y + 10), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 0, 0), 2);  // Texte bleu
                    }
                }

                // Afficher le nombre de pixels affectés par le mouvement
                dprintf(pipe_fd, "Mouvement détecté dans %s, Nombre de pixels affectés : %d\n", video_path, movement_pixels);
                break;  // Sortir dès qu'un mouvement est détecté
            }
        }

        gray.copyTo(prev_gray);
        first_frame = false;

        // Afficher l'image avec les contours et le centre (pour voir la vidéo)
        imshow("Mouvement détecté", frame);
        if (waitKey(30) >= 0) { // Attendre 30ms, et fermer si une touche est pressée
            break;
        }
    }

    cap.release();

    // Si un mouvement a été détecté, envoyer un message au parent via le pipe
    if (movement_detected) {
        dprintf(pipe_fd, "Mouvement détecté dans %s\n", video_path);
    }
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

    // Créer un pipe pour la communication entre les processus parent et enfant
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Erreur lors de la création du pipe");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "videos/%s", entry->d_name);
            
            pid_t pid = fork();
            if (pid == 0) {
                // Processus enfant : fermer le côté lecture du pipe
                close(pipe_fd[0]);
                
                // Effectuer la détection de mouvement et envoyer le résultat au pipe
                detect_movement(filepath, pipe_fd[1]);
                
                // Fermer le côté écriture et sortir
                close(pipe_fd[1]);
                _exit(0);
            }
        }
    }

    // Processus parent : fermer le côté écriture du pipe
    close(pipe_fd[1]);

    // Lire les messages du pipe
    char buffer[512];
    while (read(pipe_fd[0], buffer, sizeof(buffer)) > 0) {
        printf("%s", buffer);
    }

    // Fermer le côté lecture du pipe et attendre la fin de tous les processus enfants
    close(pipe_fd[0]);
    while (wait(NULL) > 0);

    closedir(dir);

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps total d'exécution (Multiprocessus avec Pipe) : %.2f secondes\n", elapsed_time);

    return 0;
}

