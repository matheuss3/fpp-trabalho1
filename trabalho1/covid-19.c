#pragma once
#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

// Boolean
#define TRUE 1
#define FALSE 0

#define QTD_LABS 3

// Ingredientes da vacina
#define VIRUS_MORTO 1
#define INJECAO 2
#define INSUMO_SECRETO 3

// Slot de suprimento vazio
#define SLOT_VAZIO -1

// Dados laboratorio
typedef struct {
	pthread_t tid;
	int thread_num;

	int suprimento_inifito1;
	int suprimento_inifito2;
	int estoque_si1;
	int estoque_si2;

	int qtd_renovacao_estoque;
	
	sem_t semaph_count;
	pthread_mutex_t mutex;

	int* nao_acabou;

	int* meta;
	int meta_batida;
} inf_laboratorio;

typedef struct {
	inf_laboratorio* laboratorios[QTD_LABS];
	pthread_mutex_t mutex;
} inf_mesa;

// Dados infectado
typedef struct {
	pthread_t tid;
	int thread_num;

	int suprimento_inifito;
	int slot1;
	int slot2;
	int qtd_vacinas_aplicadas;

	int* nao_acabou;
	inf_mesa* mesa;
	int* meta;
	int meta_batida;
} inf_infectado;

void* pegar_ingredientes(inf_infectado* infectado) {
	for (int i = 0; i < QTD_LABS; i++) { // Procurando os ingredientes na mesa
		inf_laboratorio* laboratorio = infectado->mesa->laboratorios[i];
		
		pthread_mutex_lock(&(laboratorio->mutex));
		if (laboratorio->estoque_si1 > 0 
			&& infectado->suprimento_inifito != laboratorio->suprimento_inifito1
			&& infectado->slot1 != laboratorio->suprimento_inifito1
			&& infectado->slot2 != laboratorio->suprimento_inifito1) {
			
			if (infectado->slot1 == SLOT_VAZIO) {
				infectado->slot1 = laboratorio->suprimento_inifito1;
			}
			else {
				infectado->slot2 = laboratorio->suprimento_inifito1;
			}

			laboratorio->estoque_si1--;
		}

		

		if (laboratorio->estoque_si2 > 0
			&& infectado->suprimento_inifito != laboratorio->suprimento_inifito2
			&& infectado->slot1 != laboratorio->suprimento_inifito2
			&& infectado->slot2 != laboratorio->suprimento_inifito2) {

			if (infectado->slot1 == SLOT_VAZIO) {
				infectado->slot1 = laboratorio->suprimento_inifito2;
			}
			else {
				infectado->slot2 = laboratorio->suprimento_inifito2;
			}

			laboratorio->estoque_si2--;
		}
		pthread_mutex_unlock(&(laboratorio->mutex));
		sem_post(&(laboratorio->semaph_count));

	}
}

void* thread_laboratorio(void* arg) {
	inf_laboratorio* il = (inf_laboratorio*)arg;
	
	while (*(il->nao_acabou)) {
		sem_wait(&(il->semaph_count));
		// Verificar o estoque
		pthread_mutex_lock(&(il->mutex));
		if (il->estoque_si1 == 0 && il->estoque_si2 == 0) {
			il->estoque_si1 = 1;
			il->estoque_si2 = 1;
			il->qtd_renovacao_estoque ++;
		}
		
		if (il->qtd_renovacao_estoque >= *(il->meta)) {
			il->meta_batida = TRUE;
		}
		pthread_mutex_unlock(&(il->mutex));
		sem_post(&(il->semaph_count));
	}

	return 0;
}

void* thread_infectado(void* arg) {
	inf_infectado* iinf = (inf_infectado*)arg;
	while (*(iinf->nao_acabou)) {
		if (iinf->slot1 != SLOT_VAZIO && iinf->slot2 != SLOT_VAZIO) {
			iinf->slot1 = SLOT_VAZIO;
			iinf->slot2 = SLOT_VAZIO;
			iinf->qtd_vacinas_aplicadas++;
		}

		if (iinf->qtd_vacinas_aplicadas >= *(iinf->meta)) {
			iinf->meta_batida = TRUE;
		}
		pthread_mutex_lock(&(iinf->mesa->mutex));
		pegar_ingredientes(iinf);
		pthread_mutex_unlock(&(iinf->mesa->mutex));
	}

	return 0;
}

