#include <iostream>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <cstring>
#include <ctime>

using namespace cv;
using namespace std;

// Fonction pour détecter les mouvements dans la vidéo
int detect_movement(const char *video_path) {
    cout << "Traitement de la vidéo : " << video_path << endl;
    VideoCapture cap(video_path);
    
    if (!cap.isOpened()) {
        cerr << "Erreur lors de l'ouverture de la vidéo " << video_path << endl;
        return -1;
    }

    Mat frame, gray, prev_gray, diff;
    bool first_frame = true;

    while (cap.read(frame)) {
        cvtColor(frame, gray, COLOR_BGR2GRAY);  // Conversion en niveaux de gris
        
        if (!first_frame) {
            absdiff(prev_gray, gray, diff); // Différence absolue entre les images
            threshold(diff, diff, 25, 255, THRESH_BINARY); // Seuil pour détecter les changements
            
            // Trouver les contours des zones de mouvement
            vector<vector<Point>> contours;
            findContours(diff, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            
            for (const auto &contour : contours) {
                Moments m = moments(contour);
                if (m.m00 > 0) {  // Vérifie que le contour a une zone non nulle
                    int x = static_cast<int>(m.m10 / m.m00); // Coordonnée x du centre
                    int y = static_cast<int>(m.m01 / m.m00); // Coordonnée y du centre
                    cout << "Mouvement détecté dans " << video_path << " à la position : (" << x << ", " << y << ")" << endl;

                    // Dessiner le contour et le centre sur l'image
                    drawContours(frame, contours, -1, Scalar(0, 255, 0), 2); // Dessiner les contours en vert
                    circle(frame, Point(x, y), 5, Scalar(0, 0, 255), -1);  // Dessiner un cercle rouge au centre
                    
                    // Afficher le texte avec la position sur l'image
                    char text[50];
                    snprintf(text, sizeof(text), "Pos: (%d, %d)", x, y);
                    putText(frame, text, Point(x + 10, y + 10), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 0, 0), 2); // Texte en bleu
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
        cerr << "Impossible d'ouvrir le dossier de vidéos" << endl;
        return 1;
    }

    // Lire chaque fichier dans le dossier "videos"
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Si c'est un fichier (pas un dossier)
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "videos/%s", entry->d_name);
            detect_movement(filepath);  // Appeler la fonction pour détecter le mouvement
        }
    }

    closedir(dir);  // Fermer le dossier

    clock_t end_time = clock();  // Fin du chronomètre
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    cout << "Temps total d'exécution (Monothread) : " << elapsed_time << " secondes" << endl;

    return 0;
}

