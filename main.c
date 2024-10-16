#include "main.h"
#include "mx_init.h"
#include "filter_Coeff.h"
#include "musique.h"

#define AUDIOFREQ_16K 		((uint32_t)16000U)  //AUDIOFREQ_16K = 16 Khz
#define BUFFER_SIZE_INPUT	4000
#define BUFFER_SIZE_OUTPUT	4000
#define BUFFER_SIZE_SINUS 	16000
#define BUFFER_SIZE_AUDIO	16000
#define AMPLITUDE			300
#define SAI_WAIT			100
#define TIME_START			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET)
#define TIME_STOP			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET)
#define PI					3.141592
#define Gain				0.5

extern SAI_HandleTypeDef hsai_BlockA2;
extern SAI_HandleTypeDef hsai_BlockB2;
extern TIM_HandleTypeDef htim3;

int16_t sinusTable[BUFFER_SIZE_SINUS]  = { 0 };
int16_t audioTable[BUFFER_SIZE_SINUS]  = { 0 };
int16_t bufferInputRight[BUFFER_SIZE_INPUT] = { 0 };
int16_t bufferInputLeft[BUFFER_SIZE_INPUT]  = { 0 };
int16_t bufferOutputRight[BUFFER_SIZE_OUTPUT] = { 0 };
int16_t bufferOutputLeft[BUFFER_SIZE_OUTPUT] = { 0 };
int16_t echInputLeft   = 0;
int16_t echInputRight  = 0;
int16_t echOutputRight = 0;
int16_t echOutputLeft  = 0;



void passThrough(void);


void passThrough(void){
	/* Reception des échantillons d'entrée */
	HAL_SAI_Receive (&hsai_BlockB2,(uint8_t *)&echInputLeft,1,SAI_WAIT);	// Left
	HAL_SAI_Receive (&hsai_BlockB2,(uint8_t *)&echInputRight,1,SAI_WAIT);	// Right

	/*  Output = Input */
	echOutputLeft = echInputLeft;
	echOutputRight = echInputRight;

	/* Envoi des échantillons de sortie */
	HAL_SAI_Transmit(&hsai_BlockA2,(uint8_t *)&echOutputLeft,1,SAI_WAIT);	// Left
	HAL_SAI_Transmit(&hsai_BlockA2,(uint8_t *)&echOutputRight,1,SAI_WAIT);	// Right
}





void echo(void);

void echo(void){
	static int16_t index = 0;

    /* Réception des échantillons d'entrée */
    HAL_SAI_Receive(&hsai_BlockB2, (uint8_t *)&echInputLeft, 1, SAI_WAIT);   // Gauche
    HAL_SAI_Receive(&hsai_BlockB2, (uint8_t *)&echInputRight, 1, SAI_WAIT);  // Droite

    /* Remplissage des buffers */
    bufferInputLeft[index] = echInputLeft;
    bufferInputRight[index] = echInputRight;


    echOutputLeft = echInputLeft + Gain * bufferInputLeft[(index + 1)%BUFFER_SIZE_INPUT];
    echOutputRight = echInputRight + Gain * bufferInputRight[(index + 1)%BUFFER_SIZE_INPUT];

    /* Mise à jour de l'index */
    index = (index + 1) % (BUFFER_SIZE_INPUT);

    /* Envoi des échantillons de sortie */
    HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputLeft, 1, SAI_WAIT);   // Gauche
    HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputRight, 1, SAI_WAIT);  // Droite

}


void reverb(void);

void reverb(void){
	static int16_t index = 0;

    /* Réception des échantillons d'entrée */
    HAL_SAI_Receive(&hsai_BlockB2, (uint8_t *)&echInputLeft, 1, SAI_WAIT);   // Gauche
    HAL_SAI_Receive(&hsai_BlockB2, (uint8_t *)&echInputRight, 1, SAI_WAIT);  // Droite

    /* Remplissage des buffers */
    bufferInputLeft[index] = echInputLeft;
    bufferInputRight[index] = echInputRight;

    	echOutputLeft  =  echInputLeft + Gain * bufferOutputLeft[(index + 1)%BUFFER_SIZE_INPUT];
    	echOutputRight =  echInputRight + Gain * bufferOutputRight[(index + 1)%BUFFER_SIZE_INPUT];


    /*Enregristrement des outputs*/

    bufferOutputLeft[index] = echOutputLeft;
    bufferOutputRight[index] = echOutputRight;

    /* Mise à jour de l'index */
    index = (index + 1) % (BUFFER_SIZE_INPUT);

    /* Envoi des échantillons de sortie */
    HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputLeft, 1, SAI_WAIT);   // Gauche
    HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputRight, 1, SAI_WAIT);  // Droite

}


uint32_t calculNbEchPeriod(uint32_t frequence);

uint32_t calculNbEchPeriod(uint32_t frequence){

	return  (AUDIOFREQ_16K / frequence);
}

uint32_t calculNbEchNote(float duree);

uint32_t calculNbEchNote(float duree){

	return duree * AUDIOFREQ_16K;
}


void notePlayClassic(uint32_t frequence, float duree);

void notePlayClassic(uint32_t frequence, float duree){

	uint32_t nbEchPeriod = calculNbEchPeriod(frequence);
	uint32_t nbEchNote   = calculNbEchNote(duree);



	for(uint32_t i=0; i < nbEchNote; i++){

	TIME_START;

	echOutputLeft  = AMPLITUDE * sin((i * 2 * PI) / nbEchPeriod);
	echOutputRight = echOutputLeft;

	TIME_STOP;

    /* Envoi des échantillons de sortie */
    HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputLeft, 1, SAI_WAIT);   // Gauche
    HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputRight, 1, SAI_WAIT);  // Droite
	}


}


void musicPlay(struct note musique[TAILLE_MUSIQUE]);

void musicPlay(struct note musique[TAILLE_MUSIQUE]){

    for (int i = 0; i < TAILLE_MUSIQUE; i++) {

		notePlayClassic(musique[i].freqNote, musique[i].dureeNote);
    }

}


int main(void)
{
	SCB_EnableICache();
	SCB_EnableDCache();
	HAL_Init();
	BOARD_Init();

	while(1){

		//notePlayClassic(480, 0.45);
		//reverb();
		//echo();
		musicPlay(musique);
		//passThrough();
	}

}


