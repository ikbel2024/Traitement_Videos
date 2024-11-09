#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>  // Nécessaire pour O_CREAT
#include <vector>   // Nécessaire pour std::vector
#include <opencv2/core/types.hpp> // Nécessaire pour cv::Point

using namespace cv;
using namespace std;

// Structure pour passer des paramètres aux threads
struct ThreadData {
    const char *video_path;
    sem_t *sem;
};

void *detect_movement(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
    const char *video_path = data->video_path;
    sem_t *sem = data->sem;

    printf("Traitement de la vidéo : %s dans le thread %lu\n", video_path, pthread_self());

    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        fprintf(stderr, "Erreur lors de l'ouverture de la vidéo %s\n", video_path);
        pthread_exit(NULL);
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
                sem_wait(sem); // Synchroniser l'accès au semaphore
                printf("Mouvement détecté dans %s, Nombre de pixels affectés : %d\n", video_path, movement_pixels);
                sem_post(sem); // Libérer le sémaphore
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

    // Initialiser un sémaphore pour synchroniser l'accès à la ressource partagée
    sem_t *sem = sem_open("/pipe_semaphore", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("Erreur lors de la création du sémaphore");
        return 1;
    }

    // Liste des vidéos à traiter
    pthread_t threads[256]; // Assurez-vous d'avoir assez de threads
    int thread_count = 0;

    // Parcours des vidéos et création des threads
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "videos/%s", entry->d_name);

            struct ThreadData *data = (struct ThreadData *)malloc(sizeof(struct ThreadData));
            data->video_path = filepath;
            data->sem = sem;

            // Créer un thread pour traiter la vidéo
            if (pthread_create(&threads[thread_count], NULL, detect_movement, data) != 0) {
                perror("Erreur lors de la création du thread");
                return 1;
            }
            thread_count++;
        }
    }

    // Attendre la fin de tous les threads
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    closedir(dir);

    // Nettoyer le sémaphore
    sem_close(sem);
    sem_unlink("/pipe_semaphore");

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Temps total d'exécution (Multithreading avec Sémaphore) : %.2f secondes\n", elapsed_time);

    return 0;
}

