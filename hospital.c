#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define BUSY 0
#define FREE 1
#define NUM_PATIENTS 16

pthread_mutex_t mutex;
int number_threads;
int nebulizadorLinkById[4];
int nebulizadorState[4];

int activePatients;
int patientHP[NUM_PATIENTS];
int activeThreads[NUM_PATIENTS];
int isConsulting[NUM_PATIENTS];

int curedPatients = 0;
int deadPatients = 0;
int totalOfPatients = 0;

void* doctor(void* arg){
    int thread_id = *(int*)arg;
    int consultTime;
    int maxConsultTime;
    int isStillWorking = 0;
    int menor;
    int menorId;
    int firstPatient = 0;

    while(1) {
        if(activePatients > 0 || firstPatient == 0){
            if(!isStillWorking){
                maxConsultTime = rand() % (6 - 2) + 2;
                menor = 192179877;
                for(int i = 0; i < NUM_PATIENTS; i++){
                    if(menor > patientHP[i]){
                        if(activeThreads[i]){
                            menor = patientHP[i];
                            menorId = i;
                        }
                    }
                }

                isConsulting[menorId] = 1;
                printf("\n\nMEDICO COMEÇOU CONSULTA COM PACIENTE %d de %d turnos\n\n", menorId, maxConsultTime);
                isStillWorking = 1;
                if(firstPatient==0){
                    firstPatient = 1;
                }
            }
            else {
                if(consultTime == maxConsultTime){
                    printf("\n\nMEDICO DEU ALTA PARA O PACIENTE %d\n", menorId);
                    printf("PACIENTES ESPERANDO:  %d\n\n", activePatients-1);
                    consultTime = 0;
                    isConsulting[menorId] = 0;
                    isStillWorking = 0;
                    patientHP[menorId] = 999;
                }
                else {
                    consultTime++;
                }
            }
        }else{
            break;
        }

        sleep(1);
        
    }


}

void* patient(void* arg){
    int thread_id = *(int*)arg;
    int isUsingNebulizador = 0;

    patientHP[thread_id] = rand() % (10 - 5) + 5;
    
    while(1){
        if(patientHP[thread_id] == 0){
            printf("\n\n\x1b[31mPACIENTE %d MORREU\x1b[0m\n\n", thread_id);
            deadPatients++;
            activeThreads[thread_id] = 0;
            activePatients--;
            return 0;
        }
        if(patientHP[thread_id] == 999){ //RECEBEU_ALTA
            printf("\n\nPACIENTE %d RECEBEU ALTA\n\n", thread_id);
            curedPatients++;
            activeThreads[thread_id] = 0;
            activePatients--;
            return 0;
        }

        if(!isConsulting[thread_id]){
            // if(thread_id == 0){
            //     printf("\n\nstatus dos nebulizadores:\n%d %d %d %d\n%d %d %d %d\n\n", nebulizadorState[0], nebulizadorState[1], nebulizadorState[2], nebulizadorState[3], nebulizadorLinkById[0], nebulizadorLinkById[1], nebulizadorLinkById[2], nebulizadorLinkById[3]);
            // }

            for(int i = 0; i < 4; i++){
                if(!isUsingNebulizador && nebulizadorState[i] == FREE){
                    isUsingNebulizador = 1;
                    nebulizadorLinkById[i] = thread_id;
                    nebulizadorState[i] = BUSY;
                    patientHP[thread_id]+= rand() % (4 - 2) + 2;
                    printf("Nebulizador %d foi ocupado por %d: \x1b[32mHP-%d\x1b[0m\n", i, thread_id, patientHP[thread_id]);
                    break;
                }
                else if(isUsingNebulizador && (nebulizadorLinkById[i] == thread_id) && (nebulizadorState[i] == BUSY) ){
                    if(patientHP[thread_id] < 5){
                        patientHP[thread_id]+= rand() % (4 - 2) + 2;
                        printf("Enfermeiro deixou o paciente %d no nebulizador - \x1b[32mHP-%d\x1b[0m\n", thread_id, patientHP[thread_id]);
                    }
                    else {
                        isUsingNebulizador = 0;
                        nebulizadorState[i] = FREE;
                        nebulizadorLinkById[i] = -1;
                        printf("Nebulizador %d foi desocupado por %d\n", i, thread_id);
                    }
                    break; 
                }
            }
            
            if(!isUsingNebulizador){
                // printf("Não foi a vez do %d\n", thread_id);
                patientHP[thread_id] -= (rand() % (2 - 1) + 1);
            }
        }

        sleep(1);
    }
}

void killThreads(pthread_t *threads){
    for (int i = 0; i < NUM_PATIENTS + 1; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
}

int main(){
    srand(time(NULL));
    int numPatients = 0;
    pthread_t threads[NUM_PATIENTS+1];
    int doctorThreadId = 16;
    int threads_ids[NUM_PATIENTS];
    int i;
    time_t startTime = time(NULL);
    int duracaoSegundos = 60;

    for(i = 0; i < 4; i++){ //inicializa os nebulizadores como livres sem vinculo
        nebulizadorState[i] = FREE;
        nebulizadorLinkById[i] = -1;
    }

    for(i = 0; i < NUM_PATIENTS; i++){ //inicializa as threads inativas
        activeThreads[i] = 0;
        isConsulting[i] = 0;
    }
   
    pthread_mutex_init(&mutex, NULL);
    pthread_create(&threads[NUM_PATIENTS+1], NULL, doctor, &doctorThreadId);

    while((time(NULL) - startTime) < duracaoSegundos){
        if(activePatients < NUM_PATIENTS){
            for(int i = 0; i < NUM_PATIENTS; i++){
                if(!activeThreads[i]){
                    printf("\ncriou novo paciente: %d\n", i);
                    activeThreads[i] = 1;
                    activePatients++;
                    threads_ids[i] = i;
                    totalOfPatients++;
                    pthread_create(&threads[i], NULL, patient, &threads_ids[i]);
                    numPatients++;
                    break;
                }
            }
        }
    }

    while(1){
        if(activePatients <= 0){
            printf("\n\n--------------------------------------------\n");
            printf("Total de Pacientes curados: %d\n", curedPatients);
            printf("Total de Pacientes mortos: %d\n", deadPatients);
            printf("Total de Pacientes que chegaram no hospital: %d", totalOfPatients);
            printf("\n--------------------------------------------\n");
            killThreads(threads);
            return 0;
        }
    }
}