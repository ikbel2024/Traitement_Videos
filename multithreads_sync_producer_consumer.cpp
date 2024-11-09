#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <opencv2/opencv.hpp>

#define BUFFER_SIZE 5  // Taille du buffer

int buffer[BUFFER_SIZE]; // Buffer partagé
int in = 0;              // Indice d'ajout dans le buffer
int out = 0;             // Indice de retrait dans le buffer

sem_t empty; // Semaphore pour les emplacements vides
sem_t full;  // Semaphore pour les emplacements remplis
sem_t mutex; // Semaphore pour la mutualisation d'accès

// Fonction du Producteur
void* producer(void* arg) {
    int item;
    for (int i = 0; i < 10; i++) {
        item = rand() % 100; // Génère un élément aléatoire

        // Attente pour un emplacement vide
        sem_wait(&empty);

        // Accès exclusif au buffer
        sem_wait(&mutex);
        buffer[in] = item;
        printf("Producteur produit : %d à l'emplacement %d\n", item, in);
        in = (in + 1) % BUFFER_SIZE; // Met à jour l'indice d'ajout
        sem_post(&mutex);

        // Signal qu'il y a un nouvel élément dans le buffer
        sem_post(&full);

        sleep(1); // Pause pour simuler un délai de production
    }
    return NULL;
}

// Fonction du Consommateur
void* consumer(void* arg) {
    int item;
    for (int i = 0; i < 10; i++) {
        // Attente pour un emplacement rempli
        sem_wait(&full);

        // Accès exclusif au buffer
        sem_wait(&mutex);
        item = buffer[out];
        printf("Consommateur consomme : %d depuis l'emplacement %d\n", item, out);
        out = (out + 1) % BUFFER_SIZE; // Met à jour l'indice de retrait
        sem_post(&mutex);

        // Signal qu'il y a un emplacement vide
        sem_post(&empty);

        sleep(1); // Pause pour simuler un délai de consommation
    }
    return NULL;
}

void display_buffer(int* buffer) {
    // Crée une image avec OpenCV pour afficher l'état du buffer
    cv::Mat img = cv::Mat::zeros(500, 1000, CV_8UC3); // Image vide pour l'affichage
    int x = 50; // Position de départ sur l'axe X
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // Dessiner chaque élément du buffer
        cv::Scalar color = (buffer[i] > 0) ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);  // Vert si rempli, rouge si vide
        cv::rectangle(img, cv::Point(x, 200), cv::Point(x + 100, 300), color, -1);
        cv::putText(img, std::to_string(buffer[i]), cv::Point(x + 35, 250), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
        x += 150;  // Espace entre les éléments
    }
    
    // Affichage de l'image avec l'état du buffer
    cv::imshow("Buffer", img);
    cv::waitKey(1); // Mise à jour de l'affichage en temps réel
}

int main() {
    // Initialisation des sémaphores
    sem_init(&empty, 0, BUFFER_SIZE); // Au départ, tous les emplacements sont vides
    sem_init(&full, 0, 0);            // Aucun emplacement rempli au départ
    sem_init(&mutex, 0, 1);           // Accès exclusif (1) pour le buffer

    pthread_t producer_thread, consumer_thread;

    // Création des threads de producteur et consommateur
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // Affichage du buffer pendant l'exécution des threads
    while (true) {
        display_buffer(buffer);
        usleep(100000);  // Attendre 0.1 seconde avant de rafraîchir
    }

    // Attente de la fin des threads
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Destruction des sémaphores
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    printf("Production et consommation terminées.\n");

    // Fermeture de toutes les fenêtres OpenCV
    cv::destroyAllWindows();

    return 0;
}

