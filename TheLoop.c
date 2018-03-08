#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "Ecran.h"
#include "GrilleSDL.h"
#include "Ressources.h"
#include "AStar.h"

// Dimensions de la grille de jeu
#define NB_LIGNES   15
#define NB_COLONNES 20

// Macros utilisees dans le tableau tab
#define VIDE     0
#define MUR      1

// Macros utilisees pour l'identite d'un thread
#define STATUE   2
#define MAGE     3
#define PISTON   4

// Compteur de billes
int nbBilles = 5;
pthread_mutex_t mutexNbBilles;

// Temps d'apparition d'une bille
int attenteBille = 3;
pthread_mutex_t mutexAttenteBilles;

// Compteur global de requêtes
#define NB_MAX_REQUETES 4
#define DELAI 20000000
int indRequetesE = 0, indRequetesL = 0, nbRequetesNonTraitees = 0;
CASE requetes[NB_MAX_REQUETES + 1];
pthread_mutex_t mutexRequetes;
pthread_cond_t condRequetes;
pthread_key_t cleStatue;

int tab[NB_LIGNES][NB_COLONNES]
={ {0,1,1,0,1,1,0,1,1,0,1,1,0,0,0,0,0,0,1,1},
   {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
   {1,1,0,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0},
   {0,0,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0},
   {0,1,1,1,0,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1},
   {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
   {0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0},
   {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
   {1,1,0,0,1,1,0,0,1,1,1,1,1,1,1,0,0,0,0,1},
   {0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1},
   {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,1},
   {0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1},
   {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,1},
   {0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1}};
pthread_mutex_t mutexTabGrille;

void initGrille();

typedef struct
{
	int id; //Identite du thread (statue ou mage) ! pas le pthread_self
	CASE position; //position dans la grille du perso
	int bille; //valeurs couleur(J/R/V/J) si transporte bille ou 0
} S_IDENTITE;

// Thread
void* ThreadPosseurBilles(void*);
void* ThreadBille(void*);
void* ThreadEvent(void*);
void* ThreadStatues(void*);

// Fonction
int FctEventClickGauche(int ligne,int colonne);
int Deplacement(CASE destination,int delai);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
	pthread_t tid_PosBilles, tid_Statue_1, tid_Statue_2, tid_Statue_3, tid_Statue_4, tid_Event;
	pthread_mutex_init(&mutexNbBilles,NULL);
	pthread_mutex_init(&mutexAttenteBilles,NULL);
	pthread_mutex_init(&mutexTabGrille,NULL);
	pthread_mutex_init(&mutexRequetes,NULL);
	
	srand((unsigned)time(NULL));

	// Ouverture de la fenetre graphique
	Trace("(THREAD MAIN %d) Ouverture de la fenetre graphique",pthread_self()); fflush(stdout);
	if (OuvertureFenetreGraphique() < 0)
	{
		Trace("Erreur de OuvrirGrilleSDL\n");
		exit(1);
	}

	//*******************
	initGrille();

// Etape_1
// initialisation du thread Posseur de billes
	pthread_create(&tid_PosBilles,NULL,ThreadPosseurBilles,NULL);

// Etape 2
// initialisation du thread Event
	pthread_create(&tid_Event,NULL,ThreadEvent,NULL);
	pthread_detach(tid_Event);


// Etape_3
	pthread_cond_init(&condRequetes,NULL);
	pthread_key_create(&cleStatue,NULL);

	CASE *pCase = (CASE*)malloc(sizeof(CASE));
	pCase->L = 0;
	pCase->C = 0;
	pthread_create(&tid_Statue_1,NULL,ThreadStatues,pCase);
	
	
	CASE *pCase2 = (CASE*)malloc(sizeof(CASE));
	pCase2->L = 0;
	pCase2->C = 3;
	pthread_create(&tid_Statue_2,NULL,ThreadStatues,pCase2);
	
	
	CASE *pCase3 = (CASE*)malloc(sizeof(CASE));
	pCase3->L = 0;
	pCase3->C = 6;
	pthread_create(&tid_Statue_3,NULL,ThreadStatues,pCase3);
	
	
	CASE *pCase4 = (CASE*)malloc(sizeof(CASE));
	pCase4->L = 0;
	pCase4->C = 9;
	pthread_create(&tid_Statue_4,NULL,ThreadStatues,pCase4);
	

// Attente de la fin du jeu, thread posseur de bille
	pthread_join(tid_PosBilles,NULL);
	free(pCase);
	free(pCase2);
	free(pCase3);
	free(pCase4);

/*	// Exemple d'utilisations des libriaires --> code à supprimer
	struct timespec delai;
	delai.tv_sec = 0;
	delai.tv_nsec = 200000000;
	nanosleep(&delai,NULL);

	CASE position;
	position.L = 0;
	position.C = 0;
	tab[position.L][position.C] = STATUE;
	DessineStatue(position.L,position.C,BAS,0);

	bool ok = false;
	EVENT_GRILLE_SDL event;
	while(!ok)
	{
		event = ReadEvent();
		if (event.type == CLIC_GAUCHE) ok = true;
	} 
    
	tab[event.ligne][event.colonne] = ROUGE;
	DessineBille(event.ligne,event.colonne,ROUGE);

  int  valeursAutorisees[1];
  CASE *chemin = NULL;  // Futur chemin
  int  nbCases;
  valeursAutorisees[0] = VIDEbool ok = false;
	EVENT_GRILLE_SDL event;
	while(!ok)
	{
		event = ReadEvent();
		if (event.type == CLIC_GAUCHE) ok = true;
	} ;

  CASE depart,arrivee;
  depart.L = position.L;
  depart.C = position.C;
  arrivee.L = event.ligne;
  arrivee.C = event.colonne; 

  nbCases = RechercheChemin(&tab[0][0],NB_LIGNES,NB_COLONNES,valeursAutorisees,1,depart,arrivee,&chemin);
  if (nbCases >= 1)
  {
    for (int i=0 ; i<nbCases ; i++)
    {
      tab[position.L][position.C] = VIDE;
      EffaceCarre(position.L,position.C);
      position.L = chemin[i].L;
      position.C = chemin[i].C;
      tab[position.L][position.C] = STATUE;
      DessineStatue(position.L,position.C,BAS,0);
      nanosleep(&delai,NULL);
    }
    EffaceCarre(position.L,position.C);
    D(position.L,position.C,BAS,ROUGE);
  }
  else Trace("(THREAD MAIN %d) Pas de chemin...",pthread_self()); 
  if (chemin) free(chemin);
*/
	
	setTitreGrilleSDL("GAME OVER");
  

  	pthread_exit(0);
}

// Fonction thread

void * ThreadPosseurBilles(void*)
{
	bool reprise = true;
	pthread_t tid_Bille;
	int dizaine,unite,color = JAUNE;
	timespec_t pause;	
	
	DessineBille(11,11,ROUGE);
	
	pthread_mutex_lock(&mutexNbBilles);
	while(nbBilles > 0)
	{
		pthread_mutex_unlock(&mutexNbBilles);
		
		if(nbRequetesNonTraitees == NB_MAX_REQUETES)
		{
			DessineBille(11,11,GRIS);
			reprise = false;
		}
		else
		{
		// Calcule le chiffre des dizaines et des unités et met le cpt a jour
			
			if((reprise == false && nbRequetesNonTraitees <= (NB_MAX_REQUETES / 2)) || (reprise == true && nbRequetesNonTraitees < NB_MAX_REQUETES))
			{
				reprise = true;
				DessineBille(11,11,ROUGE);
				
				if(nbBilles > 9)
				{
					dizaine = nbBilles / 10;
					unite = nbBilles % 10;
				}
				else
				{
					dizaine = 0;
					unite = nbBilles;
				}
	
				DessineChiffre(11,12,dizaine);
				DessineChiffre(11,13,unite);
		
				// création d'une bille
				switch(color)
				{
					case JAUNE : pthread_create(&tid_Bille,NULL,ThreadBille,&color);
								 pthread_detach(tid_Bille);
								 color = ROUGE;
						break;
					case ROUGE : pthread_create(&tid_Bille,NULL,ThreadBille,&color);
								 pthread_detach(tid_Bille);
								 color = VERT;
						break;
					case VERT :  pthread_create(&tid_Bille,NULL,ThreadBille,&color);
								 pthread_detach(tid_Bille);
								 color = VIOLET;
						break;
					case VIOLET : pthread_create(&tid_Bille,NULL,ThreadBille,&color);
								  pthread_detach(tid_Bille);
								  color = JAUNE;
				}
		
		
				pthread_mutex_lock(&mutexNbBilles);
				nbBilles--;
				pthread_mutex_unlock(&mutexNbBilles);
			}
		}
		
		pause.tv_sec = (rand() % 3) + 1;
		nanosleep(&pause,NULL);
		
		pthread_mutex_lock(&mutexNbBilles);
	}
	pthread_mutex_unlock(&mutexNbBilles);
	
	DessineChiffre(11,12,0);
	DessineChiffre(11,13,0);
	
	pthread_exit(0);

}

void* ThreadBille(void* c)
{
	int val, ligne, colonne;
	int *pcouleur = (int*)c;
	int couleur = *pcouleur;
	free(pcouleur);
	
	timespec_t pause;
	
// depose une bille dans la zone bleu
	val = rand() % 60;
	ligne = (val / 10) + 9;
	colonne = val % 10;
	

// Dessine une bille
	DessineBille(ligne,colonne,couleur);
	
	pthread_mutex_lock(&mutexTabGrille);
	tab[ligne][colonne] = couleur;
	pthread_mutex_unlock(&mutexTabGrille);
	
	pause.tv_sec = attenteBille;
	nanosleep(&pause,NULL);
	
// test si la bille est en prison
	pthread_mutex_lock(&mutexTabGrille);
	if(tab[ligne][colonne] > 0)
	{
	// Bille effacee\n");
		tab[ligne][colonne] = VIDE;
		EffaceCarre(ligne,colonne);
	}
	pthread_mutex_unlock(&mutexTabGrille);
	
	pthread_exit(0);
}


void* ThreadEvent(void*)
{
	bool ok = true;	
	
	while(ok)
	{
	    EVENT_GRILLE_SDL event;
		event = ReadEvent();
		switch(event.type)
		{
			case CLIC_GAUCHE:
					// teste le nombre de requetes en attende
						pthread_mutex_lock(&mutexRequetes);
						if(nbRequetesNonTraitees < NB_MAX_REQUETES)
						{
							if(!FctEventClickGauche(event.ligne,event.colonne))
							{
							
								requetes[indRequetesE].L = event.ligne;
								requetes[indRequetesE].C = event.colonne;

								if(indRequetesE == (NB_MAX_REQUETES + 1))
									indRequetesE = 0;
								else
									indRequetesE++;

								nbRequetesNonTraitees++;
								pthread_cond_signal(&condRequetes);
								
							}
							pthread_mutex_unlock(&mutexRequetes);
						}
						else
						{
							pthread_mutex_unlock(&mutexRequetes);
							if(tab[event.ligne][event.colonne] >= JAUNE && tab[event.ligne][event.colonne] <= VIOLET)
								DessineCroix(event.ligne,event.colonne);
						} 
				break;
			case CROIX : FermetureFenetreGraphique();
						 exit(1);
		}
	}

// Fermeture de la grille de jeu (SDL)
	Trace("(THREAD EVENT %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
	FermetureFenetreGraphique();
	Trace("(THREAD EVENT %d) OK Fin",pthread_self());
	
	exit(1);
}

int FctEventClickGauche(int ligne,int colonne)
{
	printf("Click gauche event, val tab : %d\n",tab[ligne][colonne]);
	printf(" Val ligne : %d, colonne : %d\n",ligne,colonne);
	
	pthread_mutex_lock(&mutexTabGrille);
	switch(tab[ligne][colonne])
	{
		case JAUNE : tab[ligne][colonne] = 0 - JAUNE;
					 DessinePrison(ligne,colonne,JAUNE);
			break;
		case ROUGE : tab[ligne][colonne] = 0 - ROUGE;
					 DessinePrison(ligne,colonne,JAUNE);
			break;
		case VERT :  tab[ligne][colonne] = 0 - VERT;
					 DessinePrison(ligne,colonne,JAUNE);
			break;
		case VIOLET : tab[ligne][colonne] = 0 - VIOLET;
					 DessinePrison(ligne,colonne,JAUNE);
			break;
		default : printf(" CLIC invalide\n");
					pthread_mutex_unlock(&mutexTabGrille);
				  return -1;
	}
	pthread_mutex_unlock(&mutexTabGrille);
	
	return 0;
}

void* ThreadStatues(void* pParamCase)
{
	CASE *pCase = (CASE*)pParamCase;
	int l = pCase->L, c = pCase->C;
	CASE posiBille;
	
	
	S_IDENTITE *IdentiteStatue = new S_IDENTITE;
	
	IdentiteStatue->id = STATUE;
	IdentiteStatue->position.L = l;
	IdentiteStatue->position.C = c;
	IdentiteStatue->bille = 0;
	
	pthread_setspecific(cleStatue,(void*)IdentiteStatue);
	
	
	printf(" THREAD statues tid( %d ): (%d,%d)\n",pthread_self(), l, c);
	
	pthread_mutex_lock(&mutexTabGrille);
	tab[l][c] = pthread_self();
	pthread_mutex_unlock(&mutexTabGrille);
	
	DessineStatue(l,c,BAS,0);
	
	
	pthread_mutex_lock(&mutexRequetes);
	while(indRequetesE == indRequetesL)
		pthread_cond_wait(&condRequetes,&mutexRequetes);
	posiBille.L = requetes[indRequetesL].L;
	posiBille.C = requetes[indRequetesL].C;
	
	indRequetesL++;
	if(indRequetesL == NB_MAX_REQUETES + 1)
		indRequetesL = 0;
	pthread_mutex_unlock(&mutexRequetes);
	pthread_cond_signal(&condRequetes);
	
	IdentiteStatue->bille = Deplacement(posiBille,DELAI);
	
	
	
	
	pthread_exit(0);
}

int Deplacement(CASE destination, int delai)
{
	int dispo = 0, caseAutorise[1] = {VIDE};
	timespec_t pause;
	CASE depart, *pChemin;
	S_IDENTITE *IdentiteStatue = new S_IDENTITE;
	
	
	IdentiteStatue = (S_IDENTITE*)pthread_getspecific(cleStatue);
	
	pause.tv_nsec = DELAI;
	pause.tv_sec = 0;
	
	IdentiteStatue = (S_IDENTITE*)pthread_getspecific(cleStatue);
	depart.L = IdentiteStatue->position.L;
	depart.C = IdentiteStatue->position.C;
	
	while(dispo >= 0)
	{
		dispo = -1;
		
		while(dispo == -1)
		{
			dispo = RechercheChemin(&tab[0][0], 15, 20, caseAutorise, 1, depart, destination,&pChemin);
			nanosleep(&pause,NULL);
		}
		printf("affiche dispo: %d case suite (%d,%d)\n",dispo,pChemin->L, pChemin->C);
		pthread_mutex_lock(&mutexTabGrille);
		
		if(dispo > 0 )
		{
			tab[pChemin->L][pChemin->C] = IdentiteStatue->id;
			DessineStatue(pChemin->L, pChemin->C,BAS,0);
			tab[depart.L][depart.C] = VIDE;
			EffaceCarre(depart.L,depart.C);
			depart.L = pChemin->L;
			depart.C = pChemin->C;
			nanosleep(&pause,NULL);
		}
		pthread_mutex_unlock(&mutexTabGrille);
		
		
	}
	printf("SORTIE dispo: %d case suite (%d,%d)\n",dispo,pChemin->L, pChemin->C);

}



//*********************************************************************************************
void  initGrille()
{
  for (int L=0 ; L<NB_LIGNES ; L++)
    for (int C=0 ; C<NB_COLONNES ; C++)
      if (tab[L][C] == MUR) DessineMur(L,C,PIERRE);
      
  DessineMur(11,11,METAL);
  DessineMur(13,11,METAL);
  DessineBille(13,11,GRIS);
}

//*********************************************************************************************
bool CaseReservee(CASE Case)
{
  // Cases depart des statues
  for (int i=0 ; i<4 ; i++)
    if ((Case.L == 0) && (Case.C == i*3)) return true;

  // Pile
  for (int C=12 ; C<=17 ; C++)
    if ((Case.L == 0) && (Case.C == C)) return true;

  // Depart Mage 1 et positions recupération bille hors de la pile
  for (int C=11 ; C<=17 ; C++)
    if ((Case.L == 1) && (Case.C == C)) return true;

  // Cases échange des mages
  for (int C=12 ; C<=13 ; C++)
    if ((Case.L == 5) && (Case.C == C)) return true;

  // Bonus vitesse, depart Mage 2, positions lancement bille par Mage 2
  for (int C=11 ; C<=18; C++)
    if ((Case.L == 7) && (Case.C == C)) return true;

  // Score (chrono)
  for (int C=11 ; C<=13; C++)
    if ((Case.L == 9) && (Case.C == C)) return true;

  // Compteur Billes
  for (int C=11 ; C<=13; C++)
    if ((Case.L == 11) && (Case.C == C)) return true;

  // Files
  for (int L=8 ; L<=12 ; L++)
    for (int C=15 ; C<=18 ; C++)
      if ((Case.L == L) && (Case.C == C)) return true;
  
  return false;
}

