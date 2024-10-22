#include "main.h"
#include "mx_init.h"
#include "filter_Coeff.h"
#include "musique.h"

#define AUDIOFREQ_16K 		((uint32_t)16000U)  //AUDIOFREQ_16K = 16 Khz
#define BUFFER_SIZE_INPUT	4000
#define BUFFER_SIZE_OUTPUT	16000
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


/*********** EFFET ECHO ***********/



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


/*********** EFFET REVERB ***********/


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

/*********** CALCUL DU NBR ECH POUR UNE PERIODE ***********/


uint32_t calculNbEchPeriod(uint32_t frequence);
uint32_t calculNbEchPeriod(uint32_t frequence){

	return  (AUDIOFREQ_16K / frequence);
}

/*********** CALCUL DU NBR ECH POUR UNE NOTE ***********/

uint32_t calculNbEchNote(float duree);
uint32_t calculNbEchNote(float duree){

	return duree * AUDIOFREQ_16K;
}

/*********** JOUE UNE NOTE DE MUSIQUE  ***********/

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

/*********** JOUER UNE MUSIQUE ENTIERE ***********/

void musicPlay(struct note musique[TAILLE_MUSIQUE]);
void musicPlay(struct note musique[TAILLE_MUSIQUE]){

	for (int i = 0; i < TAILLE_MUSIQUE; i++) {

		notePlayClassic(musique[i].freqNote, musique[i].dureeNote);
	}

}

/*********** INITIALISATION DU TAB SINUS ***********/

void initSinusTable(void);
void initSinusTable(void){
	for(int i=0; i<BUFFER_SIZE_SINUS; i++){
		sinusTable[i]= AMPLITUDE * sin((i * 2 * PI) / BUFFER_SIZE_SINUS);
	}
}

/*********** JOUE UNE NOTE EN FCT DE f ET d ***********/

void notePlayDDS(uint32_t frequence, float duree);
void notePlayDDS(uint32_t frequence, float duree){

	uint32_t nbEchNote   = calculNbEchNote(duree);


	for(uint32_t i=0; i < nbEchNote ; i++){

		TIME_START;

		echOutputLeft  = sinusTable[(i*frequence)% BUFFER_SIZE_SINUS];
		echOutputRight = echOutputLeft;

		TIME_STOP;

		/* Envoi des échantillons de sortie */
		HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputLeft, 1, SAI_WAIT);   // Gauche
		HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputRight, 1, SAI_WAIT);  // Droite
	}

}

/*********** JOUER UNE MUSIQUE ENTIERE VERSION  DSS ***********/

void musicPlayDDS(struct note musique[TAILLE_MUSIQUE]);
void musicPlayDDS(struct note musique[TAILLE_MUSIQUE]){

	for (int i = 0; i < TAILLE_MUSIQUE; i++) {

		notePlayDDS(musique[i].freqNote, musique[i].dureeNote);
	}

}

/*********** FILTRE IIR ***********/

void notePlayIIR(uint32_t frequence, float duree);
void notePlayIIR(uint32_t frequence, float duree){

	uint32_t nbEchNote   = calculNbEchNote(duree);
	uint32_t nbEchPeriod = calculNbEchPeriod(frequence);


	float a1 = -(sin((4*PI*frequence)/ AUDIOFREQ_16K) / sin((2*PI*frequence)/ AUDIOFREQ_16K));

	uint16_t y1 =  (double)(sin((2*PI*frequence)/ nbEchPeriod));
	uint16_t y2 = 0;



	for(uint32_t i=0; i < nbEchNote ; i++){

		TIME_START;

		echOutputLeft  =  a1 * y1 - y2 ; 							// + AMPLITUDE * sin((i * 2 * PI) / nbEchPeriod);
		echOutputRight = echOutputLeft;

		TIME_STOP;

		/* Envoi des échantillons de sortie */
		HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputLeft, 1, SAI_WAIT);   // Gauche
		HAL_SAI_Transmit(&hsai_BlockA2, (uint8_t *)&echOutputRight, 1, SAI_WAIT);  // Droite

		/*Enregristrement des outputs*/
		y1 = echOutputLeft;
		y2 = y1;
	}

}


/*********** JOUER UNE MUSIQUE ENTIERE VERSION  DSS avec filtre IIR ***********/

void musicPlayIIR(struct note musique[TAILLE_MUSIQUE]);
void musicPlayIIR(struct note musique[TAILLE_MUSIQUE]){

	for (int i = 0; i < TAILLE_MUSIQUE; i++) {

		notePlayIIR(musique[i].freqNote, musique[i].dureeNote);
	}

}


/********** MAIN ***********/


int main(void)
{
	SCB_EnableICache();
	SCB_EnableDCache();
	HAL_Init();
	BOARD_Init();
	//initSinusTable();


	while(1){

		//notePlayClassic(480, 0.45);
		//reverb();
		//echo();
		//musicPlay(musique);
		//musicPlayDDS(musique);
		musicPlayIIR(musique);
		//passThrough();
	}

}