void inicializa_infectado(inf_infectado* iinf, int sup_inf, 
	int thread_num, int* meta, int* flag, inf_mesa* mesa) {
	iinf->thread_num = thread_num;
	
	iinf->qtd_vacinas_aplicadas = 0;

	iinf->slot1 = SLOT_VAZIO;
	iinf->slot2 = SLOT_VAZIO;
	iinf->suprimento_inifito = sup_inf;
	
	iinf->meta_batida = FALSE;

	iinf->mesa = mesa;
	iinf->meta = meta;
	iinf->nao_acabou = flag;
}

void inicializa_laboratorio(inf_laboratorio* il, int sup_inf1, 
	int sup_inf2, int thread_num, int* meta, int* flag) {
	il->thread_num = thread_num;
	sem_init(&(il->semaph_count), 0, 0);
	il->mutex = PTHREAD_MUTEX_INITIALIZER;

	il->suprimento_inifito1 = sup_inf1;
	il->estoque_si1 = 1;

	il->suprimento_inifito2 = sup_inf2;
	il->estoque_si2 = 1;

	il->qtd_renovacao_estoque = 0;
	
	il->nao_acabou = flag;

	il->meta = meta;
	il->meta_batida = FALSE;
}

int main(int argc, char *argv[]) {
	// Meta de operações primordiais
	int meta = 10;
	if (argc > 1) {
		meta = atoi(argv[1]);
	}

	// Flag de finalização das operações
	int nao_acabou = TRUE;

	inf_laboratorio l1;
	inicializa_laboratorio(&l1, INJECAO, VIRUS_MORTO, 1, &meta, &nao_acabou);
	inf_laboratorio l2;
	inicializa_laboratorio(&l2, INJECAO, INSUMO_SECRETO, 2, &meta, &nao_acabou);
	inf_laboratorio l3;
	inicializa_laboratorio(&l3, INSUMO_SECRETO, VIRUS_MORTO, 3, &meta, &nao_acabou);

	inf_mesa mesa;
	mesa.laboratorios[0] = &l1;
	mesa.laboratorios[1] = &l2;
	mesa.laboratorios[2] = &l3;
	mesa.mutex = PTHREAD_MUTEX_INITIALIZER;

	inf_infectado i1;
	inicializa_infectado(&i1, VIRUS_MORTO, 1, &meta, &nao_acabou, &mesa);
	inf_infectado i2;
	inicializa_infectado(&i2, INJECAO, 2, &meta, &nao_acabou, &mesa);
	inf_infectado i3;
	inicializa_infectado(&i3, INSUMO_SECRETO, 3, &meta, &nao_acabou, &mesa);


	pthread_create(&(l1.tid), NULL, thread_laboratorio, &l1);
	pthread_create(&(l2.tid), NULL, thread_laboratorio, &l2);
	pthread_create(&(l3.tid), NULL, thread_laboratorio, &l3);
	pthread_create(&(i1.tid), NULL, thread_infectado, &i1);
	pthread_create(&(i2.tid), NULL, thread_infectado, &i2);
	pthread_create(&(i3.tid), NULL, thread_infectado, &i3);

	while (nao_acabou) {
		if (l1.meta_batida == TRUE && i1.meta_batida == TRUE
			&& l2.meta_batida == TRUE && i2.meta_batida == TRUE
			&& l3.meta_batida == TRUE && i3.meta_batida == TRUE) {
			nao_acabou = FALSE;
		}
	}

	pthread_join(l1.tid, NULL);
	pthread_join(i1.tid, NULL);
	pthread_join(l2.tid, NULL);
	pthread_join(i2.tid, NULL);
	pthread_join(l3.tid, NULL);
	pthread_join(i3.tid, NULL);

	printf("infectado 1: %d\n", i1.qtd_vacinas_aplicadas);
	printf("infectado 2: %d\n", i2.qtd_vacinas_aplicadas);
	printf("infectado 3: %d\n", i3.qtd_vacinas_aplicadas);
	printf("laboratorio 1: %d\n", l1.qtd_renovacao_estoque);
	printf("laboratorio 2: %d\n", l2.qtd_renovacao_estoque);
	printf("laboratorio 3: %d\n", l3.qtd_renovacao_estoque);

	return 0;
}